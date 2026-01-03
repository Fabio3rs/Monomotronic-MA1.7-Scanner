#pragma once

#include "imgui.h"
#include <cmath>
#include <map>
#include <string>

class AnimationSystem {
  public:
    // Singleton access
    static AnimationSystem &Instance() {
        static AnimationSystem instance;
        return instance;
    }

    // Update animations (call every frame)
    void Update(float delta_time);

    // Fade animation (0.0 to 1.0)
    float GetFade(const std::string &id, float duration_sec = 0.3f);
    void TriggerFade(const std::string &id);
    void ResetFade(const std::string &id);

    // Interpolation utilities
    static float Lerp(float a, float b, float t);
    static ImVec4 LerpColor(const ImVec4 &a, const ImVec4 &b, float t);

    // Pulsing effect (for critical alerts) - returns value between 0.0 and 1.0
    float GetPulse(float frequency = 2.0f);

    // Smooth damping (for smooth camera-like following)
    static float SmoothDamp(float current, float target, float &velocity,
                            float smooth_time, float max_speed,
                            float delta_time);

    // Delete copy/move constructors (singleton)
    AnimationSystem(const AnimationSystem &) = delete;
    AnimationSystem &operator=(const AnimationSystem &) = delete;
    AnimationSystem(AnimationSystem &&) = delete;
    AnimationSystem &operator=(AnimationSystem &&) = delete;

  private:
    AnimationSystem();
    ~AnimationSystem() = default;

    struct FadeState {
        float value = 0.0f;
        float target = 1.0f;
        float duration = 0.3f;
        float elapsed = 0.0f;
        bool active = false;
    };

    std::map<std::string, FadeState> fades_;
    double start_time_ = 0.0;
};
