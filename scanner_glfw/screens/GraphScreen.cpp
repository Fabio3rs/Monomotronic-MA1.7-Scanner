#include "GraphScreen.h"
#include "../app_data.h"
#include "../core/AnimationSystem.h"
#include "../core/ECUBackend.h"
#include "../core/StateManager.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/FileIO.h"
#include "../utils/Layout.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>

using namespace Layout::Button; // Use button constants

extern ImFont *font_large;

GraphScreen::GraphScreen() : signal_modal_("Select Signals") {}

void GraphScreen::OnEnter() {
    SetActive(true);

    // Sync freeze state from global (preserve state across screen transitions)
    graph_frozen_ = graphFrozen;

    LoadStateFromManager();
    SyncPinnedSensorsToGraph();
    InitializeGraphMetadata();
    ApplySubscriptions();
    CacheColorConversions();
}

void GraphScreen::LoadStateFromManager() {
    auto &state = StateManager::Instance();
    if (!state.IsInitialized()) {
        return;
    }

    graphSensorIndices = state.LoadGraphSensors();
}

void GraphScreen::SyncPinnedSensorsToGraph() {
    auto &state = StateManager::Instance();
    if (!state.IsInitialized()) {
        return;
    }

    const std::vector<int> pinned = state.LoadPinnedSensors();
    for (int sensor_idx : pinned) {
        auto it = std::find(graphSensorIndices.begin(),
                            graphSensorIndices.end(), sensor_idx);
        const bool already_in_graph = (it != graphSensorIndices.end());

        // Add to graph if space available (max 4 sensors)
        if (!already_in_graph && graphSensorIndices.size() < 4) {
            graphSensorIndices.push_back(sensor_idx);
        }
    }

    // Save back in case we added any pinned sensors
    if (!pinned.empty()) {
        state.SaveGraphSensors(graphSensorIndices);
    }
}

void GraphScreen::InitializeGraphMetadata() {
    auto_scale_y_.resize(graphSensorIndices.size(), true);
    last_plot_min_y_.resize(graphSensorIndices.size(), 0.0f);
    last_plot_max_y_.resize(graphSensorIndices.size(), 100.0f);
}

void GraphScreen::CacheColorConversions() {
    // Per.10: Cache color conversions to avoid per-frame overhead
    for (size_t i = 0;
         i < Colors::GraphColorsCount && i < cached_graph_colors_.size(); ++i) {
        cached_graph_colors_[i] =
            ImGui::ColorConvertFloat4ToU32(Colors::GraphColors[i]);
    }
}

void GraphScreen::OnExit() {
    SetActive(false);

    // Stop logging if active
    if (logging_active_) {
        StopLogging();
    }

    ECUBackend::Instance().ClearSubscriptions();

    // Sync freeze state to global (preserve across screen transitions)
    graphFrozen = graph_frozen_;

    // Save configuration
    auto &state = StateManager::Instance();
    if (state.IsInitialized()) {
        state.SaveGraphSensors(graphSensorIndices);
    }
}

void GraphScreen::Update(float delta_time) {
    // Update fade alpha for smooth transitions
    auto &anim = AnimationSystem::Instance();
    SetFadeAlpha(anim.GetFade("graph_screen", 0.2f));

    // E.2: Update error toast timer
    if (show_error_toast_) {
        error_toast_timer_ -= delta_time;
        if (error_toast_timer_ <= 0.0f) {
            show_error_toast_ = false;
        }
    }

    // F.15: CORREÇÃO - Não processar updates se congelado
    if (graph_frozen_) {
        return; // Skip all updates when frozen
    }

    // Write log sample if active
    if (logging_active_) {
        WriteLogSample();
    }

    // Update auto-scale for all signals (BUG FIX #4: Smooth interpolation)
    for (size_t i = 0; i < graphSensorIndices.size(); ++i) {
        if (auto_scale_y_[i]) {
            UpdateAutoScale(static_cast<int>(i));
        }
    }
}

