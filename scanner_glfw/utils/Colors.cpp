#include "Colors.h"
#include "../core/AnimationSystem.h"
#include "../core/ThemeManager.h"

ImVec4 Colors::GetColorForValue(double value,
                                const std::vector<ColorZone> &zones) {
    // Default: use theme primary color
    if (zones.empty()) {
        return ThemeManager::Instance().GetPrimaryColor();
    }

    // Single zone: use that color
    if (zones.size() == 1) {
        return zones[0].color;
    }

    // Below first zone: use first color
    if (value < zones[0].threshold) {
        return zones[0].color;
    }

    // Above last zone: use last color
    if (value >= zones.back().threshold) {
        return zones.back().color;
    }

    // Find adjacent zones and interpolate
    for (size_t i = 0; i < zones.size() - 1; ++i) {
        if (value >= zones[i].threshold && value < zones[i + 1].threshold) {
            // Calculate interpolation factor
            const float range =
                static_cast<float>(zones[i + 1].threshold - zones[i].threshold);
            const float t =
                (range > 0.0001f)
                    ? static_cast<float>((value - zones[i].threshold) / range)
                    : 0.0f;

            // Smooth interpolation between colors
            return AnimationSystem::LerpColor(zones[i].color,
                                              zones[i + 1].color, t);
        }
    }

    // Fallback: last color
    return zones.back().color;
}
