#pragma once

#include "../core/TouchGestureHandler.h"

// Abstract base class for all screens
class BaseScreen {
  public:
    virtual ~BaseScreen() = default;

    // Lifecycle methods (pure virtual - must be implemented)
    virtual void OnEnter() = 0; // Called when screen becomes active
    virtual void OnExit() = 0;  // Called when screen becomes inactive
    virtual void Update(float delta_time) = 0; // Called every frame
    virtual void Render() = 0;                 // Called every frame to draw UI

    // Optional overrides
    virtual void OnResize(int width, int height) {}
    virtual bool HandleGesture(const GestureEvent &event) { return false; }

    // State queries
    bool IsActive() const { return is_active_; }
    float GetFadeAlpha() const { return fade_alpha_; }

  protected:
    bool is_active_ = false;
    float fade_alpha_ = 0.0f;

    // Helper to set active state (call from OnEnter/OnExit)
    void SetActive(bool active) { is_active_ = active; }
    void SetFadeAlpha(float alpha) { fade_alpha_ = alpha; }
};
