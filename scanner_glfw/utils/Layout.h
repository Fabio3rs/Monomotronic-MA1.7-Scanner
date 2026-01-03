#pragma once

#include "imgui.h"
#include <algorithm>

namespace Layout {

// Viewport dimensions (target touchscreen)
constexpr float SCREEN_WIDTH = 1024.0f;
constexpr float SCREEN_HEIGHT = 600.0f;

// Fixed UI areas
constexpr float TOP_BAR_HEIGHT = 52.0f;
constexpr float BOTTOM_NAV_HEIGHT = 80.0f;
constexpr float CONTENT_HEIGHT =
    SCREEN_HEIGHT - TOP_BAR_HEIGHT - BOTTOM_NAV_HEIGHT; // 468px

// Touch targets (Material Design guidelines)
constexpr float MIN_BUTTON_SIZE = 48.0f;
constexpr float TOUCH_PADDING = 8.0f;
constexpr float RECOMMENDED_BUTTON_HEIGHT = 60.0f;

// Spacing constants
constexpr float SPACING_SMALL = 8.0f;
constexpr float SPACING_MEDIUM = 12.0f;
constexpr float SPACING_LARGE = 20.0f;
constexpr float SPACING_XLARGE = 32.0f;

// Padding helpers
namespace Padding {
constexpr float TIGHT = 2.0f;
constexpr float SMALL = SPACING_SMALL;
constexpr float MEDIUM = SPACING_MEDIUM;
constexpr float LARGE = SPACING_LARGE;
constexpr float XLARGE = SPACING_XLARGE;
} // namespace Padding

// Button dimensions (standardized across all screens)
namespace Button {
// Primary action buttons
constexpr float PRIMARY_WIDTH = 95.0f;  // Edit, Record, Pause, Freeze
constexpr float PRIMARY_HEIGHT = 50.0f; // Reduced from 60px

// Secondary action buttons
constexpr float SECONDARY_WIDTH = 85.0f; // Snap, Signals
constexpr float SECONDARY_HEIGHT = 50.0f;

// Wide buttons (text-heavy)
constexpr float WIDE_WIDTH = 115.0f; // Reset, Read DTCs, Clear
constexpr float WIDE_HEIGHT = 50.0f;

// Pagination buttons
constexpr float PAGINATION_WIDTH = 90.0f; // Prev/Next
constexpr float PAGINATION_HEIGHT = 50.0f;

// Icon-only buttons
constexpr float ICON_SIZE = 48.0f;

// Spacing
constexpr float SPACING = 8.0f; // Between buttons (reduced from 12px)

// Live screen buttons
constexpr float LIVE_TOGGLE_WIDTH = 140.0f;
constexpr float LIVE_EDIT_WIDTH = 100.0f;

// Common control buttons
constexpr float ACTION_WIDTH = 100.0f;
constexpr float ACTION_WIDE_WIDTH = 120.0f;
constexpr float SIGNALS_WIDTH = 150.0f;
constexpr float DTC_ACTION_WIDTH = 180.0f;
constexpr float PAGE_WIDTH = 60.0f;

// Modal buttons
constexpr float MODAL_WIDTH = 120.0f;
constexpr float MODAL_HEIGHT = 50.0f;
constexpr float MODAL_CLOSE_HEIGHT = 60.0f;

// Pin buttons
constexpr float PIN_LIVE_SIZE = 64.0f;
constexpr float PIN_TABLE_SIZE = 56.0f;
} // namespace Button

namespace Input {
constexpr float FILTER_WIDTH = 300.0f;
constexpr float COMBO_WIDTH = 120.0f;
} // namespace Input

namespace Modal {
constexpr float MAX_WIDTH_RATIO = 0.85f;
constexpr float MAX_HEIGHT_RATIO = 0.75f;
constexpr float CHILD_MAX_WIDTH = 600.0f;
constexpr float CHILD_MAX_HEIGHT = 400.0f;
constexpr float CHILD_HORIZONTAL_PADDING = 40.0f;
constexpr float CHILD_VERTICAL_PADDING = 150.0f;
constexpr float CONTENT_PADDING = 20.0f;
constexpr float BUTTON_AREA_HEIGHT = 80.0f;
constexpr float TITLE_AND_BUTTONS_HEIGHT = 60.0f;
} // namespace Modal

namespace Table {
constexpr float LIVE_STATUS_COLUMN_WIDTH = 60.0f;
constexpr float LIVE_VALUE_COLUMN_WIDTH = 140.0f;
constexpr float LIVE_PIN_COLUMN_WIDTH = 64.0f;
constexpr float LIVE_TREND_COLUMN_WIDTH = 160.0f;
constexpr float LIVE_ROW_HEIGHT = 90.0f;
constexpr float LIVE_STATUS_RADIUS = 8.0f;
constexpr float LIVE_TREND_HEIGHT = 75.0f;

constexpr float SENSOR_VALUE_COLUMN_WIDTH = 180.0f;
constexpr float SENSOR_TREND_COLUMN_WIDTH = 150.0f;
constexpr float SENSOR_PIN_COLUMN_WIDTH = 64.0f;
constexpr float SENSOR_SPARKLINE_WIDTH = 100.0f;
constexpr float SENSOR_SPARKLINE_HEIGHT = 30.0f;

constexpr float DTC_ROW_HEIGHT = 50.0f;
} // namespace Table

namespace TopBar {
constexpr float SECTION_SPACING = 30.0f;
constexpr float ITEM_SPACING = 10.0f;
constexpr float SIGNAL_BAR_BASE_HEIGHT = 8.0f;
constexpr float SIGNAL_BAR_STEP = 4.0f;
constexpr float SIGNAL_BAR_SPACING = 8.0f;
constexpr float SIGNAL_BAR_WIDTH = 6.0f;
constexpr float SIGNAL_BAR_BASELINE = 28.0f;
constexpr float SIGNAL_BARS_WIDTH = 48.0f;
constexpr float SIGNAL_BARS_HEIGHT = 28.0f;
constexpr int SIGNAL_BAR_COUNT = 5;
constexpr float SESSION_BUTTON_WIDTH = 120.0f;
constexpr float RIGHT_PADDING = 10.0f;
constexpr float SEPARATOR_HEIGHT = 1.0f;
} // namespace TopBar

namespace BottomNav {
constexpr float ITEM_SPACING_X = 2.0f;
constexpr float INDICATOR_HEIGHT = 3.0f;
constexpr float SEPARATOR_HEIGHT = 1.0f;
} // namespace BottomNav

namespace Badge {
constexpr float DOT_RADIUS = 6.0f;
constexpr float DOT_MARGIN = 12.0f;
} // namespace Badge

namespace Graph {
constexpr float POPUP_MIN_WIDTH = 300.0f;
constexpr float POPUP_MIN_HEIGHT = 180.0f;
constexpr float POPUP_MAX_WIDTH_RATIO = 0.85f;
constexpr float POPUP_MAX_HEIGHT_RATIO = 0.75f;
constexpr float POPUP_COLUMN_MAX_WIDTH = 250.0f;
constexpr float POPUP_COLUMN_PADDING = 60.0f;
constexpr float FRAME_PADDING_X = 16.0f;
constexpr float FRAME_PADDING_Y = 20.0f;
} // namespace Graph

namespace Alert {
constexpr float BANNER_HEIGHT = 60.0f;
} // namespace Alert

// Sensor table
constexpr float SENSOR_ROW_HEIGHT = 70.0f; // Optimized (was 90px)
constexpr float SENSOR_STATUS_CIRCLE_RADIUS =
    12.0f; // Increased visibility (was 8px)

// Modals
constexpr float MODAL_MIN_MARGIN = 50.0f;
constexpr float MODAL_BORDER_RADIUS = 8.0f;

// DTC screen
constexpr float DTC_ROW_HEIGHT = 60.0f; // Adjusted for font_large (was 50px)
constexpr int DTC_PER_PAGE = 10;

// Helper functions (inline for header-only)

inline float GetContentWidth() { return ImGui::GetIO().DisplaySize.x; }

inline float GetContentHeight() {
    return ImGui::GetIO().DisplaySize.y - TOP_BAR_HEIGHT - BOTTOM_NAV_HEIGHT;
}

inline ImVec2 GetContentAreaMin() { return ImVec2(0.0f, TOP_BAR_HEIGHT); }

inline ImVec2 GetContentAreaMax() {
    return ImVec2(GetContentWidth(), GetContentHeight() + TOP_BAR_HEIGHT);
}

// Modal size calculation with proper margins
inline ImVec2 GetModalSize(float width_ratio = 0.85f,
                           float height_ratio = 0.75f) {
    const ImVec2 &vp_size = ImGui::GetIO().DisplaySize;

    const float max_width = vp_size.x - 2.0f * MODAL_MIN_MARGIN;
    const float max_height = vp_size.y - 2.0f * MODAL_MIN_MARGIN;

    const float width = std::min(max_width, vp_size.x * width_ratio);
    const float height = std::min(max_height, vp_size.y * height_ratio);

    return ImVec2(width, height);
}

// Center a window on screen
inline void CenterWindow() {
    const ImVec2 &vp_size = ImGui::GetIO().DisplaySize;
    const ImVec2 window_size = ImGui::GetWindowSize();

    ImGui::SetWindowPos(ImVec2((vp_size.x - window_size.x) * 0.5f,
                               (vp_size.y - window_size.y) * 0.5f));
}

// Check if a point is inside a rectangle
inline bool IsInsideRect(const ImVec2 &point, const ImVec2 &rect_min,
                         const ImVec2 &rect_max) {
    return point.x >= rect_min.x && point.x <= rect_max.x &&
           point.y >= rect_min.y && point.y <= rect_max.y;
}

} // namespace Layout
