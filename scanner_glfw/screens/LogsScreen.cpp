#include "LogsScreen.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/ImGuiRAII.h"
#include "../utils/Layout.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

LogsScreen::LogsScreen() : last_refresh_(std::chrono::steady_clock::now()) {}

void LogsScreen::OnEnter() {
    // Initialize and load first batch of logs
    RefreshLogs();
}

void LogsScreen::OnExit() {
    // Cleanup if needed
}

void LogsScreen::Update(float delta_time) {
    // Update fade alpha for smooth transitions
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("logs_screen", 0.2f));

    // Auto-refresh logs if not paused
    if (!paused_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_refresh_);

        if (elapsed >= REFRESH_INTERVAL) {
            RefreshLogs();
            last_refresh_ = now;
        }
    }

    // Update export success message timer
    if (show_export_success_) {
        export_message_timer_ -= delta_time;
        if (export_message_timer_ <= 0.0f) {
            show_export_success_ = false;
        }
    }
}

void LogsScreen::Render() {
    // Apply fade
    UI::StyleVarGuard style;
    style.push(ImGuiStyleVar_Alpha, GetFadeAlpha());
    style.push(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    // Use all available height (BottomNav is rendered separately by main.cpp)
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("LogsScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderTopControls();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    RenderLogContent();

    ImGui::EndChild();
}

bool LogsScreen::HandleGesture(const GestureEvent &event) {
    if (event.type == GestureType::SWIPE_LEFT) {
        SetCurrentScreen(Screen::DTC);
        return true;
    }
    return false;
}

void LogsScreen::RenderTopControls() {
    using namespace Layout::Button;

    // Pause/Resume button
    const char *pause_icon = paused_ ? "\uF04B" : "\uF04C";
    const char *pause_label = paused_ ? " Resume" : " Pause";
    std::string pause_text = std::string(pause_icon) + pause_label;

    if (ImGui::Button(pause_text.c_str(), ImVec2(WIDE_WIDTH, PRIMARY_HEIGHT))) {
        paused_ = !paused_;
        if (!paused_) {
            RefreshLogs();
        }
    }

    ImGui::SameLine(0.0f, SPACING);

    // Clear button
    if (ImGui::Button("\uF1F8 Clear", ImVec2(WIDE_WIDTH, PRIMARY_HEIGHT))) {
        ClearLogs();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all log entries");
    }

    ImGui::SameLine(0.0f, SPACING);

    // Export button
    if (ImGui::Button("\uF0C7 Export", ImVec2(WIDE_WIDTH, PRIMARY_HEIGHT))) {
        ExportLogs();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Export logs to file");
    }

    ImGui::SameLine(0.0f, SPACING * 2);

    // Auto-scroll checkbox - align vertically with buttons
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);

    // Export success message (green, 3 seconds)
    if (show_export_success_) {
        ImGui::SameLine(0.0f, SPACING * 2);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15.0f);
        ImGui::TextColored(Colors::Status::OK, "Exported successfully!");
    }
}

void LogsScreen::RenderLogContent() {
    auto &theme = ThemeManager::Instance();

    // Use all remaining height
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("LogsContent", ImVec2(0, content_height), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // Use default ImGui font for better readability with smaller text
    if (log_buffer_.empty()) {
        ImGui::TextColored(
            theme.GetSecondaryColor(),
            "No logs available.\n\n"
            "Logs will appear when ECU communication is active.");
    } else {
        // Efficient text rendering (no per-line processing)
        // Default font is monospace and smaller than font_large
        ImGui::TextUnformatted(log_buffer_.c_str(),
                               log_buffer_.c_str() + log_buffer_.size());
    }

    // Auto-scroll to bottom when new content arrives
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void LogsScreen::RefreshLogs() {
    auto &backend = ECUBackend::Instance();
    log_buffer_.clear();

    if (backend.IsConnected() && backend.ecu_) {
        backend.GetLogs(log_buffer_);
    } else {
        log_buffer_ = "ECU not connected.\n\n"
                      "Connect to ECU to view communication logs.";
    }
}

void LogsScreen::ClearLogs() {
    log_buffer_.clear();

    // Note: The ECU circular buffer cannot be cleared
    // It will naturally overwrite old entries as new data comes in
    // Clearing the local buffer is sufficient for the UI

    // Immediately refresh to show empty or placeholder message
    RefreshLogs();
}

void LogsScreen::ExportLogs() {
    if (log_buffer_.empty()) {
        fprintf(stderr, "WARNING: No logs to export\n");
        return;
    }

    // Generate timestamped filename
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream filename;
    filename << "ecu_logs_"
             << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
             << ".txt";

    // Save to user's home directory
    const char *home = getenv("HOME");
    if (!home)
        home = "/tmp";
    std::string filepath = std::string(home) + "/" + filename.str();

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << log_buffer_;
        file.close();

        // Show success message for 3 seconds
        show_export_success_ = true;
        export_message_timer_ = 3.0f;

        printf("Logs exported to: %s\n", filepath.c_str());
    } else {
        fprintf(stderr, "ERROR: Failed to export logs to %s\n",
                filepath.c_str());
    }
}
