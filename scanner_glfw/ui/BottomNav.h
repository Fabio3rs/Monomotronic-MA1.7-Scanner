#pragma once

#include "../app_data.h" // For Screen enum
#include "imgui.h"

class BottomNav {
  public:
    BottomNav() = default;
    ~BottomNav() = default;

    // Render the bottom navigation bar
    void Render(float bar_height);

    // Badge configuration
    void SetDTCCount(int count) { dtc_count_ = count; }
    void SetLoggingActive(bool active) { logging_active_ = active; }

    int GetDTCCount() const { return dtc_count_; }
    bool IsLoggingActive() const { return logging_active_; }

  private:
    int dtc_count_ = 0;
    bool logging_active_ = false;

    // Helper methods
    void UpdateBadgeCounts(); // I.11: Sync badges with global state
    void RenderTabButton(const char *label, Screen screen, float btn_width,
                         float btn_height);
};
