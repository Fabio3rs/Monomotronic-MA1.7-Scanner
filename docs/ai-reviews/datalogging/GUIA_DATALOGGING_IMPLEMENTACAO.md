# Guia Prático: Implementar DataLogging Completo

## 📋 Estrutura de Dados

### **1. RecordingManager (Controlar Gravação)**

> Status 2026-02-22: implementado no código (CSV, Start/Stop, intervalo opcional). Próximos passos: JSON, compressão, replay/estatísticas.

```cpp
// scanner_glfw/core/RecordingManager.h

#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <thread>
#include <queue>
#include <fstream>

struct SensorSnapshot {
    uint64_t timestamp_ms;  // Milliseconds since recording start
    float value;            // Sensor value
    std::string unit;       // "RPM", "g/s", etc
    int sensor_id;          // Index in sensor table
    std::string sensor_name;
};

struct RecordingConfig {
    bool enabled = true;
    std::string output_dir = "~/ecu_logs";
    std::string format = "csv";  // csv, json
    std::vector<int> filtered_sensors;  // Empty = all
    uint32_t buffer_size_mb = 100;
    bool compress = false;
};

class RecordingManager {
public:
    static RecordingManager& Instance();

    // Control
    bool StartRecording(const std::string &session_name);
    void StopRecording();
    bool IsRecording() const { return recording_; }

    // Data
    void LogSensorSnapshot(const SensorSnapshot &snapshot);
    void LogMessage(const std::string &message);

    // Config
    void SetConfig(const RecordingConfig &config);
    RecordingConfig GetConfig() const;

    // Info
    std::string GetCurrentFilename() const;
    uint64_t GetBytesWritten() const;
    uint64_t GetSampleCount() const;

private:
    RecordingManager();
    ~RecordingManager();

    void WriteThreadMain();
    void FlushBuffer();

    bool recording_ = false;
    RecordingConfig config_;
    std::string current_filename_;
    std::ofstream output_file_;

    std::queue<SensorSnapshot> buffer_;
    std::thread write_thread_;
    std::mutex buffer_mutex_;

    uint64_t samples_written_ = 0;
    std::chrono::system_clock::time_point start_time_;
};
```

### **2. DataLogFile (Escrever no Disco)**

```cpp
// scanner_glfw/core/DataLogFile.h

#pragma once
#include <string>
#include <fstream>
#include <vector>
#include "RecordingManager.h"

class DataLogFile {
public:
    explicit DataLogFile(const std::string &filename);
    ~DataLogFile();

    bool OpenCSV();
    bool OpenJSON();

    void WriteCSVHeader(const std::vector<std::string> &sensor_names);
    void WriteCSVRow(const SensorSnapshot &snapshot);
    void WriteJSONObject(const SensorSnapshot &snapshot);
    void WriteJSONFooter();

    void Close();
    bool IsOpen() const { return file_.is_open(); }

    size_t GetFileSize() const;

private:
    std::string filename_;
    std::ofstream file_;
    bool is_json_ = false;
};

// Implementação
DataLogFile::DataLogFile(const std::string &filename)
    : filename_(filename) {}

bool DataLogFile::OpenCSV() {
    file_.open(filename_, std::ios::app);
    if (!file_.is_open()) {
        fprintf(stderr, "ERROR: Cannot open %s\n", filename_.c_str());
        return false;
    }
    is_json_ = false;
    return true;
}

void DataLogFile::WriteCSVHeader(const std::vector<std::string> &sensor_names) {
    file_ << "timestamp_ms,sensor_id,sensor_name,value,unit\n";
    file_.flush();
}

void DataLogFile::WriteCSVRow(const SensorSnapshot &snapshot) {
    file_ << snapshot.timestamp_ms << ","
          << snapshot.sensor_id << ","
          << snapshot.sensor_name << ","
          << snapshot.value << ","
          << snapshot.unit << "\n";

    // Flush every N writes
    static int write_count = 0;
    if (++write_count % 100 == 0) {
        file_.flush();
    }
}
```

