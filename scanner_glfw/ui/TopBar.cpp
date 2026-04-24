#include "TopBar.h"
#include "../core/AnimationSystem.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/Layout.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <ctime>

// External function to get window pointer from main.cpp
extern GLFWwindow *GetAppWindow();

void TopBar::Render(float bar_height) {
    auto &theme = ThemeManager::Instance();

    ImGui::BeginChild("TopBar", ImVec2(0, bar_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Add vertical padding - different for left and right sides
    constexpr float kLeftVerticalOffset = 2.0f; // Small padding for ECU status
    constexpr float kRightVerticalOffset =
        -10.0f; // More negative for time (to prevent clipping)

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kLeftVerticalOffset);

    // Left side: Connection status + signal bars
    RenderConnectionStatus();
    ImGui::SameLine();
    RenderSignalBars();

    // Center: Simulation mode banner
    if (!ecu_connected_) {
        ImGui::SameLine();
        RenderSimulationBanner();
    }

    // Right side: Window controls + Theme toggle + Time
    // ES.10: Calculate actual time width dynamically to prevent overlap
    const float right_padding = Layout::TopBar::RIGHT_PADDING;
    const float theme_btn_width = Layout::Button::ICON_SIZE;
    const float window_controls_width =
        Layout::Button::ICON_SIZE * 2; // Fullscreen + Close

    char time_buffer[32];
    const time_t now = time(nullptr);
    const tm *local_time = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", local_time);
    const float time_width =
        ImGui::CalcTextSize(time_buffer).x + Layout::TopBar::RIGHT_PADDING;

    const float right_total_width = window_controls_width + theme_btn_width +
                                    time_width + right_padding * 3;

    const float available_width = ImGui::GetContentRegionAvail().x;
    if (available_width > right_total_width) {
        ImGui::SameLine(ImGui::GetCursorPosX() + available_width -
                        right_total_width);
    } else {
        ImGui::SameLine();
    }

    // Apply different offset for right side (time needs to go higher to avoid
    // clipping)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kRightVerticalOffset -
                         kLeftVerticalOffset);
    const float right_baseline_y =
        ImGui::GetCursorPosY(); // Save Y for time alignment

    RenderWindowControls(); // Fullscreen + Close buttons
    RenderThemeToggle();

    ImGui::SameLine();
    ImGui::SetCursorPosY(right_baseline_y); // Restore Y after SameLine
    RenderTime();

    ImGui::EndChild();

    // Separator line
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 p_min = ImVec2(0, bar_height);
    const ImVec2 p_max = ImVec2(ImGui::GetIO().DisplaySize.x,
                                bar_height + Layout::TopBar::SEPARATOR_HEIGHT);
    draw_list->AddRectFilled(
        p_min, p_max, ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()));
}

void TopBar::RenderConnectionStatus() {
    auto &theme = ThemeManager::Instance();

    // ES.5: Eliminate duplication - single label
    const char *status_label = "ECU:";
    const ImVec4 status_color =
        ecu_connected_ ? Colors::Status::OK : Colors::Status::CRITICAL;
    const char *connection_text = ecu_connected_ ? "CONNECTED" : "DISCONNECTED";

    ImGui::TextColored(theme.GetTextColor(), "%s", status_label);
    ImGui::SameLine();
    ImGui::TextColored(status_color, "%s", connection_text);

    if (ecu_connected_) {
        ImGui::SameLine();
        ImGui::TextColored(theme.GetSecondaryColor(), "|");
        ImGui::SameLine();

        const ImVec4 latency_color = (latency_ms_ < 20.0f) ? Colors::Status::OK
                                     : (latency_ms_ < 50.0f)
                                         ? Colors::Status::WARN
                                         : Colors::Status::CRITICAL;

        ImGui::TextColored(latency_color, "%.1fms", latency_ms_);
    }
}

void TopBar::RenderSignalBars() {
    if (!ecu_connected_)
        return;

    const int num_bars = CalculateSignalBars();
    constexpr int kSignalBarCount = 5;
    constexpr float kBarWidth = 4.0f;
    constexpr float kBarSpacing = 2.0f;
    constexpr float kBarMaxHeight = 20.0f;
    constexpr float kBarCornerRadius = 1.0f;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    auto &theme = ThemeManager::Instance();

    for (int i = 0; i < kSignalBarCount; ++i) {
        const float bar_height =
            kBarMaxHeight * (i + 1) / static_cast<float>(kSignalBarCount);
        const float x = cursor_pos.x + i * (kBarWidth + kBarSpacing);
        const float y = cursor_pos.y + (kBarMaxHeight - bar_height);

        const ImU32 color =
            (i < num_bars)
                ? ImGui::ColorConvertFloat4ToU32(theme.GetPrimaryColor())
                : ImGui::ColorConvertFloat4ToU32(ImVec4(
                      theme.GetSecondaryColor().x, theme.GetSecondaryColor().y,
                      theme.GetSecondaryColor().z, 0.3f));

        draw_list->AddRectFilled(ImVec2(x, y),
                                 ImVec2(x + kBarWidth, y + bar_height), color,
                                 kBarCornerRadius);
    }

    ImGui::Dummy(
        ImVec2(kSignalBarCount * (kBarWidth + kBarSpacing), kBarMaxHeight));
}

