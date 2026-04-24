#pragma once

#include "imgui.h"        // For ImVec4, ImFont
#include "utils/Colors.h" // For ColorZone
#include <cmath>          // For NAN, isnan, etc.
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Enumeração das telas da aplicação
enum class Screen { LIVE, GRAPH, DASH, DTC, LOGS };

// Tipos de widgets para Dashboard
enum class WidgetType { NUMERIC, GAUGE, GRAPH, BARGRAPH };

struct DashboardWidget {
    int sensor_idx;
    WidgetType type;

    DashboardWidget(int idx = 0, WidgetType t = WidgetType::NUMERIC)
        : sensor_idx(idx), type(t) {}
};

// Estado da aplicação e dados simulados
extern Screen currentScreen;
extern std::vector<int> pinnedSensorIndices;
extern std::vector<int>
    customSensorList; // User favorite sensors for LIVE screen
extern std::vector<int> defaultSensorList; // ECU default sensor set
extern std::vector<DashboardWidget>
    dashboardWidgets;        // Dashboard widget configuration
extern int dashboardColumns; // 1, 2, or 3 columns
extern std::vector<int> graphSensorIndices;
extern float graphTimeWindowSecs;
extern bool graphFrozen;
extern bool loggingActive;
extern int selectedSensorForDetail;
extern std::vector<bool> graphSignalAutoScaleY;
extern bool graphCursorActive;
extern ImVec2 graphCursorPos;
extern bool graphCursorBActive; // Second cursor for delta measurement
extern ImVec2 graphCursorBPos;  // Position of cursor B
extern float graphLastPlotMinY[4];
extern float graphLastPlotMaxY[4];

// Connection status variables
extern float kLineLatency;   // milliseconds
extern float kLineErrorRate; // 0.0 - 1.0
extern bool ecuConnected;
extern int kLineTableActive; // 0 = none, 1 = table1 (6E78), 2 = table2 (6E8C)
extern bool simulationModeActive; // Explicit simulation flag
extern uint64_t dataSampleSequence;
extern double dataSampleTimestampSec;
extern double dataSampleDeltaSec;

// ECU information parsed from init packets
struct ECUInfo {
    std::string model;
    std::string firmware_version;
    std::string hardware_id;
    std::uint32_t table_id = 0;

    bool IsValid() const noexcept { return !model.empty() && table_id > 0; }

    std::string ToString() const noexcept {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s v%s (table %u)", model.c_str(),
                      firmware_version.c_str(), table_id);
        return std::string(buf);
    }
};

extern std::optional<ECUInfo> g_ecu_info;
extern std::function<void(const ECUInfo &)> on_ecu_info_received;
extern std::function<void(const std::string &)> on_connection_error;

// DTC pagination
extern int dtcPageIndex;
extern const int dtcPerPage;

// Fontes
extern ImFont *font_large;
extern ImFont *font_huge;

// Cores
extern const ImVec4 GraphColors[];

// --- Estruturas de dados simuladas ---
enum class SensorStatus { OK, WARN, CRITICAL, STALE };
enum class SensorPollMode { COLLECTION, INDIVIDUAL };

struct SimulatedDTC {
    std::string code;
    std::string description;
    bool active;
};
extern std::vector<SimulatedDTC> activeDTCs;

struct SensorState {
    std::string name;
    int id;
    int subcmd;
    int dataLength;
    double lastRaw;
    double lastValue;
    std::vector<float> history;
    size_t maxHistory;
    std::function<double(int)> decoder;
    std::string unit;
    std::string description;
    double displayMin;
    double displayMax;
    double alertMin;
    double alertMax;
    SensorStatus status;
    double lastUpdateTime;
    SensorPollMode pollMode;

    // Color zones for visual feedback (ordered by threshold)
    std::vector<Colors::ColorZone> colorZones;

    // Debouncing state tracking
    SensorStatus pendingStatus; // Status pendente (baseado no valor atual)
    double statusTransitionStartTime; // Timestamp do início da transição
    double alertTriggerDelay; // Segundos para acionar alerta (padrão: 4.0)
    double alertClearDelay;   // Segundos para limpar alerta (padrão: 4.0)
    bool inStatusTransition;  // Flag: em período de transição?

    SensorState(std::string_view n = "", int i = 0, int s = 0,
                std::function<double(int)> d = nullptr, std::string_view u = "",
                std::string_view desc = "", double dmin = NAN,
                double dmax = NAN, double amin = NAN, double amax = NAN,
                int len = 1,
                SensorPollMode mode = SensorPollMode::COLLECTION);
    void pushSample(int raw, double current_time);
};
extern std::vector<SensorState> simulatedSensors;
extern std::map<std::string, std::vector<int>> sensorCategories;

// Funções de manipulação de dados
void initSimulatedSensors();
void generateSimulatedSamples();
void UpdateLiveData();

// Accessors for current screen to avoid accidental duplicate symbol usage
void SetCurrentScreen(Screen s);
Screen GetCurrentScreen();
