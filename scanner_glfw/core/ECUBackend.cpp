#include "ECUBackend.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <span>
#include <sstream>
#include <thread>

#include "ECUMonomotronic.h"
#include "SensorDecoders.h"

namespace {
constexpr std::chrono::milliseconds kPollInterval{50};
constexpr std::chrono::milliseconds kIdleInterval{100};
constexpr size_t kInitStringExpectedSize = 39;
constexpr uint8_t kInitFrameId = ECUMonomotronic::ECU_INIT_STRING;

// Aggregate sensor collection tables (RAM_B3 selection bitmask)
constexpr std::array<std::pair<uint8_t, uint8_t>, ECUBackend::kCollectionSlots>
    kCollectionTable1{{{0x63, 0x00},
                       {0x66, 0x00},
                       {0x47, 0x00},
                       {0x62, 0x00},
                       {0x0C, 0xF8},
                       {0x6C, 0xF8},
                       {0x6F, 0xF8},
                       {0x3D, 0xF8},
                       {0x61, 0x00},
                       {0x69, 0xF8}}};

constexpr std::array<std::pair<uint8_t, uint8_t>, ECUBackend::kCollectionSlots>
    kCollectionTable2{{{0x63, 0x00},
                       {0x66, 0x00},
                       {0x70, 0x00},
                       {0x2F, 0x00},
                       {0x72, 0xF8},
                       {0x6C, 0xF8},
                       {0x6F, 0xF8},
                       {0x40, 0xF8},
                       {0x61, 0x00},
                       {0x69, 0xF8}}};

bool HasRequestedCollectionSlot(
    const std::array<bool, ECUBackend::kCollectionSlots> &requested) {
    for (bool slot : requested) {
        if (slot) {
            return true;
        }
    }
    return false;
}
} // namespace

ECUBackend &ECUBackend::Instance() {
    static ECUBackend instance;
    return instance;
}

ECUBackend::ECUBackend()
    : running_(false), connected_(false), latency_ms_(0.0f), error_rate_(0.0f),
      last_table_id_(0), sensors_(nullptr), table1_indices_{},
      table2_indices_{}, sensor_count_(0), all_sensors_mask_(0),
      active_sensor_mask_(0), cached_ram_b3_(0), ram_b3_valid_(false),
      ram_b3_cache_time_(), last_sample_timestamp_sec_(0.0) {}

ECUBackend::~ECUBackend() { Stop(); }

bool ECUBackend::Start(const std::string &port,
                       std::vector<SensorState> *sensors) {
    if (running_) {
        return true;
    }

    if (sensors == nullptr) {
        return false;
    }

    port_ = port;
    sensors_ = sensors;

    BuildCollectionLookup();

    const uint64_t current_mask =
        active_sensor_mask_.load(std::memory_order_relaxed);
    if (current_mask == 0) {
        active_sensor_mask_.store(all_sensors_mask_, std::memory_order_relaxed);
    } else {
        active_sensor_mask_.store(current_mask & all_sensors_mask_,
                                  std::memory_order_relaxed);
    }
    cached_ram_b3_.store(0, std::memory_order_relaxed);
    ram_b3_valid_.store(false, std::memory_order_relaxed);

    ecu_ = std::make_unique<ECUMonomotronic>(port_.c_str());

    if (!ecu_->portIsOpen()) {
        ecu_.reset();
        return false;
    }

    ecu_->init();
    ecu_->shouldTryAutoConnect(true);
    ProcessInitPackets(ecu_->getinitPackets());

    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    worker_ = std::thread(&ECUBackend::WorkerLoop, this);
    return true;
}

void ECUBackend::Stop() {
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
    ecu_.reset();
    active_sensor_mask_.store(0, std::memory_order_relaxed);
    ram_b3_valid_.store(false, std::memory_order_relaxed);
    sensor_count_ = 0;
    all_sensors_mask_ = 0;
    table1_indices_.fill(-1);
    table2_indices_.fill(-1);
    sensors_ = nullptr;
    ecu_info_.reset();
    g_ecu_info.reset();
    last_sample_timestamp_sec_.store(0.0, std::memory_order_relaxed);
}

void ECUBackend::SubscribeSensor(int sensor_id) {
    if (sensor_id < 0 || static_cast<size_t>(sensor_id) >= kMaxSensors) {
        return;
    }

    const uint64_t bit = 1ULL << sensor_id;
    active_sensor_mask_.fetch_or(bit, std::memory_order_relaxed);
}