void GraphScreen::Render() {
    auto &theme = ThemeManager::Instance();

    // Apply fade
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, GetFadeAlpha());

    // Content area (cursor already at (0,0) within ContentArea parent)
    const float content_height = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("GraphScreenContent", ImVec2(0, content_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderTopControls();

    ImGui::Spacing();

    // Calculate remaining height for graph canvas
    const float controls_height = ImGui::GetCursorPosY();
    const float canvas_height =
        content_height - controls_height - Layout::SPACING_MEDIUM;

    // Render graph canvas
    ImGui::BeginChild("GraphCanvas", ImVec2(0, canvas_height), true,
                      ImGuiWindowFlags_NoScrollbar);
    RenderGraphCanvas();
    ImGui::EndChild();

    ImGui::EndChild();

    // Render modal
    RenderSignalSelector();

    // E.2: Render error toast if active
    if (show_error_toast_) {
        RenderErrorToast();
    }

    ImGui::PopStyleVar();
}

bool GraphScreen::HandleGesture(const GestureEvent &event) {
    // Handle swipe gestures for screen navigation
    if (event.type == GestureType::SWIPE_RIGHT) {
        SetCurrentScreen(Screen::LIVE);
        return true;
    }
    if (event.type == GestureType::SWIPE_LEFT) {
        SetCurrentScreen(Screen::DASH);
        return true;
    }

    return false;
}

void GraphScreen::RenderTopControls() {
    auto &theme = ThemeManager::Instance();

    // Single row: All controls in one line
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::SliderFloat("##timewindow", &time_window_secs_, 5.0f, 120.0f,
                           "%.0f s")) {
        // Time window changed
    }

    ImGui::SameLine();

    if (ImGui::Button("\uF013 Signals",
                      ImVec2(SECONDARY_WIDTH, SECONDARY_HEIGHT))) { // FA cog
        show_signal_selector_ = true;
        signal_modal_.Open();
    }

    ImGui::SameLine();

    if (ImGui::Button(graph_frozen_ ? "\uF04B Resume" : "\uF04C Freeze",
                      ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA play/pause
        graph_frozen_ = !graph_frozen_;
        graphFrozen = graph_frozen_; // Sync to global state
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(graph_frozen_ ? "Resume live graph"
                                        : "Freeze graph for analysis");
    }

    ImGui::SameLine();

    if (ImGui::Button(
            logging_active_ ? "\uF04D Stop Log" : "\uF111 Rec",
            ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA stop/circle
        if (logging_active_) {
            StopLogging();
        } else {
            // E.2: Check return value and propagate errors to UI
            if (!StartLogging()) {
                last_error_message_ = "Failed to start logging:\nNo sensors "
                                      "selected or file I/O error";
                show_error_toast_ = true;
                error_toast_timer_ = 5.0f; // Show for 5 seconds
                fprintf(stderr, "ERROR: Failed to start logging (no sensors "
                                "selected or file I/O error)\n");
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("\uF067 Cursor A",
                      ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA plus
        cursor_a_active_ = !cursor_a_active_;
        if (cursor_a_active_) {
            // Place at center of canvas
            cursor_a_pos_ = ImVec2(0.5f, 0.5f);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("\uF067 Cursor B",
                      ImVec2(PRIMARY_WIDTH, PRIMARY_HEIGHT))) { // FA plus
        cursor_b_active_ = !cursor_b_active_;
        if (cursor_b_active_) {
            cursor_b_pos_ = ImVec2(0.7f, 0.5f);
        }
    }
}

void GraphScreen::RenderGraphCanvas() {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec2 canvas_min = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_max =
        ImVec2(canvas_min.x + ImGui::GetContentRegionAvail().x,
               canvas_min.y + ImGui::GetContentRegionAvail().y);

    // Background
    draw_list->AddRectFilled(
        canvas_min, canvas_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetBackgroundColor()));

    // Border
    draw_list->AddRect(canvas_min, canvas_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       0.0f, 0, 2.0f);

    // Pulsing red border when logging
    if (logging_active_) {
        auto &anim = AnimationSystem::Instance();
        const float pulse = anim.GetPulse(2.0f); // 2 Hz
        const float alpha = 0.5f + 0.5f * pulse;
        ImU32 rec_color =
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.9f, 0.2f, 0.2f, alpha));
        draw_list->AddRect(canvas_min, canvas_max, rec_color, 0.0f, 0, 4.0f);
    }

    // Enable clipping to canvas bounds (FEATURE: Canvas clipping)
    draw_list->PushClipRect(canvas_min, canvas_max, true);

    // Empty state
    if (graphSensorIndices.empty()) {
        const ImVec2 text_size = ImGui::CalcTextSize("No signals selected");
        const ImVec2 text_pos = ImVec2(
            canvas_min.x + (canvas_max.x - canvas_min.x - text_size.x) * 0.5f,
            canvas_min.y + (canvas_max.y - canvas_min.y - text_size.y) * 0.5f);

        draw_list->AddText(
            text_pos, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
            "No signals selected");
        draw_list->PopClipRect();
        return;
    }

    // Draw grid
    const float grid_step_x = (canvas_max.x - canvas_min.x) / 10.0f;
    const float grid_step_y = (canvas_max.y - canvas_min.y) / 8.0f;

    const ImU32 grid_color = ImGui::ColorConvertFloat4ToU32(
        ImVec4(theme.GetBorderColor().x, theme.GetBorderColor().y,
               theme.GetBorderColor().z, 0.3f));

    for (int i = 1; i < 10; ++i) {
        const float x = canvas_min.x + i * grid_step_x;
        draw_list->AddLine(ImVec2(x, canvas_min.y), ImVec2(x, canvas_max.y),
                           grid_color);
    }

    for (int i = 1; i < 8; ++i) {
        const float y = canvas_min.y + i * grid_step_y;
        draw_list->AddLine(ImVec2(canvas_min.x, y), ImVec2(canvas_max.x, y),
                           grid_color);
    }

    // Draw all signal plots
    for (size_t sig_idx = 0; sig_idx < graphSensorIndices.size(); ++sig_idx) {
        const int sensor_idx = graphSensorIndices[sig_idx];

        if (sensor_idx < 0 ||
            sensor_idx >= static_cast<int>(simulatedSensors.size())) {
            continue;
        }

        const auto &sensor = simulatedSensors[sensor_idx];

        // BUG FIX #7: Check if history is empty before accessing
        if (sensor.history.empty()) {
            continue;
        }

        const float min_y = last_plot_min_y_[sig_idx];
        const float max_y = last_plot_max_y_[sig_idx];
        const float range = max_y - min_y;

        // ES.10: Named constant for minimum range, I.12: Check precondition
        constexpr float MIN_RANGE = 1e-6f;
        if (range < MIN_RANGE) {
            continue; // Skip rendering - insufficient dynamic range
        }

        // Per.10: Use cached color to avoid conversion overhead
        const ImU32 line_color =
            cached_graph_colors_[sig_idx % cached_graph_colors_.size()];

        // Calculate how many samples to display based on time window
        const size_t history_size = sensor.history.size();
        const size_t visible_samples =
            static_cast<size_t>(time_window_secs_ * SAMPLE_RATE);

        // Determine actual samples to render (handle case where history < time
        // window)
        const size_t samples_to_render =
            std::min(visible_samples, history_size);

        // Calculate start index (always show most recent data)
        const size_t start_idx = history_size > samples_to_render
                                     ? history_size - samples_to_render
                                     : 0;

        const float canvas_width = canvas_max.x - canvas_min.x;

        // Render only the visible window
        // Require at least 2 samples to avoid division by zero
        if (samples_to_render < 2) {
            continue;
        }

        for (size_t i = start_idx + 1; i < history_size; ++i) {
            // Bounds check: ensure both array accesses are valid
            if (i == 0 || i - 1 >= sensor.history.size() ||
                i >= sensor.history.size()) {
                continue;
            }

            // Normalize position relative to visible samples, not total history
            const float t0 = static_cast<float>(i - start_idx - 1) /
                             static_cast<float>(samples_to_render - 1);
            const float t1 = static_cast<float>(i - start_idx) /
                             static_cast<float>(samples_to_render - 1);

            const float x0 = canvas_min.x + t0 * canvas_width;
            const float x1 = canvas_min.x + t1 * canvas_width;

            const float val0 = sensor.history[i - 1];
            const float val1 = sensor.history[i];

            const float y0 = canvas_max.y - ((val0 - min_y) / range) *
                                                (canvas_max.y - canvas_min.y);
            const float y1 = canvas_max.y - ((val1 - min_y) / range) *
                                                (canvas_max.y - canvas_min.y);

            // Per.10: Use pre-converted cached color
            draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), line_color,
                               2.5f);
        }
    }

    // Render cursors
    RenderCursors(draw_list, canvas_min, canvas_max);

    // Render legend (BUG FIX: Semi-transparent background)
    RenderLegend(draw_list, canvas_min, canvas_max);

    // Render delta box if both cursors active (BUG FIX #6: Position correctly)
    if (cursor_a_active_ && cursor_b_active_) {
        RenderDeltaBox(draw_list, canvas_min, canvas_max);
    }

    // Render cursor values comparison table
    RenderCursorValuesTable(draw_list, canvas_min, canvas_max);

    draw_list->PopClipRect();

    // Invisible button for canvas click detection
    ImGui::SetCursorScreenPos(canvas_min);
    ImGui::InvisibleButton("GraphCanvas", ImVec2(canvas_max.x - canvas_min.x,
                                                 canvas_max.y - canvas_min.y));

    // I.12: Hover probe detection with edge tolerance
    if (ImGui::IsItemHovered()) {
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        const float canvas_width = canvas_max.x - canvas_min.x;
        const float canvas_height = canvas_max.y - canvas_min.y;

        // ES.10: Add edge margin for better edge detection
        constexpr float edge_margin = 2.0f;
        const bool within_x = (mouse_pos.x >= canvas_min.x - edge_margin &&
                               mouse_pos.x <= canvas_max.x + edge_margin);
        const bool within_y = (mouse_pos.y >= canvas_min.y - edge_margin &&
                               mouse_pos.y <= canvas_max.y + edge_margin);

        if (within_x && within_y) {
            const float mouse_x_norm = std::clamp(
                (mouse_pos.x - canvas_min.x) / canvas_width, 0.0f, 1.0f);
            const float mouse_y_norm = std::clamp(
                (mouse_pos.y - canvas_min.y) / canvas_height, 0.0f, 1.0f);

            hover_probe_pos_ = ImVec2(mouse_x_norm, mouse_y_norm);
            hover_probe_active_ = true;

            RenderHoverProbe(draw_list, canvas_min, canvas_max, mouse_pos);
        } else {
            hover_probe_active_ = false;
        }
    } else {
        hover_probe_active_ = false;
    }

    // I.12: Validate click coordinates before processing
    if (ImGui::IsItemClicked(0)) {
        const ImVec2 click_pos = ImGui::GetMousePos();

        // Validate click is within canvas bounds
        const bool valid_x =
            (click_pos.x >= canvas_min.x && click_pos.x <= canvas_max.x);
        const bool valid_y =
            (click_pos.y >= canvas_min.y && click_pos.y <= canvas_max.y);

        if (valid_x && valid_y) {
            HandleCanvasClick(click_pos, canvas_min, canvas_max);
        }
    }
}

void GraphScreen::RenderLegend(ImDrawList *draw_list, const ImVec2 &canvas_min,
                               const ImVec2 &canvas_max) {
    auto &theme = ThemeManager::Instance();

    if (graphSensorIndices.empty()) {
        return;
    }

    // Legend position: top-right corner with margin
    constexpr float legend_margin = 10.0f;
    constexpr float legend_padding = 8.0f;
    constexpr float line_sample_width = 30.0f;
    constexpr float line_height = 24.0f;

    // Calculate legend size
    float max_text_width = 0.0f;
    for (size_t i = 0; i < graphSensorIndices.size(); ++i) {
        const int sensor_idx = graphSensorIndices[i];
        if (sensor_idx >= 0 &&
            sensor_idx < static_cast<int>(simulatedSensors.size())) {
            const auto &sensor = simulatedSensors[sensor_idx];
            const float text_width = ImGui::CalcTextSize(sensor.name.c_str()).x;
            if (text_width > max_text_width) {
                max_text_width = text_width;
            }
        }
    }

    const float legend_width =
        line_sample_width + 8.0f + max_text_width + 2.0f * legend_padding;
    const float legend_height =
        graphSensorIndices.size() * line_height + 2.0f * legend_padding;

    const ImVec2 legend_min =
        ImVec2(canvas_max.x - legend_width - legend_margin,
               canvas_min.y + legend_margin);
    const ImVec2 legend_max =
        ImVec2(legend_min.x + legend_width, legend_min.y + legend_height);

    // Semi-transparent background (FEATURE: prevent overlap)
    ImVec4 bg_color = theme.GetBackgroundColor();
    bg_color.w = 0.85f;
    draw_list->AddRectFilled(legend_min, legend_max,
                             ImGui::ColorConvertFloat4ToU32(bg_color), 4.0f);

    // Border
    draw_list->AddRect(legend_min, legend_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       4.0f, 0, 1.5f);

    // Render each legend entry
    for (size_t i = 0; i < graphSensorIndices.size(); ++i) {
        const int sensor_idx = graphSensorIndices[i];
        if (sensor_idx < 0 ||
            sensor_idx >= static_cast<int>(simulatedSensors.size())) {
            continue;
        }

        const auto &sensor = simulatedSensors[sensor_idx];
        const ImU32 line_color =
            cached_graph_colors_[i % cached_graph_colors_.size()];

        const float entry_y = legend_min.y + legend_padding + i * line_height;

        // Color sample line
        const ImVec2 line_start =
            ImVec2(legend_min.x + legend_padding, entry_y + line_height * 0.5f);
        const ImVec2 line_end =
            ImVec2(line_start.x + line_sample_width, line_start.y);
        draw_list->AddLine(line_start, line_end, line_color, 3.0f);

        // Sensor name
        const ImVec2 text_pos =
            ImVec2(line_end.x + 8.0f,
                   entry_y + (line_height - ImGui::GetFontSize()) * 0.5f);
        draw_list->AddText(text_pos,
                           ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                           sensor.name.c_str());
    }

    // Per.10: Visual feedback - FROZEN overlay
    if (graph_frozen_) {
        constexpr float overlay_margin = 10.0f;
        constexpr float overlay_width = 120.0f;
        constexpr float overlay_height = 40.0f;

        const ImVec2 overlay_min = ImVec2(canvas_min.x + overlay_margin,
                                          canvas_min.y + overlay_margin);
        const ImVec2 overlay_max = ImVec2(overlay_min.x + overlay_width,
                                          overlay_min.y + overlay_height);

        // Semi-transparent background
        draw_list->AddRectFilled(
            overlay_min, overlay_max,
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.8f)),
            4.0f);

        // Border
        draw_list->AddRect(
            overlay_min, overlay_max,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 1.0f)),
            4.0f, 0, 1.5f);

        // "FROZEN" text
        const char *frozen_text = "FROZEN";
        const ImVec2 text_size = ImGui::CalcTextSize(frozen_text);
        const ImVec2 frozen_text_pos =
            ImVec2(overlay_min.x + (overlay_width - text_size.x) * 0.5f,
                   overlay_min.y + (overlay_height - text_size.y) * 0.5f);

        draw_list->AddText(
            frozen_text_pos,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 1.0f)),
            frozen_text);
    }
}

