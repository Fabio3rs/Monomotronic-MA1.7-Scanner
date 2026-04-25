#include "TopBarComponents.h"
#include "../../utils/ColorConversion.h"
#include "../../utils/Layout.h"
#include <algorithm>
#include <ctime>

namespace {
constexpr float kLatencyGoodMs = 60.0f;
constexpr float kLatencyWarnMs = 120.0f;
constexpr float kLatencyScaleMs = 200.0f;
constexpr float kLatencyWeight = 0.7f;
constexpr float kErrorWeight = 0.3f;
} // namespace

namespace UI::Components {

ImU32 ECUStatusIndicator::GetStatusColor(bool connected) {
    return connected ? IM_COL32(0, 255, 0, 255) : IM_COL32(128, 128, 128, 255);
}

const char *ECUStatusIndicator::GetStatusText(bool connected) {
    return connected ? "CONNECTED" : "DISCONNECTED";
}

void ECUStatusIndicator::Render(bool connected) {
    UI::Colors::TextColored(GetStatusColor(connected), "ECU:");
    ImGui::SameLine();
    ImGui::Text("%s", GetStatusText(connected));
}

float SignalStrengthBars::CalculateSignalStrength(float latency_ms,
                                                  float error_rate) {
    const float latency_ratio =
        std::clamp(latency_ms / kLatencyScaleMs, 0.0f, 1.0f);
    const float clamped_error = std::clamp(error_rate, 0.0f, 1.0f);
    const float penalty =
        latency_ratio * kLatencyWeight + clamped_error * kErrorWeight;
    return 1.0f - std::clamp(penalty, 0.0f, 1.0f);
}

ImU32 SignalStrengthBars::GetLatencyColor(float latency_ms) {
    if (latency_ms < kLatencyGoodMs) {
        return IM_COL32(0, 255, 0, 255);
    }
    if (latency_ms < kLatencyWarnMs) {
        return IM_COL32(255, 165, 0, 255);
    }
    return IM_COL32(255, 0, 0, 255);
}

void SignalStrengthBars::RenderBars(float latency_ms, float error_rate) {
    const float signal_strength =
        CalculateSignalStrength(latency_ms, error_rate);

    constexpr int kSignalBarCount = Layout::TopBar::SIGNAL_BAR_COUNT;
    constexpr float kSignalBarBaseHeight =
        Layout::TopBar::SIGNAL_BAR_BASE_HEIGHT;
    constexpr float kSignalBarStep = Layout::TopBar::SIGNAL_BAR_STEP;
    constexpr float kSignalBarSpacing = Layout::TopBar::SIGNAL_BAR_SPACING;
    constexpr float kSignalBarWidth = Layout::TopBar::SIGNAL_BAR_WIDTH;
    constexpr float kSignalBarBaseline = Layout::TopBar::SIGNAL_BAR_BASELINE;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 bar_start = ImGui::GetCursorScreenPos();

    for (int i = 0; i < kSignalBarCount; i++) {
        float bar_height = kSignalBarBaseHeight + i * kSignalBarStep;
        bool lit = (i < static_cast<int>(signal_strength * kSignalBarCount));
        ImU32 bar_color =
            lit ? IM_COL32(0, 255, 0, 255) : IM_COL32(60, 60, 60, 255);

        ImVec2 p0(bar_start.x + i * kSignalBarSpacing,
                  bar_start.y + kSignalBarBaseline - bar_height);
        ImVec2 p1(bar_start.x + i * kSignalBarSpacing + kSignalBarWidth,
                  bar_start.y + kSignalBarBaseline);
        draw_list->AddRectFilled(p0, p1, bar_color);
    }

    ImGui::Dummy(ImVec2(Layout::TopBar::SIGNAL_BARS_WIDTH,
                        Layout::TopBar::SIGNAL_BARS_HEIGHT));
}

void SignalStrengthBars::RenderLatency(float latency_ms) {
    UI::Colors::TextColored(GetLatencyColor(latency_ms), "LAT: %.0fms",
                            latency_ms);
}

TableStatus TableStatusDisplay::GetStatus(int active_table) {
    TableStatus status{"TABLE: -", IM_COL32(160, 160, 160, 255)};
    if (active_table == 1) {
        status.label = "TABLE: T1";
        status.color = IM_COL32(0, 200, 255, 255);
    } else if (active_table == 2) {
        status.label = "TABLE: T2";
        status.color = IM_COL32(0, 255, 170, 255);
    }
    return status;
}

void TableStatusDisplay::Render(int active_table) {
    const TableStatus status = GetStatus(active_table);
    UI::Colors::TextColored(status.color, "%s", status.label);
}

double SensorFreshnessDisplay::CalculateAgeSeconds(
    const std::vector<SensorState> &sensors, double now_sec) {
    double freshest = 0.0;
    for (const auto &sensor : sensors) {
        if (sensor.lastUpdateTime > freshest) {
            freshest = sensor.lastUpdateTime;
        }
    }

    return (freshest > 0.0) ? (now_sec - freshest) : 999.0;
}

ImU32 SensorFreshnessDisplay::GetAgeColor(double age_sec) {
    if (age_sec < 0.3) {
        return IM_COL32(0, 255, 0, 255);
    }
    if (age_sec < 1.0) {
        return IM_COL32(255, 165, 0, 255);
    }
    return IM_COL32(255, 0, 0, 255);
}

void SensorFreshnessDisplay::Render(const std::vector<SensorState> &sensors,
                                    double now_sec) {
    const double age = CalculateAgeSeconds(sensors, now_sec);
    UI::Colors::TextColored(GetAgeColor(age), "FRESH: %.1fs", age);
}

void TopBarTimeDisplay::Render() {
    char time_buf[64];
    time_t now = time(nullptr);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", localtime(&now));
    ImGui::Text("%s", time_buf);
}

void SessionManager::Render(float button_width, float button_height) {
    constexpr const char *kPopupName = "Session Manager";
    if (ImGui::Button("\u2261 Sessions", ImVec2(button_width, button_height))) {
        ImGui::OpenPopup(kPopupName);
    }

    if (ImGui::BeginPopupModal(kPopupName, nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Session management coming in Phase 2");
        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(Layout::Button::MODAL_WIDTH, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace UI::Components
