#pragma once

#include "../../app_data.h"
#include "imgui.h"
#include <vector>

namespace UI::Components {

struct TableStatus {
    const char *label;
    ImU32 color;
};

class ECUStatusIndicator {
  public:
    static ImU32 GetStatusColor(bool connected);
    static const char *GetStatusText(bool connected);
    static void Render(bool connected);
};

class SignalStrengthBars {
  public:
    static float CalculateSignalStrength(float latency_ms, float error_rate);
    static ImU32 GetLatencyColor(float latency_ms);
    static void RenderBars(float latency_ms, float error_rate);
    static void RenderLatency(float latency_ms);
};

class TableStatusDisplay {
  public:
    static TableStatus GetStatus(int active_table);
    static void Render(int active_table);
};

class SensorFreshnessDisplay {
  public:
    static double CalculateAgeSeconds(const std::vector<SensorState> &sensors,
                                      double now_sec);
    static ImU32 GetAgeColor(double age_sec);
    static void Render(const std::vector<SensorState> &sensors, double now_sec);
};

class TopBarTimeDisplay {
  public:
    static void Render();
};

class SessionManager {
  public:
    static void Render(float button_width, float button_height);
};

} // namespace UI::Components
