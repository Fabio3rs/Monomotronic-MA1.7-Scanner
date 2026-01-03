#pragma once

#include "../ui/Modal.h"
#include "BaseScreen.h"
#include <array>
#include <fstream>
#include <string>
#include <vector>

// Forward declarations
struct SensorState;

class GraphScreen : public BaseScreen {
  public:
    GraphScreen();
    ~GraphScreen() override = default;

    // BaseScreen interface
    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

  private:
    friend class GraphScreenTestAccess;

    // UI state
    bool show_signal_selector_ = false;
    bool graph_frozen_ = false;
    bool logging_active_ = false;

    // Logging state
    std::ofstream logging_file_;
    std::string current_log_path_;
    double logging_start_time_ = 0.0;

    // E.2: Error toast state for user feedback
    std::string last_error_message_;
    bool show_error_toast_ = false;
    float error_toast_timer_ = 0.0f;

    // Graph state
    float time_window_secs_ = 30.0f;
    static constexpr float SAMPLE_RATE =
        60.0f; // Samples per second (V-Sync locked)
    std::vector<bool> auto_scale_y_;
    std::vector<float> last_plot_min_y_;
    std::vector<float> last_plot_max_y_;

    // Per.10: Performance - Cache color conversions to avoid per-frame overhead
    std::array<ImU32, 6> cached_graph_colors_;

    // Cursor state
    bool cursor_a_active_ = false;
    ImVec2 cursor_a_pos_{0.0f, 0.0f}; // ES.20: Initialize all members
    bool cursor_b_active_ = false;
    ImVec2 cursor_b_pos_{0.0f, 0.0f}; // ES.20: Initialize all members
    // Per.10: Visual feedback state
    bool dragging_cursor_a_ = false;
    bool dragging_cursor_b_ = false;

    // Hover probe state
    bool hover_probe_active_ = false;
    ImVec2 hover_probe_pos_{0.0f,
                            0.0f}; // ES.20: Normalized position (0.0 - 1.0)

    // Components
    Modal signal_modal_;

    // Helper methods
    void LoadStateFromManager();
    void SyncPinnedSensorsToGraph();
    void InitializeGraphMetadata();
    void CacheColorConversions();

    void RenderTopControls();
    void RenderGraphCanvas();
    void RenderLegend(ImDrawList *draw_list, const ImVec2 &canvas_min,
                      const ImVec2 &canvas_max);
    void RenderCursors(ImDrawList *draw_list, const ImVec2 &canvas_min,
                       const ImVec2 &canvas_max);
    void RenderDeltaBox(ImDrawList *draw_list, const ImVec2 &canvas_min,
                        const ImVec2 &canvas_max);
    void RenderSignalSelector();
    void RenderErrorToast(); // E.2: Display error messages to user

    void UpdateAutoScale(int signal_idx);
    void ApplySubscriptions();
    void HandleCanvasClick(const ImVec2 &click_pos, const ImVec2 &canvas_min,
                           const ImVec2 &canvas_max);

    // Probe methods
    float GetSensorValueAtPosition(const SensorState &sensor,
                                   float normalized_pos) const;
    void RenderHoverProbe(ImDrawList *draw_list, const ImVec2 &canvas_min,
                          const ImVec2 &canvas_max,
                          const ImVec2 &mouse_pos); // Per.10: Visual feedback
    void RenderCursorValueBox(ImDrawList *draw_list, const ImVec2 &canvas_min,
                              const ImVec2 &canvas_max, float normalized_pos,
                              float cursor_x, const ImVec4 &color,
                              const char *label);
    void RenderCursorValuesTable(ImDrawList *draw_list,
                                 const ImVec2 &canvas_min,
                                 const ImVec2 &canvas_max);

    // Logging methods
    bool StartLogging();
    void StopLogging();
    void WriteLogSample();
    void WriteLogHeader(std::ofstream &file);
};
