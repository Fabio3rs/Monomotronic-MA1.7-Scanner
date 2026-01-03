#include "DashboardWidget.h"
#include "../core/ThemeManager.h"
#include "../utils/Colors.h"
#include "../utils/FontGuard.h" // C.21: RAII for font management
#include "../utils/Layout.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
bool GetDisplayRange(const SensorState &sensor, float &min_val,
                     float &max_val) {
    if (!std::isnan(sensor.displayMin) && !std::isnan(sensor.displayMax)) {
        min_val = static_cast<float>(sensor.displayMin);
        max_val = static_cast<float>(sensor.displayMax);
        return true;
    }
    if (!std::isnan(sensor.alertMin) && !std::isnan(sensor.alertMax)) {
        min_val = static_cast<float>(sensor.alertMin);
        max_val = static_cast<float>(sensor.alertMax);
        return true;
    }
    return false;
}
} // namespace

extern ImFont *font_large;
extern ImFont *font_huge;

//
// NumericWidgetRenderer
//

void NumericWidgetRenderer::Render(const SensorState &sensor,
                                   const ImVec2 &widget_size) {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 widget_min = ImGui::GetCursorScreenPos();
    const ImVec2 widget_max =
        ImVec2(widget_min.x + widget_size.x, widget_min.y + widget_size.y);

    // Draw widget background
    draw_list->AddRectFilled(
        widget_min, widget_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetHoverColor()), 6.0f);

    // Draw border
    draw_list->AddRect(widget_min, widget_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       6.0f, 0, 1.5f);

    // Padding
    const float padding = 10.0f;
    const ImVec2 content_min =
        ImVec2(widget_min.x + padding, widget_min.y + padding);

    // Sensor name at top
    draw_list->AddText(
        content_min, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.name.c_str());

    // Value (large font)
    char value_str[64];
    snprintf(value_str, sizeof(value_str), "%.1f", sensor.lastValue);

    // C.21: Use FontGuard for automatic PopFont (RAII)
    ImVec2 value_size;
    ImVec2 value_pos;
    {
        FontGuard font_guard(font_large);
        if (font_guard.IsValid()) {
            value_size = ImGui::CalcTextSize(value_str);
            value_pos =
                ImVec2(content_min.x, widget_min.y + widget_size.y * 0.30f);
            draw_list->AddText(
                font_large, 28.0f, value_pos,
                ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                value_str);
        }
    } // PopFont() called automatically

    // Unit (next to value)
    const ImVec2 unit_pos = ImVec2(value_pos.x + value_size.x + 8.0f,
                                   value_pos.y + 8.0f // Slightly offset down
    );
    draw_list->AddText(
        unit_pos, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.unit.c_str());

    float range_min = 0.0f;
    float range_max = 0.0f;
    if (GetDisplayRange(sensor, range_min, range_max)) {
        const float bar_width = widget_size.x - 2 * padding;
        const float bar_height = 8.0f;
        const ImVec2 bar_min =
            ImVec2(content_min.x, widget_min.y + widget_size.y * 0.60f);
        const ImVec2 bar_max =
            ImVec2(bar_min.x + bar_width, bar_min.y + bar_height);

        // Background
        draw_list->AddRectFilled(
            bar_min, bar_max,
            ImGui::ColorConvertFloat4ToU32(
                ImVec4(theme.GetSecondaryColor().x, theme.GetSecondaryColor().y,
                       theme.GetSecondaryColor().z, 0.3f)),
            4.0f);

        // Fill
        const float range = range_max - range_min;
        constexpr float MIN_RANGE = 1e-6f;
        float progress = 0.0f;
        if (std::abs(range) >= MIN_RANGE) {
            progress = (sensor.lastValue - range_min) / range;
        }
        const float fill_width =
            bar_width * std::max(0.0f, std::min(1.0f, progress));

        // Use color based on current value
        const ImVec4 fill_color =
            Colors::GetColorForValue(sensor.lastValue, sensor.colorZones);

        draw_list->AddRectFilled(
            bar_min, ImVec2(bar_min.x + fill_width, bar_max.y),
            ImGui::ColorConvertFloat4ToU32(fill_color), 4.0f);
    }

    // Sparkline (small trend graph closer to value, not at bottom)
    if (!sensor.history.empty()) {
        const float sparkline_height = 35.0f; // Increased from 26px
        const float sparkline_width = widget_size.x - 2 * padding;
        // Position sparkline at ~52% height (closer to value, less empty space)
        const ImVec2 sparkline_min =
            ImVec2(content_min.x, widget_min.y + widget_size.y * 0.52f);
        const ImVec2 sparkline_max = ImVec2(sparkline_min.x + sparkline_width,
                                            sparkline_min.y + sparkline_height);

        // Find min/max for scaling
        // I.12: Check if history is empty before accessing
        if (sensor.history.empty()) {
            // No data to render sparkline
            ImGui::Dummy(widget_size);
            return;
        }

        const auto [min_it, max_it] =
            std::minmax_element(sensor.history.begin(), sensor.history.end());
        const float min_val = *min_it;
        const float max_val = *max_it;
        const float range = max_val - min_val;

        // ES.10: Named constant for minimum range
        constexpr float MIN_RANGE = 1e-6f;
        if (range < MIN_RANGE) {
            // Insufficient dynamic range to render sparkline
            ImGui::Dummy(widget_size);
            return;
        }

        // ES.102: Prevent division by zero and unsigned underflow
        // Need at least 2 samples to draw a sparkline
        if (sensor.history.size() < 2) {
            ImGui::Dummy(widget_size);
            return;
        }

        // Draw sparkline
        const size_t history_size_minus_1 = sensor.history.size() - 1;
        for (size_t i = 1; i < sensor.history.size(); ++i) {
            const float x0 = sparkline_min.x +
                             (i - 1) * sparkline_width / history_size_minus_1;
            const float x1 =
                sparkline_min.x + i * sparkline_width / history_size_minus_1;

            const float val0 = sensor.history[i - 1];
            const float val1 = sensor.history[i];

            const float y0 =
                sparkline_max.y - ((val0 - min_val) / range) * sparkline_height;
            const float y1 =
                sparkline_max.y - ((val1 - min_val) / range) * sparkline_height;

            // Get color based on value at this segment (use average of val0 and
            // val1)
            const double segment_value = (val0 + val1) * 0.5;
            const ImVec4 line_color =
                Colors::GetColorForValue(segment_value, sensor.colorZones);

            draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1),
                               ImGui::ColorConvertFloat4ToU32(line_color),
                               2.0f);
        }
    }

    ImGui::Dummy(widget_size);
}