void GraphScreen::RenderCursors(ImDrawList *draw_list, const ImVec2 &canvas_min,
                                const ImVec2 &canvas_max) {
    auto &theme = ThemeManager::Instance();

    const float canvas_width = canvas_max.x - canvas_min.x;
    const float canvas_height = canvas_max.y - canvas_min.y;
    const ImVec2 mouse_pos = ImGui::GetMousePos();

    // Cursor A (blue/green)
    if (cursor_a_active_) {
        const float cursor_x = canvas_min.x + cursor_a_pos_.x * canvas_width;

        // Per.10: Detect hover for visual feedback
        const bool hovering_a = std::abs(mouse_pos.x - cursor_x) < 8.0f &&
                                mouse_pos.y >= canvas_min.y &&
                                mouse_pos.y <= canvas_max.y;

        // Change color and width on hover/drag
        const ImU32 cursor_color_a =
            hovering_a || dragging_cursor_a_
                ? ImGui::ColorConvertFloat4ToU32(
                      ImVec4(1.0f, 1.0f, 0.0f, 1.0f)) // Yellow
                : ImGui::ColorConvertFloat4ToU32(
                      ImVec4(0.0f, 1.0f, 0.0f, 0.8f)); // Green

        const float line_width = hovering_a || dragging_cursor_a_ ? 3.0f : 2.0f;
        const float circle_radius =
            hovering_a || dragging_cursor_a_ ? 8.0f : 6.0f;

        draw_list->AddLine(ImVec2(cursor_x, canvas_min.y),
                           ImVec2(cursor_x, canvas_max.y), cursor_color_a,
                           line_width);
        draw_list->AddCircleFilled(ImVec2(cursor_x, canvas_min.y + 10.0f),
                                   circle_radius, cursor_color_a);

        // Value readout box
        RenderCursorValueBox(
            draw_list, canvas_min, canvas_max, cursor_a_pos_.x, cursor_x,
            hovering_a || dragging_cursor_a_ ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                                             : ImVec4(0.0f, 1.0f, 0.0f, 0.8f),
            "A");
    }

    // Cursor B (red)
    if (cursor_b_active_) {
        const float cursor_x = canvas_min.x + cursor_b_pos_.x * canvas_width;

        // Per.10: Detect hover for visual feedback
        const bool hovering_b = std::abs(mouse_pos.x - cursor_x) < 8.0f &&
                                mouse_pos.y >= canvas_min.y &&
                                mouse_pos.y <= canvas_max.y;

        // Change color and width on hover/drag
        const ImU32 cursor_color_b =
            hovering_b || dragging_cursor_b_
                ? ImGui::ColorConvertFloat4ToU32(
                      ImVec4(1.0f, 1.0f, 0.0f, 1.0f)) // Yellow
                : ImGui::ColorConvertFloat4ToU32(
                      ImVec4(1.0f, 0.0f, 0.0f, 0.8f)); // Red

        const float line_width = hovering_b || dragging_cursor_b_ ? 3.0f : 2.0f;
        const float circle_radius =
            hovering_b || dragging_cursor_b_ ? 8.0f : 6.0f;

        draw_list->AddLine(ImVec2(cursor_x, canvas_min.y),
                           ImVec2(cursor_x, canvas_max.y), cursor_color_b,
                           line_width);
        draw_list->AddCircleFilled(ImVec2(cursor_x, canvas_min.y + 10.0f),
                                   circle_radius, cursor_color_b);

        // Value readout box
        RenderCursorValueBox(
            draw_list, canvas_min, canvas_max, cursor_b_pos_.x, cursor_x,
            hovering_b || dragging_cursor_b_ ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
                                             : ImVec4(1.0f, 0.0f, 0.0f, 0.8f),
            "B");
    }

    // Hover probe (dashed line with value readout)
    if (hover_probe_active_) {
        const float x_hover = canvas_min.x + hover_probe_pos_.x * canvas_width;
        const ImU32 color_hover = ImGui::ColorConvertFloat4ToU32(
            ImVec4(1.0f, 1.0f, 1.0f, 0.5f) // White semi-transparent
        );

        // Dashed line
        const float dash_length = 8.0f;
        const float gap_length = 4.0f;
        float y = canvas_min.y;

        while (y < canvas_max.y) {
            const float y_end = std::min(y + dash_length, canvas_max.y);
            draw_list->AddLine(ImVec2(x_hover, y), ImVec2(x_hover, y_end),
                               color_hover, 1.5f);
            y += dash_length + gap_length;
        }

        // Value readout box
        RenderCursorValueBox(draw_list, canvas_min, canvas_max,
                             hover_probe_pos_.x, x_hover,
                             ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "H");
    }
}

