#include "TouchGestureHandler.h"
#include <GLFW/glfw3.h>
#include <cmath>

void TouchGestureHandler::Update() {
    const ImGuiIO &io = ImGui::GetIO();
    const bool mouse_down = io.MouseDown[0]; // Left mouse button (or touch)
    const ImVec2 mouse_pos = io.MousePos;
    const double current_time = glfwGetTime();

    // Reset gesture from previous frame
    last_gesture_.type = GestureType::NONE;

    if (mouse_down && !is_touching_) {
        // Touch started
        is_touching_ = true;
        touch_start_pos_ = mouse_pos;
        touch_start_time_ = current_time;

    } else if (!mouse_down && is_touching_) {
        // Touch ended - analyze gesture
        is_touching_ = false;

        const float duration =
            static_cast<float>(current_time - touch_start_time_);
        const ImVec2 delta = ImVec2(mouse_pos.x - touch_start_pos_.x,
                                    mouse_pos.y - touch_start_pos_.y);
        const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        // Populate event
        last_gesture_.start_pos = touch_start_pos_;
        last_gesture_.end_pos = mouse_pos;
        last_gesture_.duration = duration;

        // Detect gesture type

        // 1. Long press (if still within tap area)
        if (duration >= long_press_threshold_ && distance < tap_max_movement_) {
            last_gesture_.type = GestureType::LONG_PRESS;
        }
        // 2. Swipe (significant movement)
        else if (distance >= swipe_threshold_) {
            // Determine swipe direction (horizontal vs vertical)
            if (std::abs(delta.x) > std::abs(delta.y)) {
                // Horizontal swipe
                last_gesture_.type = (delta.x > 0) ? GestureType::SWIPE_RIGHT
                                                   : GestureType::SWIPE_LEFT;
            } else {
                // Vertical swipe
                last_gesture_.type = (delta.y > 0) ? GestureType::SWIPE_DOWN
                                                   : GestureType::SWIPE_UP;
            }
        }
        // 3. Tap (quick touch with minimal movement)
        else if (duration < tap_max_duration_ && distance < tap_max_movement_) {
            last_gesture_.type = GestureType::TAP;
        }
    } else if (mouse_down && is_touching_) {
        // Touch is ongoing - check for long press detection during touch
        const float duration =
            static_cast<float>(current_time - touch_start_time_);
        const ImVec2 delta = ImVec2(mouse_pos.x - touch_start_pos_.x,
                                    mouse_pos.y - touch_start_pos_.y);
        const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        // Trigger long press when threshold is reached (while finger is still
        // down)
        if (duration >= long_press_threshold_ && distance < tap_max_movement_) {
            // Only trigger once
            if (last_gesture_.type != GestureType::LONG_PRESS) {
                last_gesture_.type = GestureType::LONG_PRESS;
                last_gesture_.start_pos = touch_start_pos_;
                last_gesture_.end_pos = mouse_pos;
                last_gesture_.duration = duration;
            }
        }
    }
}

bool TouchGestureHandler::IsGesture(GestureType type) const {
    return last_gesture_.type == type;
}

void TouchGestureHandler::ClearGesture() {
    last_gesture_.type = GestureType::NONE;
}