int TopBar::CalculateSignalBars() const {
    // Calculate signal strength based on latency and error rate
    // 5 bars: latency <20ms, error_rate <5%
    // 4 bars: latency <50ms, error_rate <10%
    // 3 bars: latency <100ms, error_rate <20%
    // 2 bars: latency <200ms, error_rate <50%
    // 1 bar: latency >200ms or error_rate >50%

    if (latency_ms_ < 20.0f && error_rate_ < 0.05f)
        return 5;
    if (latency_ms_ < 50.0f && error_rate_ < 0.10f)
        return 4;
    if (latency_ms_ < 100.0f && error_rate_ < 0.20f)
        return 3;
    if (latency_ms_ < 200.0f && error_rate_ < 0.50f)
        return 2;
    return 1;
}

void TopBar::RenderSimulationBanner() {
    // Animated warning banner for simulation mode with pulsing effect
    auto &anim = AnimationSystem::Instance();
    const float pulse = anim.GetPulse(2.0f); // 2 Hz pulsing
    const float alpha = 0.6f + 0.4f * pulse; // Oscillate between 0.6 and 1.0

    const ImVec4 warning_color = Colors::Status::WARN;
    const ImVec4 warning_bg =
        ImVec4(warning_color.x, warning_color.y, warning_color.z, 0.2f * alpha);

    // Create a semi-transparent background with pulsing
    ImGui::PushStyleColor(ImGuiCol_ChildBg, warning_bg);
    constexpr float kBannerWidth = 250.0f;
    constexpr float kBannerHeight = 36.0f;
    ImGui::BeginChild("SimBanner", ImVec2(kBannerWidth, kBannerHeight), true,
                      ImGuiWindowFlags_NoScrollbar);

    // Center the text vertically
    const float text_height = ImGui::GetTextLineHeight();
    const float child_height = ImGui::GetContentRegionAvail().y;
    ImGui::SetCursorPosY((child_height - text_height) * 0.5f);

    // Warning icon + text with pulsing alpha
    const ImVec4 pulsing_color =
        ImVec4(warning_color.x, warning_color.y, warning_color.z, alpha);

    ImGui::TextColored(pulsing_color, "\u26A0"); // Warning triangle
    ImGui::SameLine();
    ImGui::TextColored(pulsing_color, "MODO SIMULA\u00C7\u00C3O");

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void TopBar::RenderThemeToggle() {
    auto &theme = ThemeManager::Instance();
    constexpr float kIconButtonHeight =
        Layout::Button::ICON_SIZE; // 48px touch target

    // Moon icon for dark mode, sun icon for light mode
    const char *icon = (theme.GetTheme() == ThemeManager::Theme::DARK)
                           ? "\xF0\x9F\x8C\x99"
                           : "\xE2\x98\x80\xEF\xB8\x8F"; // 🌙 or ☀️

    if (ImGui::Button(icon,
                      ImVec2(Layout::Button::ICON_SIZE, kIconButtonHeight))) {
        theme.ToggleTheme();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Dark/Light Theme");
    }
}

void TopBar::RenderTime() {
    auto &theme = ThemeManager::Instance();

    // Get current time
    const time_t now = time(nullptr);
    const tm *local_time = localtime(&now);

    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", local_time);

    ImGui::TextColored(theme.GetTextColor(), "%s", time_buffer);
}

void TopBar::ToggleFullscreen() {
    GLFWwindow *window = GetAppWindow();
    if (!window) {
        fprintf(stderr,
                "ERROR: Cannot toggle fullscreen - window not available\n");
        return;
    }

    if (is_fullscreen_) {
        // Exit fullscreen - restore windowed mode
        glfwSetWindowMonitor(window, nullptr, windowed_x_, windowed_y_,
                             windowed_width_, windowed_height_, 0);
        is_fullscreen_ = false;
    } else {
        // Save current windowed position and size
        glfwGetWindowPos(window, &windowed_x_, &windowed_y_);
        glfwGetWindowSize(window, &windowed_width_, &windowed_height_);

        // Enter fullscreen on primary monitor
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (!monitor) {
            fprintf(stderr, "ERROR: Cannot get primary monitor\n");
            return;
        }

        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
        is_fullscreen_ = true;
    }
}

void TopBar::RenderWindowControls() {
    auto &theme = ThemeManager::Instance();
    constexpr float kIconButtonHeight =
        Layout::Button::ICON_SIZE; // 48px touch target

    // Fullscreen/Windowed toggle button
    const char *fullscreen_icon = is_fullscreen_ ? "\uF066" : "\uF065";
    const char *fullscreen_tooltip =
        is_fullscreen_ ? "Exit Fullscreen" : "Enter Fullscreen";

    if (ImGui::Button(fullscreen_icon,
                      ImVec2(Layout::Button::ICON_SIZE, kIconButtonHeight))) {
        ToggleFullscreen();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", fullscreen_tooltip);
    }

    ImGui::SameLine(0, 0);

    // Close button (red tint)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.9f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

    if (ImGui::Button("\u2715",
                      ImVec2(Layout::Button::ICON_SIZE, kIconButtonHeight))) {
        GLFWwindow *window = GetAppWindow();
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Close Application");
    }

    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 0);
}
