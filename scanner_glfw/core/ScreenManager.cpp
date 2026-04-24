#include "ScreenManager.h"
#include "../app_data.h"
#include "../core/ECUBackend.h"
#include "../screens/BaseScreen.h"
#include "../screens/DTCScreen.h"
#include "../screens/DashScreen.h"
#include "../screens/GraphScreen.h"
#include "../screens/LiveScreen.h"
#include "../screens/LogsScreen.h"

ScreenManager &ScreenManager::Instance() {
    static ScreenManager instance;
    return instance;
}

ScreenManager::ScreenManager() = default;

ScreenManager::~ScreenManager() = default;

void ScreenManager::Initialize() {
    if (initialized_) {
        return;
    }

    live_screen_ = std::make_unique<LiveScreen>();
    graph_screen_ = std::make_unique<GraphScreen>();
    dash_screen_ = std::make_unique<DashScreen>();
    dtc_screen_ = std::make_unique<DTCScreen>();
    logs_screen_ = std::make_unique<LogsScreen>();

    initialized_ = true;
    TransitionToScreen(::GetCurrentScreen());
}

void ScreenManager::Shutdown() {
    if (!initialized_) {
        return;
    }

    if (current_screen_) {
        current_screen_->OnExit();
    }

    current_screen_ = nullptr;
    live_screen_.reset();
    graph_screen_.reset();
    dash_screen_.reset();
    dtc_screen_.reset();
    logs_screen_.reset();
    initialized_ = false;
}

void ScreenManager::Update(float delta_time) {
    if (!initialized_) {
        return;
    }

    const Screen desired = ::GetCurrentScreen();
    if (desired != current_screen_id_ || current_screen_ == nullptr) {
        TransitionToScreen(desired);
    }

    if (current_screen_) {
        current_screen_->Update(delta_time);
    }
}

void ScreenManager::Render() {
    if (!initialized_) {
        return;
    }

    if (current_screen_) {
        current_screen_->Render();
    }
}

void ScreenManager::SetCurrentScreen(Screen screen) {
    ::SetCurrentScreen(screen);
    if (!initialized_) {
        current_screen_id_ = screen;
        return;
    }

    TransitionToScreen(screen);
}

BaseScreen *ScreenManager::GetScreen(Screen screen) const {
    switch (screen) {
    case Screen::LIVE:
        return live_screen_.get();
    case Screen::GRAPH:
        return graph_screen_.get();
    case Screen::DASH:
        return dash_screen_.get();
    case Screen::DTC:
        return dtc_screen_.get();
    case Screen::LOGS:
        return logs_screen_.get();
    }

    return nullptr;
}

void ScreenManager::TransitionToScreen(Screen screen) {
    BaseScreen *target = GetScreen(screen);
    if (!target) {
        return;
    }

    if (target == current_screen_) {
        current_screen_id_ = screen;
        return;
    }

    if (current_screen_) {
        current_screen_->OnExit();
    }

    // Pause ECU polling and trim histories on non-live screens to save
    // resources
    auto &backend = ECUBackend::Instance();
    const bool needs_live_data =
        (screen == Screen::LIVE || screen == Screen::GRAPH ||
         screen == Screen::DASH);
    backend.SetPaused(!needs_live_data);
    if (!needs_live_data) {
        for (auto &sensor : simulatedSensors) {
            sensor.SetMaxHistory(60); // Keep minimal history for sparklines
        }
    } else {
        for (auto &sensor : simulatedSensors) {
            sensor.SetMaxHistory(300); // Restore full history
        }
    }

    current_screen_ = target;
    current_screen_id_ = screen;
    current_screen_->OnEnter();
}
