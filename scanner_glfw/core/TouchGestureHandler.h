#pragma once

#include "imgui.h"

enum class GestureType {
    NONE,
    TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN
};

struct GestureEvent {
    GestureType type = GestureType::NONE;
    ImVec2 start_pos;
    ImVec2 end_pos;
    float duration = 0.0f;
};

class TouchGestureHandler {
  public:
    // Singleton access
    static TouchGestureHandler &Instance() {
        static TouchGestureHandler instance;
        return instance;
    }

    // Update gesture detection (call every frame)
    void Update();

    // Check if a specific gesture occurred this frame
    bool IsGesture(GestureType type) const;

    // Get the last detected gesture
    GestureEvent GetLastGesture() const { return last_gesture_; }

    // Clear the last gesture (useful after handling)
    void ClearGesture();

    // Configuration
    void SetLongPressThreshold(float seconds) {
        long_press_threshold_ = seconds;
    }
    void SetSwipeThreshold(float pixels) { swipe_threshold_ = pixels; }

    float GetLongPressThreshold() const { return long_press_threshold_; }
    float GetSwipeThreshold() const { return swipe_threshold_; }

    // Delete copy/move constructors (singleton)
    TouchGestureHandler(const TouchGestureHandler &) = delete;
    TouchGestureHandler &operator=(const TouchGestureHandler &) = delete;
    TouchGestureHandler(TouchGestureHandler &&) = delete;
    TouchGestureHandler &operator=(TouchGestureHandler &&) = delete;

  private:
    TouchGestureHandler() = default;
    ~TouchGestureHandler() = default;

    // Touch state
    bool is_touching_ = false;
    ImVec2 touch_start_pos_;
    double touch_start_time_ = 0.0;

    // Last detected gesture
    GestureEvent last_gesture_;

    // Configuration thresholds
    float long_press_threshold_ = 0.5f; // seconds
    float swipe_threshold_ = 100.0f;    // pixels
    float tap_max_duration_ = 0.3f;     // seconds
    float tap_max_movement_ = 10.0f;    // pixels
};
