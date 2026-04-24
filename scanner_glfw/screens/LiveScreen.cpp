#include "LiveScreen.h"
#include "../app_data.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/RecordingManager.h"
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/FileIO.h"
#include "../utils/Layout.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <ctime>

using namespace Layout::Button; // Use button constants

extern ImFont *font_large;

// C.41: Constructor initializes all members explicitly
LiveScreen::LiveScreen() : paused_sensor_snapshot_() {
    // Configure sensor table
    sensor_table_.SetShowPinButtons(true);
    sensor_table_.SetShowSparklines(true);

    // Set callbacks
    sensor_table_.SetPinCallback(
        [this](int idx, bool pinned) { OnPinToggle(idx, pinned); });
    sensor_table_.SetIsPinnedCallback(
        [this](int idx) { return IsSensorPinned(idx); });
}

void LiveScreen::OnEnter() {
    SetActive(true);

    // Load custom sensors from state
    auto &state = StateManager::Instance();
    if (state.IsInitialized()) {
        customSensorList = state.LoadCustomSensors();
        pinnedSensorIndices = state.LoadPinnedSensors();
    }

    // If custom list is empty, populate with ECU defaults
    if (customSensorList.empty()) {
        customSensorList = defaultSensorList;
    }

    ApplySubscriptions();

    sensor_list_modal_.SetOnConfirm([this](const std::vector<int> &selection) {
        customSensorList = selection;

        // Save to state
        auto &state = StateManager::Instance();
        if (state.IsInitialized()) {
            state.SaveCustomSensors(customSensorList);
        }

        ApplySubscriptions();
    });
}

void LiveScreen::OnExit() {
    SetActive(false);

    // Stop recording if active
    if (RecordingManager::Instance().IsRecording()) {
        StopRecording();
    }

    if (sensor_list_modal_.IsOpen()) {
        sensor_list_modal_.Close();
    }

    ECUBackend::Instance().ClearSubscriptions();

    // Save state
    auto &state = StateManager::Instance();
    if (state.IsInitialized()) {
        state.SaveCustomSensors(customSensorList);
        state.SavePinnedSensors(pinnedSensorIndices);
    }
}

void LiveScreen::Update(float delta_time) {
    // Update fade alpha for smooth transitions
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("live_screen", 0.2f));

    // Write recording sample if active and not paused
    if (RecordingManager::Instance().IsRecording() && !paused_) {
        WriteRecordingSample();
    }

    // Toast timer
    if (show_recording_toast_) {
        recording_toast_timer_ -= delta_time;
        if (recording_toast_timer_ <= 0.0f) {
            show_recording_toast_ = false;
        }
    }

    // Fade snapshot flash
    if (snapshot_flash_alpha_ > 0.0f) {
        snapshot_flash_alpha_ -= delta_time * 3.0f; // Fade over 0.33s
        if (snapshot_flash_alpha_ < 0.0f)
            snapshot_flash_alpha_ = 0.0f;
    }

    // Fade snap button feedback
    if (snap_button_feedback_) {
        snap_feedback_time_ -= delta_time;
        if (snap_feedback_time_ <= 0.0) {
            snap_button_feedback_ = false;
        }
    }
}