void ECUBackend::UnsubscribeSensor(int sensor_id) {
    if (sensor_id < 0 || static_cast<size_t>(sensor_id) >= kMaxSensors) {
        return;
    }

    const uint64_t bit = 1ULL << sensor_id;
    active_sensor_mask_.fetch_and(~bit, std::memory_order_relaxed);
}

void ECUBackend::SubscribeSensors(const std::vector<int> &sensor_ids) {
    uint64_t mask = 0;
    const size_t limit = kMaxSensors;
    for (int id : sensor_ids) {
        if (id < 0) {
            continue;
        }
        const size_t idx = static_cast<size_t>(id);
        if (idx >= limit) {
            continue;
        }
        mask |= (1ULL << idx);
    }

    active_sensor_mask_.store(mask, std::memory_order_relaxed);
}

void ECUBackend::ClearSubscriptions() {
    active_sensor_mask_.store(0, std::memory_order_relaxed);
}

bool ECUBackend::IsSensorActive(int sensor_id) const {
    if (sensor_id < 0 || static_cast<size_t>(sensor_id) >= kMaxSensors) {
        return false;
    }

    const uint64_t mask = active_sensor_mask_.load(std::memory_order_acquire);
    return (mask & (1ULL << sensor_id)) != 0;
}

bool ECUBackend::DrainSamples(std::vector<SensorState> &sensors) {
    std::vector<SensorSample> samples;
    {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        samples.swap(pending_samples_);
    }

    if (samples.empty()) {
        return false;
    }

    for (const auto &sample : samples) {
        if (sample.index < 0 ||
            static_cast<size_t>(sample.index) >= sensors.size()) {
            continue;
        }
        sensors[sample.index].pushSample(sample.raw, sample.timestamp);
    }

    return true;
}

ECUBackend::DTCResult ECUBackend::ReadDTCs() {
    DTCResult result{false, {}, "ECU backend unavailable"};

    if (!ecu_) {
        return result;
    }

    std::lock_guard<std::mutex> lock(command_mutex_);
    auto resp = ecu_->ECUReadErrors();
    if (!resp) {
        result.error_message = "ECUReadErrors() returned no data";
        return result;
    }

    result.success = true;
    result.error_message.clear();

    for (const auto &pkt : resp->get_packets()) {
        if (pkt.frametypeid != ECUMonomotronic::ECU_ERROR_DATA_CODE) {
            continue;
        }

        bool present = false;
        auto desc = ECUMonomotronic::errorPacketToString(pkt, present);

        auto data = pkt.get_data();
        if (data.size() < 2) {
            continue;
        }

        uint16_t code_value = static_cast<uint16_t>(data[0]) |
                              (static_cast<uint16_t>(data[1]) << 8);

        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
            << code_value;

        SimulatedDTC dtc;
        dtc.code = oss.str();
        dtc.description = desc.empty() ? "Unknown error" : std::string(desc);
        dtc.active = present;

        result.dtcs.push_back(std::move(dtc));
    }

    return result;
}

ECUBackend::DTCResult ECUBackend::ClearDTCs() {
    DTCResult result{false, {}, "ECU backend unavailable"};

    if (!ecu_) {
        return result;
    }

    std::lock_guard<std::mutex> lock(command_mutex_);
    auto resp = ecu_->ECUCleanErrors();
    if (!resp) {
        result.error_message = "ECUCleanErrors() returned no data";
        return result;
    }

    result.success = true;
    result.error_message.clear();
    return result;
}

void ECUBackend::GetLogs(std::string &buffer) {
    buffer.clear();

    if (ecu_) {
        ecu_->strprintlogging(buffer);
        if (buffer.empty()) {
            buffer = "No logs available (ECU logging buffer is empty).";
        }
    } else {
        buffer = "No logs available (ECU not initialized).";
    }
}

