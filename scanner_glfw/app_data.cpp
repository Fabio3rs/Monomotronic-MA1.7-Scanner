
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "SensorDecoders.h"
#include "app_data.h"
#include "core/ECUBackend.h"
#include "imgui.h"   // For IM_COL32, ImVec2, etc.
#include <algorithm> // For std::clamp, std::max
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Configuração de debouncing
constexpr double DEFAULT_ALERT_TRIGGER_DELAY = 4.0; // segundos
constexpr double DEFAULT_ALERT_CLEAR_DELAY = 4.0;   // segundos

// --- Variáveis de estado da aplicação ---
Screen currentScreen = Screen::DASH;
std::vector<int> pinnedSensorIndices;
std::vector<int> customSensorList;  // User favorite sensors for LIVE screen
std::vector<int> defaultSensorList; // ECU default sensor set
std::vector<DashboardWidget> dashboardWidgets; // Dashboard widget configuration
int dashboardColumns = 2;                      // Default: 2 columns
std::vector<int> graphSensorIndices;
float graphTimeWindowSecs = 30.0f;
bool graphFrozen = false;
bool loggingActive = false;
int selectedSensorForDetail = -1;
std::vector<bool> graphSignalAutoScaleY;
bool graphCursorActive = false;
ImVec2 graphCursorPos = ImVec2(0, 0);
bool graphCursorBActive = false;       // Second cursor for delta measurement
ImVec2 graphCursorBPos = ImVec2(0, 0); // Position of cursor B
float graphLastPlotMinY[4] = {0.0f};
float graphLastPlotMaxY[4] = {0.0f};

// Connection status variables
float kLineLatency = 16.0f;  // milliseconds
float kLineErrorRate = 0.0f; // 0.0 - 1.0
bool ecuConnected = true;
int kLineTableActive = 0;
bool simulationModeActive = false;
std::optional<ECUInfo> g_ecu_info;
std::function<void(const ECUInfo &)> on_ecu_info_received;
std::function<void(const std::string &)> on_connection_error;
uint64_t dataSampleSequence = 0;
double dataSampleTimestampSec = 0.0;
double dataSampleDeltaSec = 0.0;

// DTC pagination
int dtcPageIndex = 0;
const int dtcPerPage = 10;

// --- Fontes (definidas aqui, mas carregadas em main.cpp) ---
ImFont *font_large = nullptr;
ImFont *font_huge = nullptr;

// --- Cores ---
const ImVec4 GraphColors[] = {
    {0.9f, 0.2f, 0.2f, 1.0f}, // Red
    {0.2f, 0.9f, 0.2f, 1.0f}, // Green
    {0.2f, 0.2f, 0.9f, 1.0f}, // Blue
    {0.9f, 0.9f, 0.2f, 1.0f}  // Yellow
};

// --- Estruturas de dados simuladas ---
std::vector<SimulatedDTC> activeDTCs;
std::vector<SensorState> simulatedSensors;
std::map<std::string, std::vector<int>> sensorCategories;

// Implementação do construtor SensorState
SensorState::SensorState(std::string_view n, int i, int s,
                         std::function<double(int)> d, std::string_view u,
                         std::string_view desc, double dmin, double dmax,
                         double amin, double amax, int len)
    : name(n), id(i), subcmd(s), dataLength(len), lastRaw(0.0), lastValue(0.0),
      maxHistory(300), decoder(d), unit(u), description(desc), displayMin(dmin),
      displayMax(dmax), alertMin(amin), alertMax(amax),
      status(SensorStatus::OK), lastUpdateTime(0.0),
      // Inicializar campos de debouncing
      pendingStatus(SensorStatus::OK), statusTransitionStartTime(0.0),
      alertTriggerDelay(DEFAULT_ALERT_TRIGGER_DELAY),
      alertClearDelay(DEFAULT_ALERT_CLEAR_DELAY), inStatusTransition(false) {
    history.reserve(maxHistory);
}

