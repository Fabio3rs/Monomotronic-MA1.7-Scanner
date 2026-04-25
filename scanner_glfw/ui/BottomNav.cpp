#include "BottomNav.h"
#include "../core/AnimationSystem.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/ImGuiRAII.h"
#include "../utils/Layout.h"
#include "components/Badge.h"
#include <algorithm>

void BottomNav::UpdateBadgeCounts() {
    logging_active_ = loggingActive;

    dtc_count_ = static_cast<int>(
        std::count_if(activeDTCs.begin(), activeDTCs.end(),
                      [](const SimulatedDTC &dtc) { return dtc.active; }));
}

void BottomNav::Render(float bar_height) {
    UpdateBadgeCounts();

    auto &theme = ThemeManager::Instance();

    // Separator line at top
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 vp_size = ImGui::GetIO().DisplaySize;
    const ImVec2 separator_min = ImVec2(0, vp_size.y - bar_height);
    const ImVec2 separator_max =
        ImVec2(vp_size.x,
               vp_size.y - bar_height + Layout::BottomNav::SEPARATOR_HEIGHT);
    draw_list->AddRectFilled(
        separator_min, separator_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()));

    ImGui::SetCursorPosY(vp_size.y - bar_height);
    ImGui::BeginChild("BottomNav", ImVec2(0, bar_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    const float total_width = vp_size.x;
    const float btn_width = total_width / 5.0f;
    const float btn_height = bar_height;

    RenderTabButton("\uF1FE LIVE", Screen::LIVE, btn_width, btn_height);
    ImGui::SameLine(0, 0);

    RenderTabButton("\uF201 GRAPH", Screen::GRAPH, btn_width, btn_height);
    ImGui::SameLine(0, 0);

    RenderTabButton("\uF0E4 DASH", Screen::DASH, btn_width, btn_height);
    ImGui::SameLine(0, 0);

    RenderTabButton("\uF071 DTC", Screen::DTC, btn_width, btn_height);
    ImGui::SameLine(0, 0);

    RenderTabButton("\uF15C LOGS", Screen::LOGS, btn_width, btn_height);

    ImGui::EndChild();
}

void BottomNav::RenderTabButton(const char *label, Screen screen,
                                float btn_width, float btn_height) {
    auto &theme = ThemeManager::Instance();
    const bool is_active = (GetCurrentScreen() == screen);

    UI::StyleColorGuard colors;
    if (is_active) {
        colors.push(ImGuiCol_Button, theme.GetPrimaryColor());
        colors.push(ImGuiCol_ButtonHovered, theme.GetPrimaryColor());
        colors.push(ImGuiCol_ButtonActive, theme.GetPrimaryColor());
    } else {
        colors.push(ImGuiCol_Button, theme.GetBackgroundColor());
        colors.push(ImGuiCol_ButtonHovered, theme.GetHoverColor());
        colors.push(ImGuiCol_ButtonActive,
                    ImVec4(theme.GetPrimaryColor().x, theme.GetPrimaryColor().y,
                           theme.GetPrimaryColor().z, 0.3f));
    }

    const ImVec2 btn_min = ImGui::GetCursorScreenPos();
    const ImVec2 btn_max =
        ImVec2(btn_min.x + btn_width, btn_min.y + btn_height);

    if (ImGui::Button(label, ImVec2(btn_width, btn_height))) {
        SetCurrentScreen(screen);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Go to %s", label);
    }

    // Render active indicator (bottom border)
    if (is_active) {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 indicator_min =
            ImVec2(btn_min.x, btn_max.y - Layout::BottomNav::INDICATOR_HEIGHT);
        const ImVec2 indicator_max = ImVec2(btn_max.x, btn_max.y);
        draw_list->AddRectFilled(
            indicator_min, indicator_max,
            ImGui::ColorConvertFloat4ToU32(theme.GetAccentColor()));
    }

    // Render badges
    if (screen == Screen::GRAPH && logging_active_) {
        auto &anim = AnimationSystem::Instance();
        const float pulse = anim.GetPulse(2.0f);
        const float alpha = 0.6f + 0.4f * pulse;

        constexpr float badge_margin = Layout::Badge::DOT_MARGIN;
        constexpr float badge_radius = Layout::Badge::DOT_RADIUS;

        const ImVec4 pulse_color =
            ImVec4(Colors::Status::CRITICAL.x, Colors::Status::CRITICAL.y,
                   Colors::Status::CRITICAL.z, alpha);
        UI::Components::Badge::RenderDotBadgeAtCorner(
            btn_min, btn_max, badge_radius, badge_margin, pulse_color);
    }

    if (screen == Screen::DTC && dtc_count_ > 0) {
        UI::Components::BadgeConfig config;
        config.background_color = Colors::Status::CRITICAL;
        UI::Components::Badge::RenderCountBadgeAtCorner(btn_min, btn_max,
                                                        dtc_count_, config);
    }
}