std::optional<std::vector<std::pair<int, int>>>
ECUBackend::GetActiveSensorCollection() {
    if (!ecu_) {
        return std::nullopt;
    }

    // Acquire mutex (blocks worker thread like ReadDTCs does)
    std::lock_guard<std::mutex> lock(command_mutex_);

    // Read RAM_B3 to determine active table
    auto table_resp = ecu_->readECUMemory(0x00, 0xB3, 1);
    if (!table_resp || table_resp->count == 0) {
        return std::nullopt;
    }

    // Get first packet data
    auto data = table_resp->packets[0].get_data();
    if (data.empty()) {
        return std::nullopt;
    }

    const uint8_t ram_b3 = data[0];

    // Table selection logic matching DecodeSensorCollection()
    const auto *table = (ram_b3 & 0x01)   ? &kCollectionTable2
                        : (ram_b3 & 0x02) ? &kCollectionTable1
                                          : nullptr;

    if (table == nullptr) {
        return std::nullopt; // Unknown table selection
    }

    // Extract sensor IDs from selected table
    std::vector<std::pair<int, int>> sensor_ids;
    sensor_ids.reserve(10);
    for (const auto &[id, subcmd] : *table) {
        sensor_ids.emplace_back(static_cast<int>(id), static_cast<int>(subcmd));
    }

    return sensor_ids;
}

void ECUBackend::WorkerLoop() {
    while (running_) {
        if (!ecu_ || sensors_ == nullptr) {
            connected_.store(false, std::memory_order_relaxed);
            InvalidateRamB3Cache();
            std::this_thread::sleep_for(kIdleInterval);
            continue;
        }

        const bool connected_now = ecu_->isECUConnectedNow();
        connected_.store(connected_now, std::memory_order_relaxed);
        if (!connected_now) {
            InvalidateRamB3Cache();
        }
        UpdateHealthSnapshot();

        if (!ecu_->isThreadRunning() || !ecu_->canAcceptCommands()) {
            std::this_thread::sleep_for(kIdleInterval);
            continue;
        }

        const double timestamp =
            std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                          start_time_)
                .count();

        PollSensors(timestamp);
        std::this_thread::sleep_for(kPollInterval);
    }
}

void ECUBackend::PollSensors(double timestamp) {
    if (sensors_ == nullptr || sensor_count_ == 0) {
        return;
    }

    const uint64_t mask = active_sensor_mask_.load(std::memory_order_acquire);
    if (mask == 0) {
        return; // No active subscriptions
    }

    std::array<int, kMaxSensors> active_indices{};
    size_t active_count = 0;
    CollectActiveSensorIndices(mask, active_indices, active_count);

    if (active_count == 0) {
        return;
    }

    std::array<SensorSample, kMaxSensors> samples{};
    size_t sample_count = 0;

    std::lock_guard<std::mutex> lock(command_mutex_);

    const int table_id = DetermineCollectionTable();

    std::array<bool, kCollectionSlots> requested_slots{};
    std::array<int, kMaxSensors> collection_indices{};
    size_t collection_count = 0;
    std::array<int, kMaxSensors> individual_indices{};
    size_t individual_count = 0;

    PartitionSensors(active_indices, active_count, table_id, requested_slots,
                     collection_indices, collection_count, individual_indices,
                     individual_count);

    if ((table_id == 1 || table_id == 2) && collection_count > 0 &&
        HasRequestedCollectionSlot(requested_slots)) {
        const bool collection_ok = PollSensorCollection(
            timestamp, table_id, requested_slots, samples, sample_count);
        if (!collection_ok) {
            for (size_t i = 0;
                 i < collection_count && individual_count < kMaxSensors; ++i) {
                individual_indices[individual_count++] = collection_indices[i];
            }
        }
    }

    for (size_t i = 0; i < individual_count && sample_count < kMaxSensors;
         ++i) {
        const int idx = individual_indices[i];
        if (idx < 0 || static_cast<size_t>(idx) >= sensor_count_) {
            continue;
        }

        const auto &sensor = (*sensors_)[idx];
        SensorSample sample{};
        if (PollSingleSensor(sensor, idx, timestamp, sample)) {
            samples[sample_count++] = sample;
        }
    }

    if (sample_count > 0) {
        std::lock_guard<std::mutex> sample_lock(sample_mutex_);
        pending_samples_.insert(pending_samples_.end(), samples.begin(),
                                samples.begin() +
                                    static_cast<std::ptrdiff_t>(sample_count));
        last_sample_timestamp_sec_.store(timestamp, std::memory_order_relaxed);
    }
}

