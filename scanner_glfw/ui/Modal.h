#pragma once

#include "imgui.h"
#include <functional>
#include <string>

class Modal {
  public:
    Modal(const char *title);
    ~Modal() = default;

    // Modal lifecycle
    void Open();
    void Close();
    // I.11: Check actual ImGui popup state for consistency
    bool IsOpen() const { return ImGui::IsPopupOpen(title_.c_str()); }

    // Render the modal (call every frame)
    // Returns true if modal is still open
    bool Render(const std::function<void()> &content_func);

    // Configuration
    void SetSize(float width_ratio = 0.85f, float height_ratio = 0.75f);
    void SetFlags(ImGuiWindowFlags flags) { flags_ = flags; }

    // Convenience: Render with OK/Cancel buttons
    bool RenderWithButtons(const std::function<void()> &content_func,
                           bool *confirmed = nullptr);

  private:
    std::string title_;
    bool is_open_ = false;
    float width_ratio_ = 0.85f;
    float height_ratio_ = 0.75f;
    ImGuiWindowFlags flags_ = ImGuiWindowFlags_None;
};
