#pragma once

#include "imgui.h"

namespace UI::Components {

struct BadgeConfig {
    ImVec4 background_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    float min_radius = 10.0f;
    float padding = 6.0f;
    float margin = 12.0f;
    int max_display = 99;
};

class Badge {
  public:
    static bool ShouldRenderCount(int count);
    static int ClampCount(int count, int max_display);
    static float CalculateRadiusForTextWidth(float text_width,
                                             const BadgeConfig &config);
    static ImVec2 CalculateCornerCenter(const ImVec2 &btn_min,
                                        const ImVec2 &btn_max, float radius,
                                        float margin);

    static void RenderCountBadge(const ImVec2 &center, int count,
                                 const BadgeConfig &config);
    static void RenderDotBadge(const ImVec2 &center, float radius,
                               const ImVec4 &color);
    static void RenderCountBadgeAtCorner(const ImVec2 &btn_min,
                                         const ImVec2 &btn_max, int count,
                                         const BadgeConfig &config);
    static void RenderDotBadgeAtCorner(const ImVec2 &btn_min,
                                       const ImVec2 &btn_max, float radius,
                                       float margin, const ImVec4 &color);

  private:
    static void RenderCircle(ImDrawList *draw_list, const ImVec2 &center,
                             float radius, const ImVec4 &color);
    static void RenderText(ImDrawList *draw_list, const ImVec2 &center,
                           const char *text, const ImVec4 &color);
};

} // namespace UI::Components
