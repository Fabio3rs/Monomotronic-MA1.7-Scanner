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

// ============================================================================
// Structured application contexts (C++20 inline for zero-overhead linkage)
// ============================================================================
namespace AppContext {

struct UIState {
    Screen currentScreen = Screen::DASH;
    std::vector<int> pinnedSensorIndices;
    std::vector<int> customSensorList;  // User favorite sensors for LIVE screen
    std::vector<int> defaultSensorList; // ECU default sensor set
    std::vector<DashboardWidget> dashboardWidgets;
    int dashboardColumns = 2;
    std::vector<int> graphSensorIndices;
    float graphTimeWindowSecs = 30.0f;
    bool graphFrozen = false;
    bool loggingActive = false;
    int selectedSensorForDetail = -1;
    std::vector<bool> graphSignalAutoScaleY;
    bool graphCursorActive = false;
    ImVec2 graphCursorPos = ImVec2(0, 0);
    bool graphCursorBActive = false;
    ImVec2 graphCursorBPos = ImVec2(0, 0);
    float graphLastPlotMinY[4] = {0.0f};
    float graphLastPlotMaxY[4] = {0.0f};
};
inline UIState ui;

struct ECUState {
    float kLineLatency = 16.0f;  // milliseconds
    float kLineErrorRate = 0.0f; // 0.0 - 1.0
    bool ecuConnected = true;
    int kLineTableActive = 0; // 0 = none, 1 = table1 (6E78), 2 = table2 (6E8C)
    bool simulationModeActive = false;
    uint64_t dataSampleSequence = 0;
    double dataSampleTimestampSec = 0.0;
    double dataSampleDeltaSec = 0.0;
};
inline ECUState ecu;

} // namespace AppContext

// ============================================================================
// Legacy global aliases (preserved for backward compatibility)
// ============================================================================
inline Screen &currentScreen = AppContext::ui.currentScreen;
inline std::vector<int> &pinnedSensorIndices =
    AppContext::ui.pinnedSensorIndices;
inline std::vector<int> &customSensorList = AppContext::ui.customSensorList;
inline std::vector<int> &defaultSensorList = AppContext::ui.defaultSensorList;
inline std::vector<DashboardWidget> &dashboardWidgets =
    AppContext::ui.dashboardWidgets;
inline int &dashboardColumns = AppContext::ui.dashboardColumns;
inline std::vector<int> &graphSensorIndices = AppContext::ui.graphSensorIndices;
inline float &graphTimeWindowSecs = AppContext::ui.graphTimeWindowSecs;
inline bool &graphFrozen = AppContext::ui.graphFrozen;
inline bool &loggingActive = AppContext::ui.loggingActive;
inline int &selectedSensorForDetail = AppContext::ui.selectedSensorForDetail;
inline std::vector<bool> &graphSignalAutoScaleY =
    AppContext::ui.graphSignalAutoScaleY;
inline bool &graphCursorActive = AppContext::ui.graphCursorActive;
inline ImVec2 &graphCursorPos = AppContext::ui.graphCursorPos;
inline bool &graphCursorBActive = AppContext::ui.graphCursorBActive;
inline ImVec2 &graphCursorBPos = AppContext::ui.graphCursorBPos;
inline float *graphLastPlotMinY = AppContext::ui.graphLastPlotMinY;
inline float *graphLastPlotMaxY = AppContext::ui.graphLastPlotMaxY;

inline float &kLineLatency = AppContext::ecu.kLineLatency;
inline float &kLineErrorRate = AppContext::ecu.kLineErrorRate;
inline bool &ecuConnected = AppContext::ecu.ecuConnected;
inline int &kLineTableActive = AppContext::ecu.kLineTableActive;
inline bool &simulationModeActive = AppContext::ecu.simulationModeActive;
inline uint64_t &dataSampleSequence = AppContext::ecu.dataSampleSequence;
inline double &dataSampleTimestampSec = AppContext::ecu.dataSampleTimestampSec;
inline double &dataSampleDeltaSec = AppContext::ecu.dataSampleDeltaSec;

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
                int len = 1, SensorPollMode mode = SensorPollMode::COLLECTION);
    void pushSample(int raw, double current_time);
    void SetMaxHistory(size_t new_max);
};
extern std::vector<SensorState> simulatedSensors;
extern std::map<std::string, std::vector<int>> sensorCategories;

// Funções de manipulação de dados
void initSimulatedSensors();
void generateSimulatedSamples();
void UpdateLiveData();
void RenderECULoadingOverlay();

// Accessors for current screen to avoid accidental duplicate symbol usage
void SetCurrentScreen(Screen s);
Screen GetCurrentScreen();
