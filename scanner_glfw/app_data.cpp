
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "KlineCollectionTables.h"
#include "SensorDecoders.h"
#include "app_data.h"
#include "core/AnimationSystem.h"
#include "core/ECUBackend.h"
#include "core/ThemeManager.h"
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
// (Migrated to AppContext namespace in app_data.h as inline variables)

std::optional<ECUInfo> g_ecu_info;
std::function<void(const ECUInfo &)> on_ecu_info_received;
std::function<void(const std::string &)> on_connection_error;

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
                         double amin, double amax, int len, SensorPollMode mode)
    : name(n), id(i), subcmd(s), dataLength(len), lastRaw(0.0), lastValue(0.0),
      maxHistory(300), decoder(d), unit(u), description(desc), displayMin(dmin),
      displayMax(dmax), alertMin(amin), alertMax(amax),
      status(SensorStatus::OK), lastUpdateTime(0.0), pollMode(mode),
      // Inicializar campos de debouncing
      pendingStatus(SensorStatus::OK), statusTransitionStartTime(0.0),
      alertTriggerDelay(DEFAULT_ALERT_TRIGGER_DELAY),
      alertClearDelay(DEFAULT_ALERT_CLEAR_DELAY), inStatusTransition(false) {
    history.reserve(maxHistory);
}

namespace {
// Local wrapper for KlineEntry to maintain compatibility with
// BuildDefaultKlineSet
struct KlineEntry {
    uint8_t id;
    uint8_t subcmd;
};

std::vector<KlineEntry> BuildDefaultKlineSet() {
    std::vector<KlineEntry> merged;
    std::set<std::pair<uint8_t, uint8_t>> seen;

    // Helper to add entries from a collection table
    auto addTable = [&](const auto &table) {
        for (const auto &[id, subcmd] : table) {
            const auto key = std::make_pair(id, subcmd);
            if (seen.insert(key).second) {
                merged.push_back(KlineEntry{id, subcmd});
            }
        }
    };

    // 1) Keep original collection defaults (from shared tables)
    addTable(ecu::kCollectionTable1);
    addTable(ecu::kCollectionTable2);

    // 2) Add any decoders not covered by the default tables
    for (const auto &dec : GetSensorDecoders()) {
        const auto key = std::make_pair(static_cast<uint8_t>(dec.id),
                                        static_cast<uint8_t>(dec.subcommand));
        if (seen.insert(key).second) {
            merged.push_back(KlineEntry{static_cast<uint8_t>(dec.id),
                                        static_cast<uint8_t>(dec.subcommand)});
        }
    }

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
    // Use explicit category from SensorDecoderEntry instead of heuristics
    const char *category_name = nullptr;
    switch (dec.category) {
    case ecu::SensorCategory::ENGINE:
        category_name = "Engine";
        break;
    case ecu::SensorCategory::TEMPERATURE:
        category_name = "Temps";
        break;
    case ecu::SensorCategory::ELECTRICAL:
        category_name = "Battery";
        break;
    case ecu::SensorCategory::FUEL_LAMBDA:
        category_name = "Lambda/Fuel";
        break;
    case ecu::SensorCategory::DIAGNOSTICS:
    case ecu::SensorCategory::UNKNOWN:
    default:
        category_name = "Diagnostics";
        break;
    }
    sensorCategories[category_name].push_back(index);
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

void SensorState::SetMaxHistory(size_t new_max) {
    maxHistory = new_max;
    if (history.size() > maxHistory) {
        history.erase(history.begin(),
                      history.begin() + (history.size() - maxHistory));
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

        // Use collection_capable flag from SensorDecoderEntry
        const SensorPollMode poll_mode = dec->collection_capable
                                             ? SensorPollMode::COLLECTION
                                             : SensorPollMode::INDIVIDUAL;

        SensorState sensor(displayName, dec->id, dec->subcommand, dec->decode,
                           unit, description, dec->display_min,
                           dec->display_max, dec->alert_min, dec->alert_max,
                           dec->length, poll_mode);

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
ECULinkConfig DetectECULinkConfig() {
    ECULinkConfig config =
        MakeKnownProfileConfig(ECUKnownProfile::FiatTipo16Ie);

    if (const char *profile_env = std::getenv("ECU_PROFILE");
        profile_env != nullptr && *profile_env != '\0') {
        if (const auto profile = ParseKnownProfile(profile_env);
            profile.has_value()) {
            config = MakeKnownProfileConfig(profile.value());
        }
    }

    if (const char *port_env = std::getenv("ECU_PORT");
        port_env != nullptr && *port_env != '\0') {
        config.port = port_env;
    }

    if (const char *baud_env = std::getenv("ECU_BAUD");
        baud_env != nullptr && *baud_env != '\0') {
        if (const auto baud = ParseECUBaudRate(baud_env); baud.has_value()) {
            config.session_baud = baud.value();
        } else if (on_connection_error) {
            on_connection_error(std::string("Unsupported ECU_BAUD: ") +
                                baud_env);
        }
    }

    return config;
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
        backend_started = backend.Start(DetectECULinkConfig(), &simulatedSensors);
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

void RenderECULoadingOverlay() {
    auto &anim = AnimationSystem::Instance();
    const float pulse = anim.GetPulse(1.5f);
    const float alpha = 0.7f + 0.3f * pulse;

    const ImVec2 vp_size = ImGui::GetIO().DisplaySize;
    const ImVec2 center(vp_size.x * 0.5f, vp_size.y * 0.5f);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Dark semi-transparent background
    draw_list->AddRectFilled(
        ImVec2(0, 0), vp_size,
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.6f)));

    // Spinner ring
    constexpr float kRadius = 32.0f;
    constexpr float kThickness = 4.0f;
    const float angle = static_cast<float>(ImGui::GetTime()) * 3.0f;
    const int num_segments = 24;
    const float arc_length = std::numbers::pi_v<float> * 0.75f;

    for (int i = 0; i < num_segments; ++i) {
        const float a0 = angle + (i / static_cast<float>(num_segments)) *
                                     std::numbers::pi_v<float> * 2.0f;
        const float a1 = angle + ((i + 1) / static_cast<float>(num_segments)) *
                                     std::numbers::pi_v<float> * 2.0f;
        const float fade =
            std::max(0.0f, std::cos(a0 - angle - arc_length * 0.5f));
        if (fade < 0.01f)
            continue;

        const ImU32 col = ImGui::ColorConvertFloat4ToU32(
            ImVec4(0.2f, 0.7f, 1.0f, fade * alpha));
        draw_list->AddLine(ImVec2(center.x + std::cos(a0) * kRadius,
                                  center.y + std::sin(a0) * kRadius),
                           ImVec2(center.x + std::cos(a1) * kRadius,
                                  center.y + std::sin(a1) * kRadius),
                           col, kThickness);
    }

    // Text below spinner
    auto &theme = ThemeManager::Instance();
    const char *text = "Connecting to ECU...";
    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 text_pos(center.x - text_size.x * 0.5f,
                          center.y + kRadius + 20.0f);
    draw_list->AddText(text_pos,
                       ImGui::ColorConvertFloat4ToU32(theme.GetPrimaryColor()),
                       text);

    // Subtext
    const char *sub = "Please ensure ignition is ON";
    const ImVec2 sub_size = ImGui::CalcTextSize(sub);
    const ImVec2 sub_pos(center.x - sub_size.x * 0.5f,
                         text_pos.y + text_size.y + 8.0f);
    draw_list->AddText(
        sub_pos, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sub);
}