---

## 🎯 Integração na LiveScreen

### **Modificação: LiveScreen.h**

```cpp
// Adicionar após includes
#include "core/RecordingManager.h"

class LiveScreen : public BaseScreen {
public:
    // ... existing methods

private:
    void RenderRecordingControls();  // NOVO
    void UpdateRecording();           // NOVO

    // State
    bool show_recording_ui_ = false;  // NOVO
    std::string recording_filename_;  // NOVO
};
```

### **Modificação: LiveScreen.cpp**

```cpp
// No RenderTopControls() - adicionar botão de gravação

void LiveScreen::RenderTopControls() {
    ImGui::SetCursorPosY(20);
    ImGui::SetCursorPosX(20);

    // Recording control button
    ImGui::PushStyleColor(ImGuiCol_Button,
        RecordingManager::Instance().IsRecording()
        ? ImVec4(1.0f, 0.2f, 0.2f, 0.7f)  // Red when recording
        : ImVec4(0.2f, 0.2f, 0.2f, 0.7f)  // Gray normally
    );

    const char *record_label = RecordingManager::Instance().IsRecording()
        ? "■ Stop Recording"
        : "● Start Recording";

    if (ImGui::Button(record_label, ImVec2(150, 40))) {
        if (RecordingManager::Instance().IsRecording()) {
            RecordingManager::Instance().StopRecording();
        } else {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            char buffer[100];
            strftime(buffer, sizeof(buffer),
                    "session_%Y%m%d_%H%M%S",
                    std::localtime(&time_t));
            RecordingManager::Instance().StartRecording(buffer);
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine(200);
    ImGui::Text("Recording: %llu samples | %.1f MB",
                RecordingManager::Instance().GetSampleCount(),
                RecordingManager::Instance().GetBytesWritten() / 1024.0f / 1024.0f);
}

// Na function Update()
void LiveScreen::Update(float delta_time) {
    // ... existing code

    // Verificar samples do backend
    auto &backend = ECUBackend::Instance();
    std::vector<SensorState> samples;

    if (backend.DrainSamples(samples)) {
        // Atualizar UI
        for (const auto &sample : samples) {
            UpdateSensorUI(sample);
        }

        // NOVO: Gravar se ativo
        if (RecordingManager::Instance().IsRecording()) {
            for (const auto &sample : samples) {
                SensorSnapshot snapshot{
                    .timestamp_ms =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
                        ).count(),
                    .value = sample.value,
                    .unit = sample.unit,
                    .sensor_id = sample.id,
                    .sensor_name = sample.name
                };
                RecordingManager::Instance().LogSensorSnapshot(snapshot);
            }
        }
    }
}
```

---

## 📊 Análise Pós-Sessão

### **GraphScreen Enhancement**

