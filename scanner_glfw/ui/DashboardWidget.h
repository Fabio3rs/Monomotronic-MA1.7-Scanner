#pragma once

#include "../app_data.h"
#include "imgui.h"

// Base class for dashboard widgets
class DashboardWidgetRenderer {
  public:
    virtual ~DashboardWidgetRenderer() = default;

    // Render widget with given sensor data
    // widget_size: Size of the widget area
    virtual void Render(const SensorState &sensor,
                        const ImVec2 &widget_size) = 0;

    // Optional: Handle widget-specific interactions
    virtual bool HandleClick(const ImVec2 &click_pos, const ImVec2 &widget_min,
                             const ImVec2 &widget_max) {
        return false;
    }
};

// Numeric Widget: Value + Sparkline + Progress Bar
class NumericWidgetRenderer : public DashboardWidgetRenderer {
  public:
    void Render(const SensorState &sensor, const ImVec2 &widget_size) override;
};

// Gauge Widget: Arc dial with tick marks
class GaugeWidgetRenderer : public DashboardWidgetRenderer {
  public:
    void Render(const SensorState &sensor, const ImVec2 &widget_size) override;
};

// Graph Widget: Line chart with history
class GraphWidgetRenderer : public DashboardWidgetRenderer {
  public:
    void Render(const SensorState &sensor, const ImVec2 &widget_size) override;
};

// BarGraph Widget: Retro 80s/90s vertical bars (like digital tachometers)
class BarGraphWidgetRenderer : public DashboardWidgetRenderer {
  public:
    void Render(const SensorState &sensor, const ImVec2 &widget_size) override;
};
