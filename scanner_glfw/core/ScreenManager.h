#pragma once

#include "../app_data.h"
#include <memory>

class BaseScreen;
class LiveScreen;
class GraphScreen;
class DashScreen;
class DTCScreen;
class LogsScreen;

class ScreenManager {
  public:
    static ScreenManager &Instance();

    void Initialize();
    void Shutdown();

    void Update(float delta_time);
    void Render();

    void SetCurrentScreen(Screen screen);
    Screen GetCurrentScreen() const { return current_screen_id_; }
    BaseScreen *GetScreen(Screen screen) const;

    ScreenManager(const ScreenManager &) = delete;
    ScreenManager &operator=(const ScreenManager &) = delete;
    ScreenManager(ScreenManager &&) = delete;
    ScreenManager &operator=(ScreenManager &&) = delete;

  private:
    ScreenManager();
    ~ScreenManager();

    void TransitionToScreen(Screen screen);

    std::unique_ptr<LiveScreen> live_screen_;
    std::unique_ptr<GraphScreen> graph_screen_;
    std::unique_ptr<DashScreen> dash_screen_;
    std::unique_ptr<DTCScreen> dtc_screen_;
    std::unique_ptr<LogsScreen> logs_screen_;

    BaseScreen *current_screen_ = nullptr;
    Screen current_screen_id_ = Screen::DASH;
    bool initialized_ = false;
};