//
// GaugeWidgetRenderer
//

void GaugeWidgetRenderer::Render(const SensorState &sensor,
                                 const ImVec2 &widget_size) {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 widget_min = ImGui::GetCursorScreenPos();
    const ImVec2 widget_max =
        ImVec2(widget_min.x + widget_size.x, widget_min.y + widget_size.y);

    // Draw widget background
    draw_list->AddRectFilled(
        widget_min, widget_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetHoverColor()), 6.0f);

    // Draw border
    draw_list->AddRect(widget_min, widget_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       6.0f, 0, 1.5f);

    // Sensor name at top
    const float padding = 12.0f;
    draw_list->AddText(
        ImVec2(widget_min.x + padding, widget_min.y + padding),
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.name.c_str());

    // Gauge parameters
    const ImVec2 center = ImVec2(widget_min.x + widget_size.x * 0.5f,
                                 widget_min.y + widget_size.y * 0.58f);
    const float radius =
        std::min(widget_size.x, widget_size.y) * 0.36f; // Increased from 0.30f
    const float start_angle = 3.14159f * 0.75f;         // 135 degrees
    const float end_angle = 3.14159f * 2.25f; // 405 degrees (270 degree arc)

    // Background arc
    draw_list->PathArcTo(center, radius, start_angle, end_angle, 32);
    draw_list->PathStroke(
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(theme.GetSecondaryColor().x, theme.GetSecondaryColor().y,
                   theme.GetSecondaryColor().z, 0.3f)),
        0, 8.0f);

    // Value arc with division-by-zero safety guard (I.12)
    float range_min = 0.0f;
    float range_max = 0.0f;
    const bool has_range = GetDisplayRange(sensor, range_min, range_max);
    const float range = range_max - range_min;
    constexpr float MIN_RANGE = 1e-6f; // ES.10: Named constant

    float value_angle;
    if (!has_range || std::abs(range) < MIN_RANGE) {
        // Fallback to 50% if range invalid
        value_angle = start_angle + 0.5f * (end_angle - start_angle);
    } else {
        const float progress = std::clamp(
            static_cast<float>((sensor.lastValue - range_min) / range), 0.0f,
            1.0f);
        value_angle = start_angle + progress * (end_angle - start_angle);
    }

    // Use color based on current value
    const ImVec4 arc_color =
        Colors::GetColorForValue(sensor.lastValue, sensor.colorZones);

    draw_list->PathArcTo(center, radius, start_angle, value_angle, 32);
    draw_list->PathStroke(ImGui::ColorConvertFloat4ToU32(arc_color), 0, 8.0f);

    // Tick marks (every 20%, with emphasis on 0%, 50%, 100%)
    for (int i = 0; i <= 5; ++i) {
        const float tick_angle =
            start_angle + i * 0.2f * (end_angle - start_angle);
        const float tick_cos = std::cos(tick_angle);
        const float tick_sin = std::sin(tick_angle);

        // Major ticks at 0%, 50%, 100% are thicker and longer
        const bool is_major = (i == 0 || i == 5); // Start and end
        const float tick_length = is_major ? 14.0f : 10.0f;
        const float tick_thickness = is_major ? 3.0f : 2.0f;

        const ImVec2 tick_start =
            ImVec2(center.x + tick_cos * (radius - tick_length),
                   center.y + tick_sin * (radius - tick_length));
        const ImVec2 tick_end =
            ImVec2(center.x + tick_cos * radius, center.y + tick_sin * radius);

        draw_list->AddLine(
            tick_start, tick_end,
            ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
            tick_thickness);
    }

    // Center value text
    char value_str[64];
    snprintf(value_str, sizeof(value_str), "%.1f", sensor.lastValue);

    // C.21: Use FontGuard for automatic PopFont (RAII)
    {
        FontGuard font_guard(font_huge);
        if (font_guard.IsValid()) {
            const ImVec2 text_size = ImGui::CalcTextSize(value_str);
            const ImVec2 text_pos =
                ImVec2(center.x - text_size.x * 0.5f,
                       center.y - text_size.y * 0.5f - 10.0f);
            draw_list->AddText(
                font_huge, 48.0f, text_pos,
                ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                value_str);
        }
    } // PopFont() called automatically

    // Unit below value
    const ImVec2 unit_size = ImGui::CalcTextSize(sensor.unit.c_str());
    const ImVec2 unit_pos =
        ImVec2(center.x - unit_size.x * 0.5f, center.y + 10.0f);
    draw_list->AddText(
        unit_pos, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.unit.c_str());

    // Min/Max labels at arc extremes
    char min_label[32];
    char max_label[32];
    if (has_range) {
        snprintf(min_label, sizeof(min_label), "%.0f", range_min);
        snprintf(max_label, sizeof(max_label), "%.0f", range_max);
    } else {
        snprintf(min_label, sizeof(min_label), "N/A");
        snprintf(max_label, sizeof(max_label), "N/A");
    }

    // Min label (bottom left)
    const float min_angle = start_angle;
    const float min_label_distance = radius;
    const ImVec2 min_label_pos =
        ImVec2(center.x + std::cos(min_angle) * min_label_distance - 20.0f,
               center.y + std::sin(min_angle) * min_label_distance - 5.0f);
    draw_list->AddText(
        min_label_pos,
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()), min_label);

    // Max label (bottom right)
    const float max_angle = end_angle;
    const float max_label_distance = radius;
    const ImVec2 max_label_pos =
        ImVec2(center.x + std::cos(max_angle) * max_label_distance - 15.0f,
               center.y + std::sin(max_angle) * max_label_distance - 5.0f);
    draw_list->AddText(
        max_label_pos,
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()), max_label);

    // Needle pointer from center to current value
    const float needle_length = radius * 0.75f;
    const ImVec2 needle_tip =
        ImVec2(center.x + std::cos(value_angle) * needle_length,
               center.y + std::sin(value_angle) * needle_length);

    // Draw needle as a thick line with accent color
    draw_list->AddLine(center, needle_tip,
                       ImGui::ColorConvertFloat4ToU32(theme.GetAccentColor()),
                       3.0f);

    // Draw center dot to cover needle base
    draw_list->AddCircleFilled(
        center, 6.0f, ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()));

    ImGui::Dummy(widget_size);
}

