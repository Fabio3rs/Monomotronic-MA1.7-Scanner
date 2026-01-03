#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "../app_data.h"
#include "ECUMonomotronic.h"

// Lightweight wrapper around ECUMonomotronic that polls sensors and exposes
// DTC operations to the UI. All ECU interactions are serialized through a
// single background thread to avoid conflicting commands.
class ECUBackend {
  public:
    struct DTCResult {
        bool success;
        std::vector<SimulatedDTC> dtcs;
        std::string error_message;
    };

    static constexpr size_t kMaxSensors = 64;
    static constexpr size_t kCollectionSlots = 10;

    static ECUBackend &Instance();

    bool Start(const std::string &port, std::vector<SensorState> *sensors);
    void Stop();

    // Copy latest polled samples into the UI-owned SensorState vector.
    bool DrainSamples(std::vector<SensorState> &sensors);

    DTCResult ReadDTCs();
    DTCResult ClearDTCs();

    // Get active sensor collection from ECU (based on RAM_B3 register)
    // Returns sensor (id, subcmd) pairs from table1 or table2
    std::optional<std::vector<std::pair<int, int>>> GetActiveSensorCollection();

    bool IsConnected() const {
        return connected_.load(std::memory_order_relaxed);
    }
    float GetLatencyMs() const {
        return latency_ms_.load(std::memory_order_relaxed);
    }
    float GetErrorRate() const {
        return error_rate_.load(std::memory_order_relaxed);
    }
    int GetLastTableId() const {
        return last_table_id_.load(std::memory_order_relaxed);
    }
    double GetLastSampleTimestampSec() const {
        return last_sample_timestamp_sec_.load(std::memory_order_relaxed);
    }
    const std::optional<ECUInfo> &GetECUInfo() const noexcept {
        return ecu_info_;
    }
    void SetECUInfo(const ECUInfo &info) noexcept { ecu_info_ = info; }

    // Get logs from ECU communication
    void GetLogs(std::string &buffer);

    // Public ECU instance access for advanced features
    std::unique_ptr<ECUMonomotronic> ecu_;

    // Sensor subscription API (fixed-size bitmask, NASA P10 compliant)
    void SubscribeSensor(int sensor_id);
    void UnsubscribeSensor(int sensor_id);
    void SubscribeSensors(const std::vector<int> &sensor_ids);
    void ClearSubscriptions();
    bool IsSensorActive(int sensor_id) const;

  private:
    static constexpr auto kRamB3CacheDuration = std::chrono::seconds(5);

    ECUBackend();
    ~ECUBackend();

    ECUBackend(const ECUBackend &) = delete;
    ECUBackend &operator=(const ECUBackend &) = delete;

    struct SensorSample {
        int index;
        int raw;
        double timestamp;
    };

    void WorkerLoop();
    void PollSensors(double timestamp);
    bool PollSensorCollection(
        double timestamp, int table_id,
        const std::array<bool, kCollectionSlots> &requested_slots,
        std::array<SensorSample, kMaxSensors> &out_samples, size_t &out_count);
    bool PollSingleSensor(const SensorState &sensor, int index,
                          double timestamp, SensorSample &outSample);
    void UpdateHealthSnapshot();
    void BuildCollectionLookup();
    void CollectActiveSensorIndices(uint64_t mask,
                                    std::array<int, kMaxSensors> &out,
                                    size_t &count) const;
    void PartitionSensors(const std::array<int, kMaxSensors> &active_indices,
                          size_t active_count, int current_table,
                          std::array<bool, kCollectionSlots> &requested_slots,
                          std::array<int, kMaxSensors> &collection_indices,
                          size_t &collection_count,
                          std::array<int, kMaxSensors> &individual_indices,
                          size_t &individual_count) const;
    int DetermineCollectionTable();
    void InvalidateRamB3Cache();

    std::thread worker_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::atomic<float> latency_ms_;
    std::atomic<float> error_rate_;
    std::atomic<int> last_table_id_;
    std::vector<SensorState> *sensors_;
    std::array<int, kCollectionSlots> table1_indices_;
    std::array<int, kCollectionSlots> table2_indices_;
    size_t sensor_count_;
    uint64_t all_sensors_mask_;
    std::atomic<uint64_t> active_sensor_mask_;
    std::atomic<uint8_t> cached_ram_b3_;
    std::atomic<bool> ram_b3_valid_;
    std::chrono::steady_clock::time_point ram_b3_cache_time_;
    std::mutex command_mutex_;
    std::mutex sample_mutex_;
    std::vector<SensorSample> pending_samples_;
    std::chrono::steady_clock::time_point start_time_;
    std::string port_;
    std::optional<ECUInfo> ecu_info_;
    std::atomic<double> last_sample_timestamp_sec_;

    void ProcessInitPackets(std::span<const ECUmmpacket> packets) noexcept;
};