void GraphScreen::RenderDeltaBox(ImDrawList *draw_list,
                                 const ImVec2 &canvas_min,
                                 const ImVec2 &canvas_max) {
    auto &theme = ThemeManager::Instance();

    // ES.10: Calculate time delta between cursors
    const float time_delta =
        std::abs(cursor_b_pos_.x - cursor_a_pos_.x) * time_window_secs_;

    char delta_text[128];
    snprintf(delta_text, sizeof(delta_text), "\u0394T: %.2fs", time_delta);

    const ImVec2 text_size = ImGui::CalcTextSize(delta_text);
    constexpr float box_padding = 10.0f;
    const ImVec2 box_size(text_size.x + box_padding * 2,
                          60.0f + box_padding * 2);

    // ES.10: Position at bottom-left to avoid legend (top-right)
    constexpr float margin = 10.0f;
    const ImVec2 box_min(canvas_min.x + margin,
                         canvas_max.y - box_size.y - margin);
    const ImVec2 box_max(box_min.x + box_size.x, box_min.y + box_size.y);

    // Semi-transparent background
    ImVec4 bg_color = theme.GetBackgroundColor();
    bg_color.w = 0.90f;
    draw_list->AddRectFilled(box_min, box_max,
                             ImGui::ColorConvertFloat4ToU32(bg_color), 4.0f);

    // Border
    draw_list->AddRect(box_min, box_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       4.0f, 0, 1.5f);

    // Delta text
    const ImVec2 text_pos =
        ImVec2(box_min.x + box_padding, box_min.y + box_padding);
    draw_list->AddText(text_pos,
                       ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                       delta_text);
}

