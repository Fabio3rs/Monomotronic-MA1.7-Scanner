#include "AnimationSystem.h"
#include <GLFW/glfw3.h>
#include <algorithm>

AnimationSystem::AnimationSystem() { start_time_ = glfwGetTime(); }

void AnimationSystem::Update(float delta_time) {
    // Update all fade animations
    for (auto &pair : fades_) {
        FadeState &fade = pair.second;

        if (fade.active) {
            fade.elapsed += delta_time;

            // Calculate progress (0.0 to 1.0)
            const float t = std::min(fade.elapsed / fade.duration, 1.0f);

            // Ease out cubic for smooth deceleration
            const float eased_t = 1.0f - std::pow(1.0f - t, 3.0f);

            fade.value = Lerp(0.0f, fade.target, eased_t);

            // Mark as inactive when complete
            if (t >= 1.0f) {
                fade.active = false;
                fade.value = fade.target;
            }
        }
    }
}

float AnimationSystem::GetFade(const std::string &id, float duration_sec) {
    auto it = fades_.find(id);

    if (it == fades_.end()) {
        // Create new fade animation
        FadeState fade;
        fade.value = 0.0f;
        fade.target = 1.0f;
        fade.duration = duration_sec;
        fade.elapsed = 0.0f;
        fade.active = true;

        fades_[id] = fade;
        return 0.0f;
    }

    return it->second.value;
}

void AnimationSystem::TriggerFade(const std::string &id) {
    auto it = fades_.find(id);

    if (it != fades_.end()) {
        it->second.elapsed = 0.0f;
        it->second.active = true;
    } else {
        // Create new fade
        FadeState fade;
        fade.value = 0.0f;
        fade.target = 1.0f;
        fade.duration = 0.3f;
        fade.elapsed = 0.0f;
        fade.active = true;

        fades_[id] = fade;
    }
}

void AnimationSystem::ResetFade(const std::string &id) {
    auto it = fades_.find(id);

    if (it != fades_.end()) {
        it->second.value = 0.0f;
        it->second.elapsed = 0.0f;
        it->second.active = false;
    }
}

float AnimationSystem::Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

ImVec4 AnimationSystem::LerpColor(const ImVec4 &a, const ImVec4 &b, float t) {
    return ImVec4(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t),
                  Lerp(a.w, b.w, t));
}

float AnimationSystem::GetPulse(float frequency) {
    const double current_time = glfwGetTime() - start_time_;
    const float phase = static_cast<float>(current_time) * frequency * 2.0f *
                        3.14159265f;        // 2*PI
    return (std::sin(phase) + 1.0f) * 0.5f; // Map from [-1,1] to [0,1]
}

float AnimationSystem::SmoothDamp(float current, float target, float &velocity,
                                  float smooth_time, float max_speed,
                                  float delta_time) {
    // Based on Game Programming Gems 4 Chapter 1.10
    smooth_time = std::max(0.0001f, smooth_time);
    const float omega = 2.0f / smooth_time;

    const float x = omega * delta_time;
    const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

    float change = current - target;
    const float original_to = target;

    // Clamp maximum speed
    const float max_change = max_speed * smooth_time;
    change = std::max(-max_change, std::min(change, max_change));
    target = current - change;

    const float temp = (velocity + omega * change) * delta_time;
    velocity = (velocity - omega * temp) * exp;

    float output = target + (change + temp) * exp;

    // Prevent overshooting
    if ((original_to > current) == (output > original_to)) {
        output = original_to;
        velocity = (output - original_to) / delta_time;
    }

    return output;
}
