#pragma once

#include "imgui.h"
#include <vector>

namespace Colors {

// Dark Theme (default)
namespace Dark {
constexpr ImVec4 Background = ImVec4(0.102f, 0.114f, 0.137f, 1.0f); // #1A1D23
constexpr ImVec4 Text = ImVec4(0.898f, 0.906f, 0.922f, 1.0f);       // #E5E7EB
constexpr ImVec4 Primary = ImVec4(0.231f, 0.510f, 0.965f, 1.0f);    // #3B82F6
constexpr ImVec4 Secondary = ImVec4(0.314f, 0.353f, 0.431f, 1.0f);  // #505A6E
constexpr ImVec4 Accent = ImVec4(0.545f, 0.361f, 0.965f, 1.0f);     // #8B5CF6
constexpr ImVec4 Success = ImVec4(0.063f, 0.725f, 0.506f, 1.0f);    // #10B981
constexpr ImVec4 Warning = ImVec4(0.961f, 0.620f, 0.043f, 1.0f);    // #F59E0B
constexpr ImVec4 Error = ImVec4(0.937f, 0.267f, 0.267f, 1.0f);      // #EF4444
constexpr ImVec4 Border = ImVec4(0.235f, 0.263f, 0.314f, 1.0f);     // #3C4350
constexpr ImVec4 Hover = ImVec4(0.157f, 0.176f, 0.212f, 1.0f);      // #282D36
} // namespace Dark

// Light Theme
namespace Light {
constexpr ImVec4 Background = ImVec4(0.976f, 0.980f, 0.984f, 1.0f); // #F9FAFB
constexpr ImVec4 Text = ImVec4(0.122f, 0.161f, 0.216f, 1.0f);       // #1F2937
constexpr ImVec4 Primary = ImVec4(0.145f, 0.388f, 0.922f, 1.0f);    // #2563EB
constexpr ImVec4 Secondary = ImVec4(0.557f, 0.616f, 0.690f, 1.0f);  // #8E9DB0
constexpr ImVec4 Accent = ImVec4(0.486f, 0.227f, 0.929f, 1.0f);     // #7C3AED
constexpr ImVec4 Success = ImVec4(0.020f, 0.588f, 0.412f, 1.0f);    // #059669
constexpr ImVec4 Warning = ImVec4(0.851f, 0.467f, 0.024f, 1.0f);    // #D97706
constexpr ImVec4 Error = ImVec4(0.863f, 0.149f, 0.149f, 1.0f);      // #DC2626
constexpr ImVec4 Border = ImVec4(0.831f, 0.851f, 0.871f, 1.0f);     // #D4D9DF
constexpr ImVec4 Hover = ImVec4(0.941f, 0.953f, 0.965f, 1.0f);      // #F0F3F6
} // namespace Light

// Sensor Status Colors (common to both themes)
namespace Status {
constexpr ImVec4 OK = ImVec4(0.063f, 0.725f, 0.506f, 1.0f); // #10B981 (Green)
constexpr ImVec4 WARN =
    ImVec4(0.961f, 0.620f, 0.043f, 1.0f); // #F59E0B (Orange)
constexpr ImVec4 CRITICAL =
    ImVec4(0.937f, 0.267f, 0.267f, 1.0f);                      // #EF4444 (Red)
constexpr ImVec4 STALE = ImVec4(0.627f, 0.667f, 0.706f, 1.0f); // #A0AAB4 (Gray)
} // namespace Status

// Graph Colors (for multi-signal plotting)
constexpr ImVec4 GraphColors[] = {
    ImVec4(0.231f, 0.510f, 0.965f, 1.0f), // #3B82F6 Blue
    ImVec4(0.937f, 0.267f, 0.267f, 1.0f), // #EF4444 Red
    ImVec4(0.063f, 0.725f, 0.506f, 1.0f), // #10B981 Green
    ImVec4(0.961f, 0.620f, 0.043f, 1.0f), // #F59E0B Orange
    ImVec4(0.545f, 0.361f, 0.965f, 1.0f), // #8B5CF6 Purple
    ImVec4(0.236f, 0.678f, 0.843f, 1.0f), // #3CADD7 Cyan
};

constexpr int GraphColorsCount = sizeof(GraphColors) / sizeof(GraphColors[0]);

// Color zone for threshold-based coloring
struct ColorZone {
    double threshold; // Value at which color changes
    ImVec4 color;     // Color for this zone

    ColorZone(double t, ImVec4 c) : threshold(t), color(c) {}
    ColorZone(double t, float r, float g, float b, float a = 1.0f)
        : threshold(t), color(r, g, b, a) {}
};

// Get color for a value based on threshold zones (with smooth interpolation)
ImVec4 GetColorForValue(double value, const std::vector<ColorZone> &zones);

} // namespace Colors
