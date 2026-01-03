#include "Badge.h"
#include <algorithm>
#include <cstdio>

namespace UI::Components {

bool Badge::ShouldRenderCount(int count) { return count > 0; }

int Badge::ClampCount(int count, int max_display) {
    if (count <= 0) {
        return 0;
    }
    if (max_display <= 0) {
        return count;
    }
    return std::min(count, max_display);
}

float Badge::CalculateRadiusForTextWidth(float text_width,
                                         const BadgeConfig &config) {
    return std::max(config.min_radius, text_width * 0.5f + config.padding);
}

ImVec2 Badge::CalculateCornerCenter(const ImVec2 &btn_min,
                                    const ImVec2 &btn_max, float radius,
                                    float margin) {
    return ImVec2(btn_max.x - margin - radius, btn_min.y + margin + radius);
}

void Badge::RenderCountBadge(const ImVec2 &center, int count,
                             const BadgeConfig &config) {
    if (!ShouldRenderCount(count)) {
        return;
    }

    const int clamped_count = ClampCount(count, config.max_display);
    if (clamped_count <= 0) {
        return;
    }

    char count_text[8];
    std::snprintf(count_text, sizeof(count_text), "%d", clamped_count);
    const ImVec2 text_size = ImGui::CalcTextSize(count_text);
    const float radius = CalculateRadiusForTextWidth(text_size.x, config);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    RenderCircle(draw_list, center, radius, config.background_color);
    RenderText(draw_list, center, count_text, config.text_color);
}

void Badge::RenderDotBadge(const ImVec2 &center, float radius,
                           const ImVec4 &color) {
    if (radius <= 0.0f) {
        return;
    }

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    RenderCircle(draw_list, center, radius, color);
}

void Badge::RenderCountBadgeAtCorner(const ImVec2 &btn_min,
                                     const ImVec2 &btn_max, int count,
                                     const BadgeConfig &config) {
    if (!ShouldRenderCount(count)) {
        return;
    }

    const int clamped_count = ClampCount(count, config.max_display);
    if (clamped_count <= 0) {
        return;
    }

    char count_text[8];
    std::snprintf(count_text, sizeof(count_text), "%d", clamped_count);
    const ImVec2 text_size = ImGui::CalcTextSize(count_text);
    const float radius = CalculateRadiusForTextWidth(text_size.x, config);
    const ImVec2 center =
        CalculateCornerCenter(btn_min, btn_max, radius, config.margin);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    RenderCircle(draw_list, center, radius, config.background_color);
    RenderText(draw_list, center, count_text, config.text_color);
}

void Badge::RenderDotBadgeAtCorner(const ImVec2 &btn_min, const ImVec2 &btn_max,
                                   float radius, float margin,
                                   const ImVec4 &color) {
    const ImVec2 center =
        CalculateCornerCenter(btn_min, btn_max, radius, margin);
    RenderDotBadge(center, radius, color);
}

void Badge::RenderCircle(ImDrawList *draw_list, const ImVec2 &center,
                         float radius, const ImVec4 &color) {
    draw_list->AddCircleFilled(center, radius,
                               ImGui::ColorConvertFloat4ToU32(color));
}

void Badge::RenderText(ImDrawList *draw_list, const ImVec2 &center,
                       const char *text, const ImVec4 &color) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }

    const ImVec2 text_size = ImGui::CalcTextSize(text);
    const ImVec2 text_pos =
        ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);
    draw_list->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(color), text);
}

} // namespace UI::Components
