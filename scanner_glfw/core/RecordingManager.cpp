#include "RecordingManager.h"

#include "../app_data.h"
#include "../utils/FileIO.h"
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace {
constexpr const char *kRecordingPrefix = "sensor_recording";
constexpr const char *kRecordingExtension = ".csv";
constexpr const char *kRecordingsDirName = "recordings";

std::string FormatLocalTimestamp(std::chrono::system_clock::time_point tp) {
    const std::time_t time_value = std::chrono::system_clock::to_time_t(tp);
    std::tm local_tm{};
#if defined(_WIN32)
    localtime_s(&local_tm, &time_value);
#else
    localtime_r(&time_value, &local_tm);
#endif
    std::ostringstream out;
    out << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string BuildSensorList(const std::vector<SensorState> &sensors,
                            const std::vector<int> *indices) {
    std::ostringstream out;
    bool first = true;
    if (indices) {
        for (int idx : *indices) {
            if (idx < 0 || idx >= static_cast<int>(sensors.size())) {
                continue;
            }
            if (!first) {
                out << "; ";
            }
            out << sensors[static_cast<size_t>(idx)].name << " ("
                << sensors[static_cast<size_t>(idx)].id << ":"
                << sensors[static_cast<size_t>(idx)].subcmd << ")";
            first = false;
        }
        return out.str();
    }

    for (const auto &sensor : sensors) {
        if (!first) {
            out << "; ";
        }
        out << sensor.name << " (" << sensor.id << ":" << sensor.subcmd << ")";
        first = false;
    }
    return out.str();
}

std::size_t CountValidSensors(const std::vector<SensorState> &sensors,
                              const std::vector<int> *indices) {
    if (!indices) {
        return sensors.size();
    }

    std::size_t count = 0;
    for (int idx : *indices) {
        if (idx >= 0 && idx < static_cast<int>(sensors.size())) {
            ++count;
        }
    }
    return count;
}
} // namespace

RecordingManager &RecordingManager::Instance() {
    static RecordingManager instance;
    return instance;
}

bool RecordingManager::StartRecording(const std::vector<SensorState> &sensors,
                                      std::vector<int> sensor_indices) {
    if (recording_) {
        return false;
    }

    if (sensors.empty()) {
        return false;
    }

    sensor_indices_ = std::move(sensor_indices);

    if (!OpenFile(sensors)) {
        sensor_indices_.clear();
        return false;
    }

    recording_ = true;
    loggingActive = true;
    sample_count_ = 0;
    last_sample_sequence_ = 0;
    first_sample_timestamp_sec_ = 0.0;
    has_first_sample_timestamp_ = false;
    start_time_ = std::chrono::steady_clock::now();
    last_sample_time_ = start_time_ - sample_interval_;
    return true;
}

void RecordingManager::StopRecording() {
    if (!recording_) {
        return;
    }

    CloseFile();
    recording_ = false;
    loggingActive = false;
    sensor_indices_.clear();
}

void RecordingManager::Update(const std::vector<SensorState> &sensors,
                              uint64_t sample_sequence,
                              double sample_timestamp_sec,
                              double sample_delta_sec) {
    if (!recording_ || !recording_file_.is_open()) {
        return;
    }

    if (sample_sequence == 0 || sample_sequence == last_sample_sequence_) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!ShouldWriteSample(now)) {
        return;
    }

    const std::vector<int> *indices =
        sensor_indices_.empty() ? nullptr : &sensor_indices_;
    double elapsed = 0.0;
    if (sample_timestamp_sec > 0.0) {
        if (!has_first_sample_timestamp_) {
            first_sample_timestamp_sec_ = sample_timestamp_sec;
            has_first_sample_timestamp_ = true;
        }
        elapsed = sample_timestamp_sec - first_sample_timestamp_sec_;
        if (elapsed < 0.0) {
            elapsed = 0.0;
        }
    } else {
        elapsed = std::chrono::duration<double>(now - start_time_).count();
    }
    const double delta_ms = sample_delta_sec * 1000.0;

    FileIO::WriteCSVRow(recording_file_, elapsed, delta_ms, sensors, indices);
    if (recording_file_.fail()) {
        CloseFile();
        recording_ = false;
        loggingActive = false;
        return;
    }

    last_sample_time_ = now;
    last_sample_sequence_ = sample_sequence;
    ++sample_count_;
}

void RecordingManager::SetSampleInterval(
    std::chrono::milliseconds interval) noexcept {
    sample_interval_ = interval;
}

bool RecordingManager::OpenFile(const std::vector<SensorState> &sensors) {
    std::string base_dir = FileIO::GetUserDataDirectory();
    FileIO::CreateDirectoryIfNotExists(base_dir);

    std::string recordings_dir = base_dir + "/" + kRecordingsDirName;
    FileIO::CreateDirectoryIfNotExists(recordings_dir);

    const std::string filename = FileIO::GenerateTimestampedFilename(
        kRecordingPrefix, kRecordingExtension);
    current_path_ = recordings_dir + "/" + filename;

    recording_file_.open(current_path_, std::ios::out | std::ios::trunc);
    if (!recording_file_.is_open()) {
        current_path_.clear();
        return false;
    }

    const std::vector<int> *indices =
        sensor_indices_.empty() ? nullptr : &sensor_indices_;
    recording_file_ << "# recording_start_local: "
                    << FormatLocalTimestamp(std::chrono::system_clock::now())
                    << "\n";
    recording_file_ << "# sample_interval_ms: " << sample_interval_.count();
    if (sample_interval_.count() == 0) {
        recording_file_ << " (sync)";
    }
    recording_file_ << "\n";
    recording_file_ << "# ecu_connected: " << (ecuConnected ? "true" : "false")
                    << "\n";
    recording_file_ << "# simulation_mode: "
                    << (simulationModeActive ? "true" : "false") << "\n";
    recording_file_ << "# kline_table_active: " << kLineTableActive << "\n";

    if (g_ecu_info) {
        recording_file_ << "# ecu_model: " << g_ecu_info->model << "\n";
        recording_file_ << "# ecu_firmware: " << g_ecu_info->firmware_version
                        << "\n";
        recording_file_ << "# ecu_hardware_id: " << g_ecu_info->hardware_id
                        << "\n";
        recording_file_ << "# ecu_table_id: " << g_ecu_info->table_id << "\n";
    }

    const std::size_t sensor_count = CountValidSensors(sensors, indices);
    recording_file_ << "# sensor_count: " << sensor_count << "\n";
    recording_file_ << "# sensor_list: " << BuildSensorList(sensors, indices)
                    << "\n";

    FileIO::WriteCSVHeader(recording_file_, sensors, indices, true);

    if (recording_file_.fail()) {
        CloseFile();
        std::error_code error;
        std::filesystem::remove(current_path_, error);
        current_path_.clear();
        return false;
    }

    return true;
}

void RecordingManager::CloseFile() {
    if (recording_file_.is_open()) {
        recording_file_.close();
    }
    current_path_.clear();
}

bool RecordingManager::ShouldWriteSample(
    const std::chrono::steady_clock::time_point &now) const noexcept {
    if (sample_interval_.count() == 0) {
        return true;
    }
    return (now - last_sample_time_) >= sample_interval_;
}