void LiveScreen::Render() {
    // Apply fade
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, GetFadeAlpha());

    // Content area (cursor already at (0,0) within ContentArea parent)
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("LiveScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Render snapshot flash overlay
    if (snapshot_flash_alpha_ > 0.0f) {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec2 screen_min = ImGui::GetCursorScreenPos();
        ImVec2 screen_max =
            ImVec2(screen_min.x + ImGui::GetContentRegionAvail().x,
                   screen_min.y + content_height);
        ImU32 flash_color = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.0f, 1.0f, 1.0f, snapshot_flash_alpha_ * 0.3f));
        draw_list->AddRectFilled(screen_min, screen_max, flash_color);
    }

    RenderTopControls();

    ImGui::Spacing();

    // Calculate remaining height for sensor table + alert banner
    const float controls_height = ImGui::GetCursorPosY();
    const float alert_banner_height = HasCriticalSensors() ? 60.0f : 0.0f;
    const float table_height = content_height - controls_height -
                               alert_banner_height - Layout::SPACING_MEDIUM;

    // Render sensor table inside a child window (FIX: Alert banner inside
    // child)
    ImGui::BeginChild("SensorTableArea",
                      ImVec2(0, table_height + alert_banner_height), true,
                      ImGuiWindowFlags_NoScrollbar);

    RenderSensorTable();

    // Render alert banner INSIDE the child window (BUG FIX #1)
    if (HasCriticalSensors()) {
        ImGui::Spacing();
        RenderAlertBanner();
    }

    ImGui::EndChild();

    ImGui::EndChild();

    // Render modal
    sensor_list_modal_.Render();

    // Recording toast (reuse GraphScreen style)
    if (show_recording_toast_) {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 vp_size = ImGui::GetIO().DisplaySize;
        const float toast_width = 420.0f;
        const float toast_height = 90.0f;
        const ImVec2 toast_pos((vp_size.x - toast_width) * 0.5f,
                               vp_size.y - toast_height - 120.0f);
        const ImVec2 toast_max(toast_pos.x + toast_width,
                               toast_pos.y + toast_height);

        const float alpha = std::min(1.0f, recording_toast_timer_);
        ImVec4 bg_color = recording_toast_error_
                              ? Colors::Status::CRITICAL
                              : ThemeManager::Instance().GetSuccessColor();
        bg_color.w = 0.9f * alpha;

        draw_list->AddRectFilled(toast_pos, toast_max,
                                 ImGui::ColorConvertFloat4ToU32(bg_color),
                                 6.0f);

        ImVec4 border_color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
        draw_list->AddRect(toast_pos, toast_max,
                           ImGui::ColorConvertFloat4ToU32(border_color), 6.0f,
                           0, 2.0f);

        constexpr float text_padding = 10.0f;
        const ImVec2 text_pos(toast_pos.x + text_padding,
                              toast_pos.y + text_padding);
        ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
        draw_list->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(text_color),
                           recording_toast_message_.c_str());
    }

    ImGui::PopStyleVar();
}

bool LiveScreen::HandleGesture(const GestureEvent &event) {
    // Handle swipe gestures for screen navigation
    if (event.type == GestureType::SWIPE_LEFT) {
        SetCurrentScreen(Screen::GRAPH);
        return true;
    }

    return false;
}