//
// GraphWidgetRenderer
//

void GraphWidgetRenderer::Render(const SensorState &sensor,
                                 const ImVec2 &widget_size) {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 widget_min = ImGui::GetCursorScreenPos();
    const ImVec2 widget_max =
        ImVec2(widget_min.x + widget_size.x, widget_min.y + widget_size.y);

    // Draw widget background
    draw_list->AddRectFilled(
        widget_min, widget_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetHoverColor()), 6.0f);

    // Draw border
    draw_list->AddRect(widget_min, widget_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       6.0f, 0, 1.5f);

    // Sensor name at top
    const float padding = 12.0f;
    draw_list->AddText(
        ImVec2(widget_min.x + padding, widget_min.y + padding),
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.name.c_str());

    // Graph area
    const ImVec2 graph_min =
        ImVec2(widget_min.x + padding, widget_min.y + 40.0f);
    const ImVec2 graph_max =
        ImVec2(widget_max.x - padding, widget_max.y - padding);
    const float graph_width = graph_max.x - graph_min.x;
    const float graph_height = graph_max.y - graph_min.y;

    // Draw graph background
    draw_list->AddRectFilled(
        graph_min, graph_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetBackgroundColor()), 4.0f);

    if (!sensor.history.empty()) {
        // Find min/max for Y-axis scaling
        // I.12: Check if history is empty before accessing
        if (sensor.history.empty()) {
            // No data to render graph
            ImGui::Dummy(widget_size);
            return;
        }

        const auto [min_it, max_it] =
            std::minmax_element(sensor.history.begin(), sensor.history.end());
        const float min_val = *min_it;
        const float max_val = *max_it;
        const float range = max_val - min_val;

        // ES.10: Named constant for minimum range
        constexpr float MIN_RANGE = 1e-6f;
        if (range < MIN_RANGE) {
            // Insufficient dynamic range to render graph
            ImGui::Dummy(widget_size);
            return;
        }

        // Draw grid lines (horizontal)
        for (int i = 0; i <= 4; ++i) {
            const float y = graph_min.y + i * graph_height / 4.0f;
            draw_list->AddLine(ImVec2(graph_min.x, y), ImVec2(graph_max.x, y),
                               ImGui::ColorConvertFloat4ToU32(
                                   ImVec4(theme.GetSecondaryColor().x,
                                          theme.GetSecondaryColor().y,
                                          theme.GetSecondaryColor().z, 0.2f)),
                               1.0f);
        }

        // Draw line chart
        for (size_t i = 1; i < sensor.history.size(); ++i) {
            const float x0 = graph_min.x + (i - 1) * graph_width /
                                               (sensor.history.size() - 1);
            const float x1 =
                graph_min.x + i * graph_width / (sensor.history.size() - 1);

            const float val0 = sensor.history[i - 1];
            const float val1 = sensor.history[i];

            const float y0 =
                graph_max.y - ((val0 - min_val) / range) * graph_height;
            const float y1 =
                graph_max.y - ((val1 - min_val) / range) * graph_height;

            // Get color based on value at this segment (use average of val0 and
            // val1)
            const double segment_value = (val0 + val1) * 0.5;
            const ImVec4 line_color =
                Colors::GetColorForValue(segment_value, sensor.colorZones);

            draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1),
                               ImGui::ColorConvertFloat4ToU32(line_color),
                               2.0f);
        }

        // Draw Y-axis labels
        char min_label[16], max_label[16];
        snprintf(min_label, sizeof(min_label), "%.1f", min_val);
        snprintf(max_label, sizeof(max_label), "%.1f", max_val);

        draw_list->AddText(
            ImVec2(graph_min.x, graph_max.y - 14.0f),
            ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
            min_label);

        draw_list->AddText(
            ImVec2(graph_min.x, graph_min.y),
            ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
            max_label);
    }

    ImGui::Dummy(widget_size);
}