void GraphScreen::RenderHoverProbe(ImDrawList *draw_list,
                                   const ImVec2 &canvas_min,
                                   const ImVec2 &canvas_max,
                                   const ImVec2 &mouse_pos) {
    // Per.10: Draw crosshair at mouse position for visual feedback
    const ImU32 crosshair_color =
        ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.0f, 0.5f));

    // Vertical line
    draw_list->AddLine(ImVec2(mouse_pos.x, canvas_min.y),
                       ImVec2(mouse_pos.x, canvas_max.y), crosshair_color,
                       1.0f);

    // Horizontal line
    draw_list->AddLine(ImVec2(canvas_min.x, mouse_pos.y),
                       ImVec2(canvas_max.x, mouse_pos.y), crosshair_color,
                       1.0f);
}

void GraphScreen::RenderSignalSelector() {
    if (!show_signal_selector_) {
        return;
    }

    // I.11: Garantir que modal está aberto antes de renderizar
    if (!signal_modal_.IsOpen()) {
        signal_modal_.Open();
    }

    bool confirmed = false;
    static std::vector<bool> temp_signal_selection;

    signal_modal_.RenderWithButtons(
        [this]() {
            ImGui::Text("Select signals to display on graph:");
            ImGui::Spacing();

            // Initialize temp selection if needed
            if (temp_signal_selection.size() != simulatedSensors.size()) {
                temp_signal_selection.resize(simulatedSensors.size(), false);
                for (const int idx : graphSensorIndices) {
                    if (idx >= 0 &&
                        idx < static_cast<int>(temp_signal_selection.size())) {
                        temp_signal_selection[idx] = true;
                    }
                }
            }

            // Render checkboxes
            for (size_t i = 0; i < simulatedSensors.size(); ++i) {
                // BUG FIX: std::vector<bool> uses proxy objects, need temp
                // variable
                bool is_selected = temp_signal_selection[i];
                if (ImGui::Checkbox(simulatedSensors[i].name.c_str(),
                                    &is_selected)) {
                    temp_signal_selection[i] = is_selected;
                }
            }
        },
        &confirmed);

    if (confirmed) {
        // Apply selection
        graphSensorIndices.clear();
        for (size_t i = 0; i < temp_signal_selection.size(); ++i) {
            if (temp_signal_selection[i]) {
                graphSensorIndices.push_back(static_cast<int>(i));
            }
        }

        // Resize auto-scale vectors
        auto_scale_y_.resize(graphSensorIndices.size(), true);
        last_plot_min_y_.resize(graphSensorIndices.size(), 0.0f);
        last_plot_max_y_.resize(graphSensorIndices.size(), 100.0f);

        show_signal_selector_ = false;

        // Save to state
        auto &state = StateManager::Instance();
        if (state.IsInitialized()) {
            state.SaveGraphSensors(graphSensorIndices);
        }

        ApplySubscriptions();
    }

    if (!signal_modal_.IsOpen()) {
        show_signal_selector_ = false;
    }
}