void LiveScreen::RenderTopControls() {
    auto &theme = ThemeManager::Instance();

    // Single row: Custom vs Default ECU list
    if (ImGui::RadioButton("Custom", !show_all_sensors_)) {
        show_all_sensors_ = false;
        ApplySubscriptions();
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Default", show_all_sensors_)) {
        show_all_sensors_ = true;
        ApplySubscriptions();
    }

    ImGui::SameLine();

    // Edit button (always visible with placeholder)
    if (!show_all_sensors_) {
        if (ImGui::Button("\uF040 Edit",
                          ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA pencil
            sensor_list_modal_.Open(customSensorList, simulatedSensors.size());
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Customize visible sensors");
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("\uF040 Edit", ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT));
        ImGui::EndDisabled();
    }

    ImGui::SameLine(0.0f, SPACING); // 8px spacing
    if (!show_all_sensors_) {
        if (ImGui::Button("Reset to ECU", ImVec2(WIDE_WIDTH, WIDE_HEIGHT))) {
            customSensorList = defaultSensorList;
            // Save to state
            auto &state = StateManager::Instance();
            if (state.IsInitialized()) {
                state.SaveCustomSensors(customSensorList);
            }
            ApplySubscriptions();
        }
    } else {
        ImGui::Dummy(ImVec2(WIDE_WIDTH, WIDE_HEIGHT));
    }

    ImGui::SameLine(0.0f, SPACING); // 8px spacing

    // C.21: Save state before modification to ensure Push/Pop balance
    {
        const bool was_recording = RecordingManager::Instance().IsRecording();

        // Visual feedback: pulsing red button when recording
        if (was_recording) {
            auto &anim = AnimationSystem::Instance();
            const float pulse = anim.GetPulse(2.0f); // 2 Hz
            const float alpha = 0.5f + 0.5f * pulse;
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.8f, 0.2f, 0.2f, alpha));
        }

        ImGui::BeginDisabled(simulatedSensors.empty());
        if (ImGui::Button(was_recording ? "\uF04D Stop Rec" : "\uF111 Rec",
                          ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) {
            if (was_recording) {
                StopRecording();
            } else {
                if (!StartRecording()) {
                    fprintf(stderr, "ERROR: Failed to start recording\n");
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(was_recording ? "Stop recording session"
                                            : "Start recording sensor data");
        }
        ImGui::EndDisabled();

        // C.41: Pop using saved state to guarantee balance
        if (was_recording) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine(0.0f, SPACING); // 8px spacing

    // Visual feedback when paused
    if (paused_) {
        const ImVec4 warning_color = theme.GetWarningColor();
        // Create a slightly lighter hover color
        const ImVec4 warning_hover =
            ImVec4(warning_color.x * 1.2f, warning_color.y * 1.2f,
                   warning_color.z * 1.2f, warning_color.w);

        ImGui::PushStyleColor(ImGuiCol_Button, warning_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, warning_hover);
    }

    if (ImGui::Button(paused_ ? "\uF04B Play" : "\uF04C Pause",
                      ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA play/pause
        paused_ = !paused_;

        // Snapshot sensor values when pausing
        if (paused_) {
            paused_sensor_snapshot_ = simulatedSensors;
        } else {
            // Clear snapshot when resuming
            paused_sensor_snapshot_.clear();
        }
    }

    if (paused_) {
        ImGui::PopStyleColor(2);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(paused_ ? "Resume live data"
                                  : "Freeze display for inspection");
    }

    ImGui::SameLine(0.0f, SPACING); // 8px spacing

    // Visual feedback when snap was recently pressed
    if (snap_button_feedback_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    }

    if (ImGui::Button("\uF0E7 Snap",
                      ImVec2(SECONDARY_WIDTH, SECONDARY_HEIGHT))) { // FA bolt
        // E.2: Check return value and signal errors
        if (TakeSnapshot()) {
            snap_button_feedback_ = true;
            snap_feedback_time_ = 0.5; // 500ms feedback
        } else {
            fprintf(stderr,
                    "ERROR: Failed to take snapshot (file I/O error)\n");
        }
    }

    if (snap_button_feedback_) {
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::InputText("##filter", filter_buffer_, sizeof(filter_buffer_))) {
        sensor_table_.SetFilter(filter_buffer_);
    }

    ImGui::SameLine();
    ImGui::TextColored(theme.GetSecondaryColor(), "Filter");

    // Recording status text
    if (RecordingManager::Instance().IsRecording()) {
        const uint64_t samples = RecordingManager::Instance().GetSampleCount();
        const std::string &path = RecordingManager::Instance().GetCurrentPath();

        std::string short_path = path;
        const std::string marker = "/recordings/";
        const auto pos = path.find(marker);
        if (pos != std::string::npos) {
            short_path = path.substr(pos + 1); // keep recordings/...
        }

        ImGui::SameLine(0.0f, SPACING);
        ImGui::TextColored(
            theme.GetAccentColor(), "REC: %llu samples \u2022 %s",
            static_cast<unsigned long long>(samples), short_path.c_str());
    }
}

void LiveScreen::RenderSensorTable() {
    const std::vector<int> visible_indices = GetVisibleSensorIndices();
    const float table_height = ImGui::GetContentRegionAvail().y -
                               (HasCriticalSensors() ? 60.0f : 0.0f);

    // Use frozen snapshot when paused, otherwise use live data
    const auto &sensor_data = paused_ && !paused_sensor_snapshot_.empty()
                                  ? paused_sensor_snapshot_
                                  : simulatedSensors;

    const int detail_sensor_idx =
        sensor_table_.Render(visible_indices, sensor_data, table_height);

    // Open detail modal if sensor clicked
    if (detail_sensor_idx >= 0) {
        selectedSensorForDetail = detail_sensor_idx;
        // TODO: Open sensor detail modal
    }
}

void LiveScreen::RenderAlertBanner() {
    auto &anim = AnimationSystem::Instance();

    const float pulse = anim.GetPulse(2.0f); // 2 Hz pulsing
    const float alpha = 0.5f + 0.5f * pulse; // 0.5 - 1.0

    ImVec4 bg_color = Colors::Status::CRITICAL;
    bg_color.w = alpha;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);
    ImGui::BeginChild("AlertBanner", ImVec2(0, 50.0f), true);

    ImGui::PushFont(font_large);
    ImGui::TextColored(
        ImVec4(1, 1, 1, 1),
        "\u26A0 ALERTA: Sensores cr\u00edticos fora da faixa!"); // ⚠
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

std::vector<int> LiveScreen::GetVisibleSensorIndices() const {
    return show_all_sensors_ ? defaultSensorList : customSensorList;
}

void LiveScreen::ApplySubscriptions() {
    ECUBackend::Instance().SubscribeSensors(GetVisibleSensorIndices());
}

bool LiveScreen::HasCriticalSensors() const {
    for (const auto &sensor : simulatedSensors) {
        if (sensor.status == SensorStatus::CRITICAL) {
            return true;
        }
    }
    return false;
}

void LiveScreen::OnPinToggle(int sensor_idx, bool pinned) {
    if (pinned) {
        // Add to pinned list
        if (std::find(pinnedSensorIndices.begin(), pinnedSensorIndices.end(),
                      sensor_idx) == pinnedSensorIndices.end()) {
            pinnedSensorIndices.push_back(sensor_idx);
        }
    } else {
        // Remove from pinned list
        pinnedSensorIndices.erase(std::remove(pinnedSensorIndices.begin(),
                                              pinnedSensorIndices.end(),
                                              sensor_idx),
                                  pinnedSensorIndices.end());
    }

    // Save to state
    auto &state = StateManager::Instance();
    if (state.IsInitialized()) {
        state.SavePinnedSensors(pinnedSensorIndices);
    }
}

bool LiveScreen::IsSensorPinned(int sensor_idx) const {
    return std::find(pinnedSensorIndices.begin(), pinnedSensorIndices.end(),
                     sensor_idx) != pinnedSensorIndices.end();
}

// Recording methods
bool LiveScreen::StartRecording() {
    const std::vector<int> indices = GetVisibleSensorIndices();
    const bool started =
        RecordingManager::Instance().StartRecording(simulatedSensors, indices);

    if (started) {
        recording_toast_message_ = "Recording started";
        recording_toast_error_ = false;
        recording_toast_timer_ = 3.0f;
        show_recording_toast_ = true;
    } else {
        recording_toast_message_ = RecordingManager::Instance().GetLastError();
        if (recording_toast_message_.empty()) {
            recording_toast_message_ = "Failed to start recording";
        }
        recording_toast_error_ = true;
        recording_toast_timer_ = 5.0f;
        show_recording_toast_ = true;
    }

    return started;
}

void LiveScreen::StopRecording() {
    RecordingManager::Instance().StopRecording();

    recording_toast_message_ = "Recording stopped";
    recording_toast_error_ = false;
    recording_toast_timer_ = 3.0f;
    show_recording_toast_ = true;
}

void LiveScreen::WriteRecordingSample() {
    RecordingManager::Instance().Update(simulatedSensors, dataSampleSequence,
                                        dataSampleTimestampSec,
                                        dataSampleDeltaSec);
}

// Snapshot methods
bool LiveScreen::TakeSnapshot() {
    // Create snapshots directory
    std::string base_dir = FileIO::GetUserDataDirectory();
    FileIO::CreateDirectoryIfNotExists(base_dir);

    std::string snapshots_dir = base_dir + "/snapshots";
    FileIO::CreateDirectoryIfNotExists(snapshots_dir);

    // Generate timestamped filename
    std::string filename =
        FileIO::GenerateTimestampedFilename("sensor_snapshot", ".csv");
    std::string snapshot_path = snapshots_dir + "/" + filename;

    // Open file
    std::ofstream file(snapshot_path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    // Write CSV header
    FileIO::WriteCSVHeader(file, simulatedSensors, nullptr);

    // Write single data row (snapshot at t=0)
    FileIO::WriteCSVRow(file, 0.0, simulatedSensors, nullptr);

    file.close();

    // Trigger flash feedback
    snapshot_flash_alpha_ = 1.0f;

    return true;
}