//
// BarGraphWidgetRenderer - Retro 80s/90s digital bargraph
//

void BarGraphWidgetRenderer::Render(const SensorState &sensor,
                                    const ImVec2 &widget_size) {
    auto &theme = ThemeManager::Instance();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 widget_min = ImGui::GetCursorScreenPos();
    const ImVec2 widget_max =
        ImVec2(widget_min.x + widget_size.x, widget_min.y + widget_size.y);

    // Draw widget background
    draw_list->AddRectFilled(
        widget_min, widget_max,
        ImGui::ColorConvertFloat4ToU32(theme.GetHoverColor()), 6.0f);

    // Draw border
    draw_list->AddRect(widget_min, widget_max,
                       ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor()),
                       6.0f, 0, 1.5f);

    // Sensor name at top
    const float padding = 12.0f;
    draw_list->AddText(
        ImVec2(widget_min.x + padding, widget_min.y + padding),
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.name.c_str());

    // Value display area (top center)
    const float value_area_height = 40.0f;
    char value_str[64];
    snprintf(value_str, sizeof(value_str), "%.0f", sensor.lastValue);

    {
        FontGuard font_guard(font_large);
        if (font_guard.IsValid()) {
            const ImVec2 text_size = ImGui::CalcTextSize(value_str);
            const ImVec2 text_pos =
                ImVec2(widget_min.x + (widget_size.x - text_size.x) * 0.5f,
                       widget_min.y + padding + 20.0f);
            draw_list->AddText(
                font_large, 32.0f, text_pos,
                ImGui::ColorConvertFloat4ToU32(theme.GetTextColor()),
                value_str);
        }
    }

    // Unit next to value
    const ImVec2 value_text_size = ImGui::CalcTextSize(value_str);
    const ImVec2 unit_pos =
        ImVec2(widget_min.x + (widget_size.x + value_text_size.x) * 0.5f + 5.0f,
               widget_min.y + padding + 32.0f);
    draw_list->AddText(
        unit_pos, ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()),
        sensor.unit.c_str());

    // Bar graph area (below value)
    const float bar_area_top =
        widget_min.y + padding + value_area_height + 15.0f;
    const float bar_area_bottom = widget_max.y - padding - 20.0f;
    const float bar_area_left = widget_min.x + padding;
    const float bar_area_right = widget_max.x - padding;
    const float bar_area_width = bar_area_right - bar_area_left;

    // Calculate number of bars (retro style: 20-30 bars)
    constexpr int num_bars = 25;
    const float bar_spacing = 2.0f;
    const float total_spacing = bar_spacing * (num_bars - 1);
    const float bar_width = (bar_area_width - total_spacing) / num_bars;

    // Calculate which bars should be filled based on current value
    float range_min = 0.0f;
    float range_max = 0.0f;
    const bool has_range = GetDisplayRange(sensor, range_min, range_max);
    const float range = range_max - range_min;
    constexpr float MIN_RANGE = 1e-6f;

    float fill_ratio = 0.5f; // Default to 50% if range invalid
    if (has_range && std::abs(range) >= MIN_RANGE) {
        fill_ratio = std::clamp(
            static_cast<float>((sensor.lastValue - range_min) / range), 0.0f,
            1.0f);
    }

    const int filled_bars = static_cast<int>(fill_ratio * num_bars);

    // Draw bars
    for (int i = 0; i < num_bars; ++i) {
        const float x = bar_area_left + i * (bar_width + bar_spacing);
        const bool is_filled = (i < filled_bars);

        // Calculate bar value for color zones
        const float bar_value = range_min + (i + 0.5f) * range / num_bars;

        // Get color for this bar's value
        ImVec4 bar_color;
        if (is_filled) {
            bar_color = Colors::GetColorForValue(bar_value, sensor.colorZones);
        } else {
            // Unfilled bars: very dim version of secondary color
            bar_color =
                ImVec4(theme.GetSecondaryColor().x, theme.GetSecondaryColor().y,
                       theme.GetSecondaryColor().z, 0.15f);
        }

        // Draw bar as vertical rectangle
        const ImVec2 bar_min = ImVec2(x, bar_area_top);
        const ImVec2 bar_max = ImVec2(x + bar_width, bar_area_bottom);

        draw_list->AddRectFilled(
            bar_min, bar_max, ImGui::ColorConvertFloat4ToU32(bar_color), 1.0f);
    }

    // Draw min/max labels below bars
    char min_label[32];
    char max_label[32];
    if (has_range) {
        snprintf(min_label, sizeof(min_label), "%.0f", range_min);
        snprintf(max_label, sizeof(max_label), "%.0f", range_max);
    } else {
        snprintf(min_label, sizeof(min_label), "N/A");
        snprintf(max_label, sizeof(max_label), "N/A");
    }

    draw_list->AddText(
        ImVec2(bar_area_left, bar_area_bottom + 2.0f),
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()), min_label);

    const ImVec2 max_label_size = ImGui::CalcTextSize(max_label);
    draw_list->AddText(
        ImVec2(bar_area_right - max_label_size.x, bar_area_bottom + 2.0f),
        ImGui::ColorConvertFloat4ToU32(theme.GetSecondaryColor()), max_label);

    ImGui::Dummy(widget_size);
}
