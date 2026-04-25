#pragma once

#include "../app_data.h"
#include "imgui.h"
#include <functional>
#include <string>
#include <vector>

class SensorTable {
  public:
    SensorTable() = default;
    ~SensorTable() = default;

    // Render the sensor table
    // Returns: index of sensor if detail button clicked, -1 otherwise
    int Render(const std::vector<int> &sensor_indices,
               const std::vector<SensorState> &all_sensors, float table_height);

    // Configuration
    void SetFilter(const char *filter) { filter_text_ = filter ? filter : ""; }
    void SetShowPinButtons(bool show) { show_pin_buttons_ = show; }
    void SetShowSparklines(bool show) { show_sparklines_ = show; }

    // Callbacks for interactions
    using PinCallback = std::function<void(int sensor_idx, bool pinned)>;
    void SetPinCallback(PinCallback callback) { pin_callback_ = callback; }

    // Check if sensor is pinned (via callback)
    using IsPinnedCallback = std::function<bool(int sensor_idx)>;
    void SetIsPinnedCallback(IsPinnedCallback callback) {
        is_pinned_callback_ = callback;
    }

  private:
    std::string filter_text_;
    bool show_pin_buttons_ = true;
    bool show_sparklines_ = true;

    // ES.20: Always initialize member variables
    PinCallback pin_callback_ = nullptr;
    IsPinnedCallback is_pinned_callback_ = nullptr;

    // Helper methods
    bool MatchesFilter(const SensorState &sensor) const;
    void RenderStatusCircle(SensorStatus status);
    void RenderSparkline(const SensorState &sensor, float width, float height);
};
