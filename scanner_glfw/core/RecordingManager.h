#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct SensorState;

class RecordingManager {
  public:
    static RecordingManager &Instance();

    bool StartRecording(const std::vector<SensorState> &sensors,
                        std::vector<int> sensor_indices = {});
    void StopRecording();
    bool IsRecording() const noexcept { return recording_; }

    void Update(const std::vector<SensorState> &sensors,
                uint64_t sample_sequence, double sample_timestamp_sec,
                double sample_delta_sec);

    void SetSampleInterval(std::chrono::milliseconds interval) noexcept;
    std::chrono::milliseconds GetSampleInterval() const noexcept {
        return sample_interval_;
    }

    uint64_t GetSampleCount() const noexcept { return sample_count_; }
    const std::string &GetCurrentPath() const noexcept { return current_path_; }
    const std::string &GetLastError() const noexcept { return last_error_; }

  private:
    bool HasValidSensors(const std::vector<SensorState> &sensors) const
        noexcept;
    void FilterSensorIndices(const std::vector<SensorState> &sensors,
                             std::vector<int> &indices);

    RecordingManager() = default;
    ~RecordingManager() = default;

    RecordingManager(const RecordingManager &) = delete;
    RecordingManager &operator=(const RecordingManager &) = delete;

    bool OpenFile(const std::vector<SensorState> &sensors);
    void CloseFile();
    bool ShouldWriteSample(
        const std::chrono::steady_clock::time_point &now) const noexcept;

    std::ofstream recording_file_;
    std::string current_path_;
    std::vector<int> sensor_indices_;
    std::string last_error_;
    bool recording_ = false;
    uint64_t sample_count_ = 0;
    uint64_t last_sample_sequence_ = 0;
    double first_sample_timestamp_sec_ = 0.0;
    bool has_first_sample_timestamp_ = false;

    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_sample_time_;
    std::chrono::milliseconds sample_interval_{0};
};