```cpp
// scanner_glfw/screens/GraphScreen.h

#include <map>
#include <vector>

struct SessionDataPoint {
    uint64_t timestamp_ms;
    std::map<int, float> sensor_values;  // sensor_id -> value
};

class GraphScreen : public BaseScreen {
public:
    void LoadSessionFile(const std::string &filename);

private:
    void RenderHistoricalGraph();
    void RenderSessionStats();
    void RenderSessionComparison();

    std::vector<SessionDataPoint> session_data_;
    std::string current_session_filename_;

    // Stats
    struct SensorStats {
        float min_value;
        float max_value;
        float avg_value;
        float std_deviation;
    };

    std::map<int, SensorStats> session_stats_;
};

// Implementação
void GraphScreen::LoadSessionFile(const std::string &filename) {
    std::ifstream file(filename);
    std::string line;

    // Skip header
    std::getline(file, line);

    // Parse CSV
    while (std::getline(file, line)) {
        // timestamp_ms,sensor_id,sensor_name,value,unit
        uint64_t ts;
        int sensor_id;
        float value;
        char comma;

        std::istringstream iss(line);
        iss >> ts >> comma >> sensor_id >> comma;

        SessionDataPoint point;
        point.timestamp_ms = ts;

        // Encontrar ou criar ponto
        auto it = std::find_if(session_data_.begin(), session_data_.end(),
            [ts](const SessionDataPoint &p) {
                return p.timestamp_ms == ts;
            });

        if (it != session_data_.end()) {
            it->sensor_values[sensor_id] = value;
        } else {
            point.sensor_values[sensor_id] = value;
            session_data_.push_back(point);
        }
    }

    CalculateStats();
}

void GraphScreen::RenderSessionStats() {
    ImGui::BeginChild("SessionStats", ImVec2(300, 200), true);

    for (const auto &[sensor_id, stats] : session_stats_) {
        ImGui::Text("Sensor %d:", sensor_id);
        ImGui::Text("  Min: %.2f", stats.min_value);
        ImGui::Text("  Max: %.2f", stats.max_value);
        ImGui::Text("  Avg: %.2f", stats.avg_value);
        ImGui::Text("  Std: %.2f", stats.std_deviation);
        ImGui::Separator();
    }

    ImGui::EndChild();
}
```

---

## 🔧 Setup Inicial

### **CMakeLists.txt - Adicionar RecordingManager**

```cmake
# Adicionar fonte
list(APPEND SOURCES
    src/core/RecordingManager.cpp
    src/core/DataLogFile.cpp
)

# Adicionar thread library (para background writing)
find_package(Threads REQUIRED)
target_link_libraries(scanner_glfw PRIVATE Threads::Threads)
```

### **app_data.h - Adicionar Globals**

```cpp
// Adicionar em app_data.h
extern RecordingConfig g_recording_config;
extern std::string g_last_session_file;

// Inicializar em app_data.cpp
RecordingConfig g_recording_config;
std::string g_last_session_file;
```

---

## 📝 Checklist de Implementação

### **Fase 1: Core (2 dias)**

- [ ] RecordingManager class
  - [ ] Start/StopRecording
  - [ ] Background write thread
  - [ ] Thread-safe queue buffer

- [ ] DataLogFile class
  - [ ] CSV writer
  - [ ] JSON writer
  - [ ] Header generation

- [ ] Integration em LiveScreen
  - [ ] Buttons na UI
  - [ ] Sample capture
  - [ ] Recording state display

- [ ] Tests
  - [ ] File creation
  - [ ] CSV format validation
  - [ ] Thread safety

### **Fase 2: Analysis (1 dia)**

- [ ] GraphScreen enhancement
  - [ ] Load session file
  - [ ] Calculate statistics
  - [ ] Render historical graphs

- [ ] Export formats
  - [ ] JSON export
  - [ ] Multi-session comparison

- [ ] Tests
  - [ ] Data integrity
  - [ ] Graph rendering

### **Fase 3: Polish (1 dia)**

- [ ] NASA P10 compliance check
- [ ] Static analysis (clang-tidy)
- [ ] Code review
- [ ] Documentation

---

## 🚨 Considerações de Segurança (NASA P10)

### **Bounded Loops**
```cpp
// ❌ BAD: Unbounded
while (true) {
    buffer_.push(snapshot);
}

// ✅ GOOD: Bounded with max size
const size_t MAX_BUFFER_SIZE = 10000;  // NASA P10
void LogSensorSnapshot(const SensorSnapshot &snapshot) {
    if (buffer_.size() < MAX_BUFFER_SIZE) {
        buffer_.push(snapshot);
    } else {
        // Handle full buffer
        fprintf(stderr, "WARNING: Recording buffer full\n");
        FlushBuffer();
    }
}
```