bool ECUBackend::PollSingleSensor(const SensorState &sensor, int index,
                                  double timestamp, SensorSample &outSample) {
    auto resp = ecu_->readECUMemory(static_cast<uint8_t>(sensor.subcmd),
                                    static_cast<uint8_t>(sensor.id),
                                    static_cast<uint8_t>(sensor.dataLength));
    if (!resp) {
        return false;
    }

    for (const auto &pkt : resp->get_packets()) {
        if (pkt.frametypeid == ECUMonomotronic::ECU_ACK_CODE) {
            continue;
        }

        const auto data = pkt.get_data();
        if (data.size() < static_cast<size_t>(sensor.dataLength)) {
            continue;
        }

        int raw = 0;
        if (sensor.dataLength == 2) {
            raw = (static_cast<int>(data[0]) << 8) | static_cast<int>(data[1]);
        } else {
            raw = data[0];
        }

        outSample = SensorSample{index, raw, timestamp};
        return true;
    }

    return false;
}

void ECUBackend::UpdateHealthSnapshot() {
    if (!ecu_) {
        latency_ms_.store(0.0f, std::memory_order_relaxed);
        error_rate_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    auto snapshot = ecu_->getHealthSnapshot();

    const double avg_latency_ms =
        static_cast<double>(snapshot.avg_loop_latency_us) / 1000.0;
    latency_ms_.store(static_cast<float>(avg_latency_ms),
                      std::memory_order_relaxed);

    const double total_packets =
        static_cast<double>(snapshot.total_packets_sent) +
        static_cast<double>(snapshot.total_packets_received);

    if (total_packets > 0.0) {
        const double recovery =
            static_cast<double>(snapshot.total_recovery_cycles);
        error_rate_.store(static_cast<float>(recovery / total_packets),
                          std::memory_order_relaxed);
    } else {
        error_rate_.store(0.0f, std::memory_order_relaxed);
    }
}

bool ECUBackend::PollSensorCollection(
    double timestamp, int table_id,
    const std::array<bool, kCollectionSlots> &requested_slots,
    std::array<SensorSample, kMaxSensors> &out_samples, size_t &out_count) {
    if (table_id != 1 && table_id != 2) {
        return false;
    }

    const auto &table = (table_id == 1) ? kCollectionTable1 : kCollectionTable2;
    const auto &index_lookup =
        (table_id == 1) ? table1_indices_ : table2_indices_;

    auto resp = ecu_->requestSensorCollection();
    if (!resp) {
        InvalidateRamB3Cache();
        return false;
    }

    bool decoded = false;

    for (const auto &pkt : resp->get_packets()) {
        if (pkt.frametypeid == ECUMonomotronic::ECU_ACK_CODE) {
            continue;
        }

        const auto data = pkt.get_data();
        if (data.size() < table.size()) {
            continue;
        }

        for (size_t i = 0; i < table.size() && out_count < out_samples.size();
             ++i) {
            if (!requested_slots[i]) {
                continue;
            }

            const int sensor_index = index_lookup[i];
            if (sensor_index < 0 ||
                static_cast<size_t>(sensor_index) >= sensor_count_) {
                continue;
            }

            const int raw = data[i];
            out_samples[out_count++] =
                SensorSample{sensor_index, raw, timestamp};
        }

        decoded = true;
    }

    if (decoded) {
        last_table_id_.store(table_id, std::memory_order_relaxed);
    } else {
        InvalidateRamB3Cache();
    }

    return decoded;
}

void ECUBackend::BuildCollectionLookup() {
    table1_indices_.fill(-1);
    table2_indices_.fill(-1);
    sensor_count_ =
        (sensors_ != nullptr)
            ? std::min(sensors_->size(), static_cast<size_t>(kMaxSensors))
            : 0;
    all_sensors_mask_ = 0;

    for (size_t i = 0; i < sensor_count_; ++i) {
        all_sensors_mask_ |= (1ULL << i);
    }

    if (!sensors_ || sensor_count_ == 0) {
        active_sensor_mask_.store(0, std::memory_order_relaxed);
        return;
    }

    const auto fill_table = [this](const auto &table,
                                   std::array<int, kCollectionSlots> &indices) {
        for (size_t slot = 0; slot < table.size(); ++slot) {
            const auto &entry = table[slot];
            for (size_t i = 0; i < sensor_count_; ++i) {
                const auto &sensor = (*sensors_)[i];
                if (sensor.id == static_cast<int>(entry.first) &&
                    sensor.subcmd == static_cast<int>(entry.second)) {
                    indices[slot] = static_cast<int>(i);
                    break;
                }
            }
        }
    };

    fill_table(kCollectionTable1, table1_indices_);
    fill_table(kCollectionTable2, table2_indices_);
}

void ECUBackend::CollectActiveSensorIndices(uint64_t mask,
                                            std::array<int, kMaxSensors> &out,
                                            size_t &count) const {
    count = 0;
    const size_t limit =
        std::min(sensor_count_, static_cast<size_t>(kMaxSensors));
    for (size_t i = 0; i < limit; ++i) {
        if (mask & (1ULL << i)) {
            out[count++] = static_cast<int>(i);
        }
    }
}

void ECUBackend::PartitionSensors(
    const std::array<int, kMaxSensors> &active_indices, size_t active_count,
    int current_table, std::array<bool, kCollectionSlots> &requested_slots,
    std::array<int, kMaxSensors> &collection_indices, size_t &collection_count,
    std::array<int, kMaxSensors> &individual_indices,
    size_t &individual_count) const {
    requested_slots.fill(false);
    collection_count = 0;
    individual_count = 0;

    const std::array<int, kCollectionSlots> *lookup = nullptr;
    if (current_table == 1) {
        lookup = &table1_indices_;
    } else if (current_table == 2) {
        lookup = &table2_indices_;
    }

    for (size_t i = 0; i < active_count; ++i) {
        const int idx = active_indices[i];
        bool handled = false;

        if (lookup) {
            for (size_t slot = 0; slot < lookup->size(); ++slot) {
                if ((*lookup)[slot] == idx) {
                    requested_slots[slot] = true;
                    if (collection_count < kMaxSensors) {
                        collection_indices[collection_count++] = idx;
                    }
                    handled = true;
                    break;
                }
            }
        }

        if (!handled && individual_count < kMaxSensors) {
            individual_indices[individual_count++] = idx;
        }
    }
}

int ECUBackend::DetermineCollectionTable() {
    const auto now = std::chrono::steady_clock::now();
    if (ram_b3_valid_.load(std::memory_order_acquire)) {
        const auto elapsed = now - ram_b3_cache_time_;
        if (elapsed < kRamB3CacheDuration) {
            const uint8_t cached =
                cached_ram_b3_.load(std::memory_order_relaxed);
            if (cached & 0x01) {
                return 2;
            }
            if (cached & 0x02) {
                return 1;
            }
            return 0;
        }
    }

    ram_b3_valid_.store(false, std::memory_order_release);

    auto b3_resp = ecu_->readECUMemory(0x00, 0xB3, 1);
    if (!b3_resp) {
        return 0;
    }

    for (const auto &pkt : b3_resp->get_packets()) {
        if (pkt.frametypeid == ECUMonomotronic::ECU_ACK_CODE) {
            continue;
        }
        auto data = pkt.get_data();
        if (!data.empty()) {
            const uint8_t ram_b3 = data[0];
            cached_ram_b3_.store(ram_b3, std::memory_order_release);
            ram_b3_valid_.store(true, std::memory_order_release);
            ram_b3_cache_time_ = now;

            if (ram_b3 & 0x01) {
                return 2;
            }
            if (ram_b3 & 0x02) {
                return 1;
            }
            return 0;
        }
    }

    return 0;
}

void ECUBackend::InvalidateRamB3Cache() {
    ram_b3_valid_.store(false, std::memory_order_release);
}

namespace {
std::string TrimSegment(std::string_view input) {
    const auto begin = input.find_first_not_of(' ');
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = input.find_last_not_of(' ');
    return std::string(input.substr(begin, end - begin + 1));
}

ECUInfo ParseInitString(std::string_view raw) {
    ECUInfo info{};
    if (raw.size() < kInitStringExpectedSize) {
        return info;
    }

    info.hardware_id = TrimSegment(raw.substr(16, 1));
    info.firmware_version = TrimSegment(raw.substr(17, 7));
    info.model = TrimSegment(raw.substr(30, 5));
    info.table_id = static_cast<std::uint32_t>(kLineTableActive);
    return info;
}
} // namespace

void ECUBackend::ProcessInitPackets(
    std::span<const ECUmmpacket> packets) noexcept {
    for (const auto &pkt : packets) {
        if (pkt.frametypeid != kInitFrameId) {
            continue;
        }

        const auto data = pkt.get_data();
        if (data.empty()) {
            continue;
        }

        const std::string raw(data.begin(), data.end());
        ECUInfo parsed = ParseInitString(raw);
        if (!parsed.IsValid()) {
            continue;
        }

        ecu_info_ = parsed;
        g_ecu_info = ecu_info_;
        if (on_ecu_info_received) {
            on_ecu_info_received(*ecu_info_);
        }
        break;
    }
}
