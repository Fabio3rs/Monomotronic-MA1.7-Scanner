#pragma once

#include <fstream>
#include <string>
#include <vector>

struct SensorState; // Forward declaration

namespace FileIO {
// Directory management
std::string GetUserDataDirectory(); // Returns ~/.config/ecu_monitor
void CreateDirectoryIfNotExists(const std::string &path);

// Filename generation
std::string
GenerateTimestampedFilename(const std::string &prefix,   // "sensor_recording"
                            const std::string &extension // ".csv"
);

// CSV writing (nullptr sensor_indices = all sensors)
void WriteCSVHeader(std::ofstream &file,
                    const std::vector<SensorState> &sensors,
                    const std::vector<int> *sensor_indices = nullptr,
                    bool include_delta_ms = false);

void WriteCSVRow(std::ofstream &file, double timestamp,
                 const std::vector<SensorState> &sensors,
                 const std::vector<int> *sensor_indices = nullptr);

void WriteCSVRow(std::ofstream &file, double timestamp, double delta_ms,
                 const std::vector<SensorState> &sensors,
                 const std::vector<int> *sensor_indices = nullptr);
} // namespace FileIO