### **Assertions**
```cpp
// Adicionar em cada método
void LogSensorSnapshot(const SensorSnapshot &snapshot) {
    ASSERT(recording_, "Recording not active");
    ASSERT(snapshot.value >= -1e6f, "Sensor value suspiciously low");
    ASSERT(snapshot.value <= 1e6f, "Sensor value suspiciously high");
    ASSERT(!snapshot.sensor_name.empty(), "Sensor name empty");
}
```

### **Error Handling**
```cpp
bool RecordingManager::StartRecording(const std::string &session_name) {
    ASSERT(!recording_, "Already recording");

    std::string filename = BuildFilename(session_name);

    // RAII: Use unique_ptr
    auto file = std::make_unique<DataLogFile>(filename);
    if (!file->OpenCSV()) {
        fprintf(stderr, "ERROR: Cannot open recording file\n");
        return false;
    }

    output_file_ = std::move(file);
    recording_ = true;
    start_time_ = std::chrono::system_clock::now();

    return true;
}
```

---

## 📚 Exemplo Completo: Sessão Gravada

### **Arquivo CSV Gerado**

```csv
timestamp_ms,sensor_id,sensor_name,value,unit
0,0,RPM,800.0,rpm
10,0,RPM,850.5,rpm
20,1,MAF,2.3,g/s
30,0,RPM,900.0,rpm
40,1,MAF,2.5,g/s
50,2,IAT,25.0,C
60,0,RPM,950.0,rpm
...
```

### **Análise Pós-Sessão**

```
Session: session_20260114_143052
Duration: 123.45 seconds
Samples recorded: 12,345

Sensor Statistics:
  RPM:
    Min: 800 rpm
    Max: 4200 rpm
    Avg: 2100 rpm
    Std Dev: 850 rpm

  MAF:
    Min: 1.2 g/s
    Max: 15.5 g/s
    Avg: 5.3 g/s
    Std Dev: 3.2 g/s
```

---

## 🔍 Testing Strategy

```cpp
// test/RecordingManagerTest.cpp

#include <gtest/gtest.h>
#include "core/RecordingManager.h"

class RecordingManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = &RecordingManager::Instance();
    }

    RecordingManager *manager_;
};

TEST_F(RecordingManagerTest, StartStop) {
    ASSERT_FALSE(manager_->IsRecording());

    ASSERT_TRUE(manager_->StartRecording("test_session"));
    ASSERT_TRUE(manager_->IsRecording());

    manager_->StopRecording();
    ASSERT_FALSE(manager_->IsRecording());
}

TEST_F(RecordingManagerTest, LogSnapshot) {
    manager_->StartRecording("test_session");

    SensorSnapshot snapshot{
        .timestamp_ms = 1000,
        .value = 2500.0f,
        .unit = "rpm",
        .sensor_id = 0,
        .sensor_name = "RPM"
    };

    manager_->LogSensorSnapshot(snapshot);
    ASSERT_EQ(manager_->GetSampleCount(), 1);

    manager_->StopRecording();
}

TEST_F(RecordingManagerTest, CSVFormat) {
    manager_->StartRecording("test_session");

    // Log samples
    for (int i = 0; i < 10; i++) {
        SensorSnapshot snapshot{
            .timestamp_ms = (uint64_t)(i * 100),
            .value = 1000.0f + i * 100,
            .unit = "rpm",
            .sensor_id = 0,
            .sensor_name = "RPM"
        };
        manager_->LogSensorSnapshot(snapshot);
    }

    manager_->StopRecording();
    std::string filename = manager_->GetCurrentFilename();

    // Validate CSV
    std::ifstream file(filename);
    std::string line;

    // Read header
    std::getline(file, line);
    ASSERT_EQ(line, "timestamp_ms,sensor_id,sensor_name,value,unit");

    // Read first row
    std::getline(file, line);
    ASSERT_TRUE(line.find("0,0,RPM,1000.0,rpm") != std::string::npos);
}
```