void GraphScreen::ApplySubscriptions() {
    ECUBackend::Instance().SubscribeSensors(graphSensorIndices);
}

void GraphScreen::RenderErrorToast() {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    const ImVec2 vp_size = ImGui::GetIO().DisplaySize;
    constexpr float toast_width = 400.0f;
    constexpr float toast_height = 100.0f;

    const ImVec2 toast_pos(
        (vp_size.x - toast_width) * 0.5f,
        vp_size.y - toast_height - 120.0f // Above bottom navigation
    );

    const ImVec2 toast_max(toast_pos.x + toast_width,
                           toast_pos.y + toast_height);

    // Per.10: Fade out smoothly as timer expires
    const float alpha = std::min(1.0f, error_toast_timer_);

    ImVec4 bg_color = Colors::Status::CRITICAL;
    bg_color.w = 0.9f * alpha;

    draw_list->AddRectFilled(toast_pos, toast_max,
                             ImGui::ColorConvertFloat4ToU32(bg_color), 6.0f);

    // Border
    ImVec4 border_color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
    draw_list->AddRect(toast_pos, toast_max,
                       ImGui::ColorConvertFloat4ToU32(border_color), 6.0f, 0,
                       2.0f);

    // Error message text
    constexpr float text_padding = 10.0f;
    const ImVec2 text_pos(toast_pos.x + text_padding,
                          toast_pos.y + text_padding);

    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, alpha);
    draw_list->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(text_color),
                       last_error_message_.c_str());
}

void GraphScreen::UpdateAutoScale(int signal_idx) {
    if (signal_idx < 0 ||
        signal_idx >= static_cast<int>(graphSensorIndices.size())) {
        return;
    }

    const int sensor_idx = graphSensorIndices[signal_idx];
    if (sensor_idx < 0 ||
        sensor_idx >= static_cast<int>(simulatedSensors.size())) {
        return;
    }

    const auto &sensor = simulatedSensors[sensor_idx];

    // BUG FIX #7: Check if history is empty
    if (sensor.history.empty()) {
        return;
    }

    // Only auto-scale based on visible samples
    const size_t history_size = sensor.history.size();
    const size_t visible_samples =
        static_cast<size_t>(time_window_secs_ * SAMPLE_RATE);
    const size_t samples_to_analyze = std::min(visible_samples, history_size);
    const size_t start_idx = history_size > samples_to_analyze
                                 ? history_size - samples_to_analyze
                                 : 0;

    // Find min/max only in visible range
    const auto [min_it, max_it] = std::minmax_element(
        sensor.history.begin() + start_idx, sensor.history.end());
    float target_min = *min_it;
    float target_max = *max_it;

    // ES.10: Add margin for better visualization
    const float range = target_max - target_min;
    constexpr float MARGIN_PERCENT = 0.10f;
    const float margin = range * MARGIN_PERCENT;

    target_min -= margin;
    target_max += margin;

    // Per.10: Improved smoothing to prevent flickering
    constexpr float SMOOTHING_FACTOR = 0.1f;

    // Initialize on first run (detect default values)
    if (last_plot_min_y_[signal_idx] == 0.0f &&
        last_plot_max_y_[signal_idx] == 100.0f) {
        last_plot_min_y_[signal_idx] = target_min;
        last_plot_max_y_[signal_idx] = target_max;
    } else {
        last_plot_min_y_[signal_idx] +=
            (target_min - last_plot_min_y_[signal_idx]) * SMOOTHING_FACTOR;
        last_plot_max_y_[signal_idx] +=
            (target_max - last_plot_max_y_[signal_idx]) * SMOOTHING_FACTOR;
    }

    // Sync to global state for first 4 signals
    if (signal_idx < 4) {
        graphLastPlotMinY[signal_idx] = last_plot_min_y_[signal_idx];
        graphLastPlotMaxY[signal_idx] = last_plot_max_y_[signal_idx];
    }
}

void GraphScreen::HandleCanvasClick(const ImVec2 &click_pos,
                                    const ImVec2 &canvas_min,
                                    const ImVec2 &canvas_max) {
    const float canvas_width = canvas_max.x - canvas_min.x;

    // Normalize click position
    const float click_x_norm = (click_pos.x - canvas_min.x) / canvas_width;

    // If cursor A active, move it to click position
    if (cursor_a_active_) {
        cursor_a_pos_.x = click_x_norm;
    }
    // Otherwise if cursor B active, move it
    else if (cursor_b_active_) {
        cursor_b_pos_.x = click_x_norm;
    }
}