namespace {
struct KlineEntry {
    uint8_t id;
    uint8_t subcmd;
};

constexpr std::array<KlineEntry, 10> kKlineTable1{{
    {0x63, 0x00}, // Temp. água
    {0x66, 0x00}, // TPS trilha 1
    {0x47, 0x00}, // RPM motor
    {0x62, 0x00}, // Temp. ar
    {0x0C, 0xF8}, // Integrador lambda
    {0x6C, 0xF8}, // Adaptação MAP
    {0x6F, 0xF8}, // Coef. MLLECK
    {0x3D, 0xF8}, // Atuador marcha lenta
    {0x61, 0x00}, // Bateria (bruto)
    {0x69, 0xF8}, // Adaptação FTEAD
}};

constexpr std::array<KlineEntry, 10> kKlineTable2{{
    {0x63, 0x00}, // Temp. água
    {0x66, 0x00}, // TPS trilha 1
    {0x70, 0x00}, // Ângulo borboleta
    {0x2F, 0x00}, // Flags combustível
    {0x72, 0xF8}, // Coef. TS
    {0x6C, 0xF8}, // Adaptação MAP
    {0x6F, 0xF8}, // Coef. MLLECK
    {0x40, 0xF8}, // Parâmetro adapt. 0 (XRAM_0x40)
    {0x61, 0x00}, // Bateria (bruto)
    {0x69, 0xF8}, // Adaptação FTEAD
}};

std::vector<KlineEntry> BuildDefaultKlineSet() {
    std::vector<KlineEntry> merged;
    merged.reserve(kKlineTable1.size() + kKlineTable2.size());
    std::set<std::pair<uint8_t, uint8_t>> seen;

    auto addTable = [&](const auto &table) {
        for (const auto &entry : table) {
            auto key = std::make_pair(entry.id, entry.subcmd);
            if (seen.insert(key).second) {
                merged.push_back(entry);
            }
        }
    };

    addTable(kKlineTable1);
    addTable(kKlineTable2);
    return merged;
}

void ApplyColorZones(const SensorDecoderEntry &dec, SensorState &sensor) {
    if (dec.id == 0x47 && dec.subcommand == 0x00) { // Engine RPM
        sensor.colorZones = {
            Colors::ColorZone(0, ImVec4(0.95f, 0.25f, 0.25f, 1.0f)),
            Colors::ColorZone(800, ImVec4(0.3f, 1.0f, 0.3f, 1.0f)),
            Colors::ColorZone(5750, ImVec4(0.96f, 0.62f, 0.04f, 1.0f)),
            Colors::ColorZone(6500, ImVec4(0.95f, 0.25f, 0.25f, 1.0f))};
    } else if (dec.id == 0x63 && dec.subcommand == 0x00) { // Coolant temp
        sensor.colorZones = {
            Colors::ColorZone(-40, ImVec4(0.3f, 0.5f, 1.0f, 1.0f)),
            Colors::ColorZone(70, ImVec4(0.3f, 1.0f, 0.3f, 1.0f)),
            Colors::ColorZone(95, ImVec4(0.96f, 0.62f, 0.04f, 1.0f)),
            Colors::ColorZone(110, ImVec4(0.95f, 0.25f, 0.25f, 1.0f))};
    }
}

void Categorize(const SensorDecoderEntry &dec, int index) {
    const std::string_view key(dec.key);
    const std::string_view display(dec.display_name);
    auto addTo = [&](const char *category) {
        sensorCategories[category].push_back(index);
    };
    auto hasToken = [](std::string_view text, std::string_view token) {
        return text.find(token) != std::string_view::npos;
    };

    if (dec.id == 0x46 || dec.id == 0x47 || dec.id == 0x60 || dec.id == 0x66 ||
        dec.id == 0x6F || dec.id == 0x70 || hasToken(key, "rpm") ||
        hasToken(key, "throttle") || hasToken(key, "tps")) {
        addTo("Engine");
    } else if (dec.id == 0x62 || dec.id == 0x63 || hasToken(key, "temp") ||
               hasToken(display, "Temp")) {
        addTo("Temps");
    } else if (dec.id == 0x7D || dec.id == 0x61 || hasToken(key, "battery")) {
        addTo("Battery");
    } else if (hasToken(key, "lambda") || hasToken(key, "fuel") ||
               hasToken(key, "map") || hasToken(key, "evap")) {
        addTo("Lambda/Fuel");
    } else {
        addTo("Diagnostics");
    }
}
} // namespace

