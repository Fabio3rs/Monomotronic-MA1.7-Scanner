#pragma once

#include "imgui.h"

class TopBar {
  public:
    TopBar() = default;
    ~TopBar() = default;

    // Render the top bar
    void Render(float bar_height);

    // Configuration
    void SetConnectionStatus(bool connected) { ecu_connected_ = connected; }
    void SetLatency(float latency_ms) { latency_ms_ = latency_ms; }
    void SetErrorRate(float error_rate) { error_rate_ = error_rate; }

    bool IsConnected() const { return ecu_connected_; }

  private:
    // Connection state
    bool ecu_connected_ = true;
    float latency_ms_ = 16.0f;
    float error_rate_ = 0.0f;

    // Window control state
    bool is_fullscreen_ = false;
    int windowed_x_ = 0;
    int windowed_y_ = 0;
    int windowed_width_ = 1024;
    int windowed_height_ = 600;

    // Helper methods
    void RenderConnectionStatus();
    void RenderSignalBars();
    void RenderSimulationBanner();
    void RenderWindowControls();
    void RenderThemeToggle();
    void RenderTime();

    void ToggleFullscreen();
    int CalculateSignalBars() const;
};