// Probe methods
float GraphScreen::GetSensorValueAtPosition(const SensorState &sensor,
                                            float normalized_pos) const {
    if (sensor.history.empty()) {
        return 0.0f;
    }

    // Calculate visible samples (same calculation as rendering)
    const size_t history_size = sensor.history.size();
    const size_t visible_samples =
        static_cast<size_t>(time_window_secs_ * SAMPLE_RATE);
    const size_t samples_to_render = std::min(visible_samples, history_size);
    const size_t start_idx =
        history_size > samples_to_render ? history_size - samples_to_render : 0;

    // Map normalized position (0-1) to index in visible history
    const float exact_idx =
        start_idx + normalized_pos * (samples_to_render - 1);
    const size_t idx_low = static_cast<size_t>(exact_idx);
    const size_t idx_high = std::min(idx_low + 1, history_size - 1);

    // Linear interpolation between samples
    const float frac = exact_idx - idx_low;
    const float val_low = sensor.history[idx_low];
    const float val_high = sensor.history[idx_high];

    return val_low + (val_high - val_low) * frac; // Lerp manual
}

void GraphScreen::RenderCursorValueBox(ImDrawList *draw_list,
                                       const ImVec2 &canvas_min,
                                       const ImVec2 &canvas_max,
                                       float normalized_pos, float cursor_x,
                                       const ImVec4 &color, const char *label) {
    auto &theme = ThemeManager::Instance();

    // Calculate values of all sensors at this position
    std::vector<std::pair<std::string, float>> sensor_values;
    for (size_t i = 0; i < graphSensorIndices.size(); ++i) {
        const int sensor_idx = graphSensorIndices[i];
        if (sensor_idx >= 0 &&
            sensor_idx < static_cast<int>(simulatedSensors.size())) {
            const auto &sensor = simulatedSensors[sensor_idx];
            float value = GetSensorValueAtPosition(sensor, normalized_pos);
            sensor_values.push_back({sensor.name, value});
        }
    }

    if (sensor_values.empty())
        return;

    // Calculate box size
    constexpr float padding = 8.0f;
    constexpr float line_height = 20.0f;
    float max_text_width = 0.0f;

    for (const auto &[name, value] : sensor_values) {
        char text[128];
        snprintf(text, sizeof(text), "%s: %.2f", name.c_str(), value);
        max_text_width = std::max(max_text_width, ImGui::CalcTextSize(text).x);
    }

    const float box_width = max_text_width + 2.0f * padding;
    const float box_height = sensor_values.size() * line_height +
                             2.0f * padding + line_height; // Extra for header

    // Position box above cursor (or to side if doesn't fit)
    float box_y = canvas_min.y + 30.0f; // 30px below top
    ImVec2 box_min = ImVec2(cursor_x + 10.0f, box_y);
    ImVec2 box_max = ImVec2(box_min.x + box_width, box_min.y + box_height);

    // Adjust if off-screen
    if (box_max.x > canvas_max.x) {
        box_min.x = cursor_x - box_width - 10.0f;
        box_max.x = box_min.x + box_width;
    }

    // Semi-transparent background
    ImVec4 bg_color = theme.GetBackgroundColor();
    bg_color.w = 0.95f;
    draw_list->AddRectFilled(box_min, box_max,
                             ImGui::ColorConvertFloat4ToU32(bg_color), 4.0f);

    // Border with cursor color
    draw_list->AddRect(box_min, box_max, ImGui::ColorConvertFloat4ToU32(color),
                       4.0f, 0, 2.0f);

    // Header
    char header[16];
    snprintf(header, sizeof(header), "Cursor %s", label);
    const ImVec2 header_pos = ImVec2(box_min.x + padding, box_min.y + padding);
    draw_list->AddText(header_pos, ImGui::ColorConvertFloat4ToU32(color),
                       header);

    // Render values
    for (size_t i = 0; i < sensor_values.size(); ++i) {
        const auto &[name, value] = sensor_values[i];
        char text[128];
        snprintf(text, sizeof(text), "%s: %.2f", name.c_str(), value);

        const ImVec2 text_pos =
            ImVec2(box_min.x + padding,
                   box_min.y + padding + line_height + i * line_height);

        // Use cached graph color for sensor name
        const ImU32 text_color =
            cached_graph_colors_[i % cached_graph_colors_.size()];
        draw_list->AddText(text_pos, text_color, text);
    }
}