// Implementação de pushSample
void SensorState::pushSample(int raw, double current_time) {
    lastUpdateTime = current_time;
    lastRaw = raw;
    if (decoder)
        lastValue = decoder(raw);
    else
        lastValue = raw;

    history.push_back(static_cast<float>(lastValue));
    if (history.size() > maxHistory)
        history.erase(history.begin());

    // Determinar status imediato baseado no valor atual
    SensorStatus immediateStatus;
    if (!std::isnan(alertMin) && lastValue < alertMin)
        immediateStatus = SensorStatus::WARN;
    else if (!std::isnan(alertMax) && lastValue > alertMax)
        immediateStatus = SensorStatus::CRITICAL;
    else
        immediateStatus = SensorStatus::OK;

    // Lógica de debouncing com hysteresis completo
    if (immediateStatus == status) {
        // Valor corresponde ao status atual - resetar transição
        inStatusTransition = false;
        pendingStatus = status;
    } else if (immediateStatus == pendingStatus) {
        // Valor ainda indica o mesmo status pendente
        if (inStatusTransition) {
            double elapsed = current_time - statusTransitionStartTime;

            // Determinar qual delay usar baseado na direção da transição
            double requiredDelay;
            if (immediateStatus == SensorStatus::OK) {
                // Transição PARA normal (limpar alerta) - usar clear delay
                requiredDelay = alertClearDelay;
            } else {
                // Transição PARA alerta (WARN/CRITICAL) - usar trigger delay
                requiredDelay = alertTriggerDelay;
            }

            // Aplicar mudança se tempo decorrido >= delay necessário
            if (elapsed >= requiredDelay) {
                status = pendingStatus;
                inStatusTransition = false;
            }
        }
    } else {
        // Valor indica status diferente tanto do atual quanto do pendente
        // Iniciar novo período de transição
        pendingStatus = immediateStatus;
        statusTransitionStartTime = current_time;
        inStatusTransition = true;
    }
}

// Funções de manipulação de dados (implementações)
void initSimulatedSensors() {
    simulatedSensors.clear();
    sensorCategories.clear();
    auto klineSensors = BuildDefaultKlineSet();

    int index = 0;
    defaultSensorList.clear();
    for (const auto &entry : klineSensors) {
        const auto *dec = FindSensorDecoder(entry.subcmd, entry.id);
        if (!dec) {
            continue;
        }

        const std::string_view displayName =
            dec->display_name.empty() ? dec->key : dec->display_name;
        const std::string_view unit = dec->unit;
        const std::string_view description = dec->description;

        SensorState sensor(displayName, dec->id, dec->subcommand, dec->decode,
                           unit, description, dec->display_min,
                           dec->display_max, dec->alert_min, dec->alert_max,
                           dec->length);

        ApplyColorZones(*dec, sensor);
        simulatedSensors.emplace_back(sensor);
        Categorize(*dec, index);
        defaultSensorList.push_back(index);
        ++index;
    }

    // Inicializar custom sensor list com defaults (ECU)
    customSensorList = defaultSensorList;

    // Inicializar dashboard widgets com sensores fixados
    dashboardWidgets.clear();
    for (int idx : pinnedSensorIndices) {
        dashboardWidgets.push_back(DashboardWidget(idx, WidgetType::NUMERIC));
    }
}

