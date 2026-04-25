#include "FileIO.h"
#include "../app_data.h"
#include <ctime>
#include <iomanip>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace FileIO {

std::string GetUserDataDirectory() {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd *pwd = getpwuid(getuid());
        if (pwd) {
            home_dir = pwd->pw_dir;
        }
    }

    if (home_dir) {
        return std::string(home_dir) + "/.config/ecu_monitor";
    }

    return "./ecu_monitor_data";
}

void CreateDirectoryIfNotExists(const std::string &path) {
    mkdir(path.c_str(), 0755);
}

std::string GenerateTimestampedFilename(const std::string &prefix,
                                        const std::string &extension) {
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);

    char buffer[256];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", local_time);

    return prefix + "_" + std::string(buffer) + extension;
}

void WriteCSVHeader(std::ofstream &file,
                    const std::vector<SensorState> &sensors,
                    const std::vector<int> *sensor_indices,
                    bool include_delta_ms) {
    file << "Timestamp (s)";
    if (include_delta_ms) {
        file << ",Delta (ms)";
    }

    if (sensor_indices == nullptr) {
        for (const auto &sensor : sensors) {
            file << "," << sensor.name << " (" << sensor.unit << ")";
        }
    } else {
        for (int idx : *sensor_indices) {
            if (idx >= 0 && idx < static_cast<int>(sensors.size())) {
                const auto &sensor = sensors[idx];
                file << "," << sensor.name << " (" << sensor.unit << ")";
            }
        }
    }

    file << "\n";
}

void WriteCSVRow(std::ofstream &file, double timestamp,
                 const std::vector<SensorState> &sensors,
                 const std::vector<int> *sensor_indices) {
    file << std::fixed << std::setprecision(3) << timestamp;

    if (sensor_indices == nullptr) {
        for (const auto &sensor : sensors) {
            file << "," << std::fixed << std::setprecision(2)
                 << sensor.lastValue;
        }
    } else {
        for (int idx : *sensor_indices) {
            if (idx >= 0 && idx < static_cast<int>(sensors.size())) {
                const auto &sensor = sensors[idx];
                file << "," << std::fixed << std::setprecision(2)
                     << sensor.lastValue;
            }
        }
    }

    file << "\n";
    // Performance: rely on std::ofstream internal buffering instead of
    // flushing on every row. The RecordingManager destructor or StopRecording
    // will close the stream and flush naturally.
}

void WriteCSVRow(std::ofstream &file, double timestamp, double delta_ms,
                 const std::vector<SensorState> &sensors,
                 const std::vector<int> *sensor_indices) {
    file << std::fixed << std::setprecision(3) << timestamp;
    file << "," << std::fixed << std::setprecision(2) << delta_ms;

    if (sensor_indices == nullptr) {
        for (const auto &sensor : sensors) {
            file << "," << std::fixed << std::setprecision(2)
                 << sensor.lastValue;
        }
    } else {
        for (int idx : *sensor_indices) {
            if (idx >= 0 && idx < static_cast<int>(sensors.size())) {
                const auto &sensor = sensors[idx];
                file << "," << std::fixed << std::setprecision(2)
                     << sensor.lastValue;
            }
        }
    }

    file << "\n";
}

} // namespace FileIO
