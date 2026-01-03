#pragma once

#include "BaseScreen.h"
#include <chrono>
#include <string>

class LogsScreen : public BaseScreen {
  public:
    LogsScreen();
    ~LogsScreen() override = default;

    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

  private:
    void RenderTopControls();
    void RenderLogContent();
    void RefreshLogs();
    void ClearLogs();
    void ExportLogs();

    // State
    std::string log_buffer_;
    std::chrono::steady_clock::time_point last_refresh_;
    bool paused_ = false;
    bool auto_scroll_ = true;
    bool show_export_success_ = false;
    float export_message_timer_ = 0.0f;

    static constexpr auto REFRESH_INTERVAL = std::chrono::milliseconds(500);
};