void generateSimulatedSamples() {
    double t = glfwGetTime();
    for (size_t i = 0; i < simulatedSensors.size(); ++i) {
        int raw = 0;
        const auto &s = simulatedSensors[i];
        switch (s.id) {
        case 0x47: { // RPM
            double v = 35.0 + 30.0 * pow(sin(t * 0.5), 3);
            raw = static_cast<int>(std::max(0.0, floor(v)));
            break;
        }
        case 0x66: { // TPS track 1 (%)
            // Track 1 covers roughly the first 24° of travel
            double target_deg = 12.0 + 8.0 * sin(t * 1.5 + 0.1);
            double frac = std::clamp(target_deg / 24.0, 0.0, 1.0);
            raw = static_cast<int>(std::clamp(frac * 255.0, 0.0, 255.0));
            break;
        }
        case 0x60: { // TPS track 2 (deg)
            // Track 2 starts near 18° and only becomes useful past that point
            constexpr double tps2_min_deg = 18.0;
            constexpr double tps_max_deg = 90.0;
            constexpr double tps2_start_counts =
                (tps2_min_deg / tps_max_deg) * 255.0;
            constexpr double tps2_counts_range = 255.0 - tps2_start_counts;
            constexpr double tps2_deg_per_count =
                (tps_max_deg - tps2_min_deg) / tps2_counts_range;
            constexpr double tps2_counts_per_deg = 1.0 / tps2_deg_per_count;

            double target_deg = 50.0 + 25.0 * sin(t * 1.5 - 0.1);
            double clamped_deg =
                std::clamp(target_deg, tps2_min_deg, tps_max_deg);
            double raw_double =
                (clamped_deg <= tps2_min_deg)
                    ? tps2_start_counts
                    : tps2_start_counts +
                          (clamped_deg - tps2_min_deg) * tps2_counts_per_deg;
            raw = static_cast<int>(std::clamp(raw_double, 0.0, 255.0));
            break;
        }
        case 0x63:   // Coolant temp
        case 0x62: { // Air temp
            double base_temp = (s.id == 0x63) ? 85.0 : 45.0;
            double v = base_temp + 5.0 * sin(t * 0.3 + static_cast<double>(i));
            raw = static_cast<int>(std::clamp((v - 40.0) + 120.0, 0.0, 255.0));
            break;
        }
        case 0x65: { // Lambda voltage (if present)
            double phase = fmod(t * 2.0 + static_cast<double>(i) * 0.05, 1.0);
            double mv = (phase < 0.5) ? (150.0 + 50.0 * sin(t * 5.0))
                                      : (800.0 - 50.0 * sin(t * 5.0));
            raw = static_cast<int>(std::clamp(mv / 3.87, 0.0, 255.0));
            break;
        }
        case 0x7D: { // Battery voltage
            raw = static_cast<int>(
                std::clamp(13.8 / 0.0637 + sin(t) * 0.1 / 0.0637, 0.0, 255.0));
            break;
        }
        case 0x61: { // Battery raw staging
            raw = static_cast<int>(
                std::clamp(13.8 / 0.0637 + sin(t) * 0.1 / 0.0637, 0.0, 255.0));
            break;
        }
        case 0x14: { // Injection time
            double rpm_val =
                simulatedSensors.empty() ? 0.0 : simulatedSensors[0].lastValue;
            double base_inj = 2.0 + (rpm_val / 8000.0) * 10.0;
            double v = base_inj + 0.5 * sin(t * 20.0);
            raw = static_cast<int>(std::clamp(v * 1000.0 / 4.0, 0.0, 65535.0));
            break;
        }
        default: {
            raw = static_cast<int>(128 + 50 * sin(t + static_cast<double>(i)));
            break;
        }
        }
        simulatedSensors[i].pushSample(raw, t);
    }
}

namespace {
std::string DetectECUPort() {
    const char *env = std::getenv("ECU_PORT");
    if (env && *env) {
        return std::string(env);
    }
    return "/dev/ttyUSB0";
}
} // namespace

void UpdateLiveData() {
    auto &backend = ECUBackend::Instance();
    static bool backend_started = false;
    static double last_data_timestamp = 0.0;
    static bool has_last_timestamp = false;
    static bool last_timestamp_from_backend = false;
    bool new_data = false;
    double sample_timestamp = 0.0;
    bool timestamp_from_backend = false;

    if (!backend_started) {
        backend_started = backend.Start(DetectECUPort(), &simulatedSensors);
    }

    if (backend_started) {
        ecuConnected = backend.IsConnected();
        kLineLatency = backend.GetLatencyMs();
        kLineErrorRate = backend.GetErrorRate();
        kLineTableActive = backend.GetLastTableId();

        const bool had_samples = backend.DrainSamples(simulatedSensors);
        if (had_samples) {
            new_data = true;
            sample_timestamp = backend.GetLastSampleTimestampSec();
            timestamp_from_backend = true;
        }
        // If we didn't receive new samples, keep previous values instead of
        // overwriting with mocks. This avoids hiding real data during slow
        // polls.
        if (!had_samples && !ecuConnected) {
            generateSimulatedSamples();
            new_data = true;
            sample_timestamp = glfwGetTime();
            timestamp_from_backend = false;
        }
    } else {
        // Fallback to simulated data if backend failed to start
        generateSimulatedSamples();
        ecuConnected = false;
        new_data = true;
        sample_timestamp = glfwGetTime();
        timestamp_from_backend = false;
    }

    if (new_data) {
        if (timestamp_from_backend && sample_timestamp <= 0.0) {
            sample_timestamp = glfwGetTime();
            timestamp_from_backend = false;
        }

        dataSampleTimestampSec = sample_timestamp;
        if (!has_last_timestamp ||
            timestamp_from_backend != last_timestamp_from_backend) {
            dataSampleDeltaSec = 0.0;
        } else {
            dataSampleDeltaSec = sample_timestamp - last_data_timestamp;
        }

        last_data_timestamp = sample_timestamp;
        has_last_timestamp = true;
        last_timestamp_from_backend = timestamp_from_backend;
        ++dataSampleSequence;
    }
}

// Accessors
void SetCurrentScreen(Screen s) { currentScreen = s; }
Screen GetCurrentScreen() { return currentScreen; }