void GraphScreen::RenderCursorValuesTable(ImDrawList *draw_list,
                                          const ImVec2 &canvas_min,
                                          const ImVec2 &canvas_max) {
    // Only render if at least one cursor active
    if (!cursor_a_active_ && !cursor_b_active_) {
        return;
    }

    auto &theme = ThemeManager::Instance();

    // Prepare data
    struct SensorRow {
        std::string name;
        float value_a;
        float value_b;
        float delta;
    };

    std::vector<SensorRow> rows;
    for (size_t i = 0; i < graphSensorIndices.size(); ++i) {
        const int sensor_idx = graphSensorIndices[i];
        if (sensor_idx < 0 ||
            sensor_idx >= static_cast<int>(simulatedSensors.size())) {
            continue;
        }

        const auto &sensor = simulatedSensors[sensor_idx];
        SensorRow row;
        row.name = sensor.name;
        row.value_a = cursor_a_active_
                          ? GetSensorValueAtPosition(sensor, cursor_a_pos_.x)
                          : 0.0f;
        row.value_b = cursor_b_active_
                          ? GetSensorValueAtPosition(sensor, cursor_b_pos_.x)
                          : 0.0f;
        row.delta = row.value_b - row.value_a;

        rows.push_back(row);
    }

    if (rows.empty())
        return;

    // Calculate table dimensions
    constexpr float padding = 8.0f;
    constexpr float row_height = 22.0f;
    constexpr float header_height = 26.0f;
    constexpr float col_width = 100.0f;

    const float num_cols = (cursor_a_active_ && cursor_b_active_) ? 4.0f : 2.0f;
    const ImVec2 table_size(col_width * num_cols + padding * 2.0f,
                            header_height + rows.size() * row_height +
                                padding * 2.0f);

    // I.12: Dynamic positioning - try right, then left, then overlay
    constexpr float margin = 10.0f;
    const ImVec2 vp_size = ImGui::GetIO().DisplaySize;

    ImVec2 table_pos;
    if (canvas_max.x + margin + table_size.x <= vp_size.x) {
        // Try right of canvas
        table_pos = ImVec2(canvas_max.x + margin, canvas_min.y);
    } else if (canvas_min.x - margin - table_size.x >= 0) {
        // Try left of canvas
        table_pos = ImVec2(canvas_min.x - margin - table_size.x, canvas_min.y);
    } else {
        // Overlay at bottom-right of canvas
        table_pos = ImVec2(canvas_max.x - table_size.x - margin,
                           canvas_max.y - table_size.y - margin);
    }

    const ImVec2 table_min = table_pos;
    const ImVec2 table_max(table_min.x + table_size.x,
                           table_min.y + table_size.y);

    // Background
    ImVec4 bg_color = theme.GetBackgroundColor();
    bg_color.w = 0.92f;
    draw_list->AddRectFilled(table_min, table_max,
                             ImGui::ColorConvertFloat4ToU32(bg_color), 4.0f);

    // Border
    draw_list->AddRect(table_min, table_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       4.0f, 0, 1.5f);

    // Header
    float x_offset = table_min.x + padding;
    const float y_header = table_min.y + padding;

    draw_list->AddText(ImVec2(x_offset, y_header),
                       ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                       "Sensor");
    x_offset += col_width;

    if (cursor_a_active_) {
        draw_list->AddText(
            ImVec2(x_offset, y_header),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.6f, 1.0f, 1.0f)),
            "A");
        x_offset += col_width;
    }

    if (cursor_b_active_) {
        draw_list->AddText(
            ImVec2(x_offset, y_header),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 1.0f, 0.6f, 1.0f)),
            "B");
        x_offset += col_width;
    }

    if (cursor_a_active_ && cursor_b_active_) {
        draw_list->AddText(
            ImVec2(x_offset, y_header),
            ImGui::ColorConvertFloat4ToU32(theme.GetAccentColor()),
            "\u0394"); // Delta symbol
    }

    // Separator
    const float sep_y = y_header + header_height - 4.0f;
    draw_list->AddLine(ImVec2(table_min.x + padding, sep_y),
                       ImVec2(table_max.x - padding, sep_y),
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()));

    // Rows
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto &row = rows[i];
        const float y_row = y_header + header_height + i * row_height;

        x_offset = table_min.x + padding;

        // Sensor name (with cached graph color)
        const ImU32 sensor_color =
            cached_graph_colors_[i % cached_graph_colors_.size()];
        draw_list->AddText(ImVec2(x_offset, y_row), sensor_color,
                           row.name.c_str());
        x_offset += col_width;

        // Value A
        if (cursor_a_active_) {
            char text_a[32];
            snprintf(text_a, sizeof(text_a), "%.2f", row.value_a);
            draw_list->AddText(
                ImVec2(x_offset, y_row),
                ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()), text_a);
            x_offset += col_width;
        }

        // Value B
        if (cursor_b_active_) {
            char text_b[32];
            snprintf(text_b, sizeof(text_b), "%.2f", row.value_b);
            draw_list->AddText(
                ImVec2(x_offset, y_row),
                ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()), text_b);
            x_offset += col_width;
        }

        // Delta
        if (cursor_a_active_ && cursor_b_active_) {
            char text_delta[32];
            snprintf(text_delta, sizeof(text_delta), "%.2f", row.delta);

            // Green if positive, red if negative
            ImVec4 delta_color = row.delta >= 0
                                     ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)  // Green
                                     : ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red

            draw_list->AddText(ImVec2(x_offset, y_row),
                               ImGui::ColorConvertFloat4ToU32(delta_color),
                               text_delta);
        }
    }
}

// Logging methods
bool GraphScreen::StartLogging() {
    // E.25: Early return on precondition failure
    if (graphSensorIndices.empty()) {
        return false;
    }

    // Create logs directory
    std::string base_dir = FileIO::GetUserDataDirectory();
    FileIO::CreateDirectoryIfNotExists(base_dir);

    std::string logs_dir = base_dir + "/logs";
    FileIO::CreateDirectoryIfNotExists(logs_dir);

    // Generate timestamped filename
    std::string filename =
        FileIO::GenerateTimestampedFilename("graph_log", ".csv");
    current_log_path_ = logs_dir + "/" + filename;

    // Open file
    logging_file_.open(current_log_path_, std::ios::out | std::ios::trunc);
    if (!logging_file_.is_open()) {
        // E.3: Clean up partial state on error
        current_log_path_.clear();
        return false;
    }

    // Write CSV header (only selected sensors)
    WriteLogHeader(logging_file_);

    // E.3: Check if write failed
    if (logging_file_.fail()) {
        logging_file_.close();
        current_log_path_.clear();
        return false;
    }

    // E.25: Only set flag after all operations succeed
    logging_active_ = true;
    logging_start_time_ = glfwGetTime();

    return true;
}

void GraphScreen::StopLogging() {
    if (logging_file_.is_open()) {
        logging_file_.close();
    }
    logging_active_ = false;
    current_log_path_.clear();
}

void GraphScreen::WriteLogSample() {
    if (!logging_active_ || !logging_file_.is_open()) {
        return;
    }

    double elapsed = glfwGetTime() - logging_start_time_;
    FileIO::WriteCSVRow(logging_file_, elapsed, simulatedSensors,
                        &graphSensorIndices);
}

void GraphScreen::WriteLogHeader(std::ofstream &file) {
    FileIO::WriteCSVHeader(file, simulatedSensors, &graphSensorIndices);
}
