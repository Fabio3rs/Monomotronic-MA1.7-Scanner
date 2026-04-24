#include "Modal.h"
#include "../core/ThemeManager.h"
#include "../utils/Layout.h"

Modal::Modal(const char *title) : title_(title) {}

void Modal::Open() {
    if (!is_open_) {
        is_open_ = true;
        is_closing_ = false;
        fade_alpha_ = 0.0f;
        ImGui::OpenPopup(title_.c_str());
    }
}

void Modal::Close() {
    if (is_open_ && !is_closing_) {
        is_closing_ = true;
    }
}

void Modal::SetSize(float width_ratio, float height_ratio) {
    width_ratio_ = width_ratio;
    height_ratio_ = height_ratio;
}

bool Modal::Render(const std::function<void()> &content_func) {
    if (!is_open_) {
        return false;
    }

    // Update fade animation
    const float dt = ImGui::GetIO().DeltaTime;
    if (is_closing_) {
        fade_alpha_ -= kFadeSpeed * dt;
        if (fade_alpha_ <= 0.0f) {
            fade_alpha_ = 0.0f;
            is_open_ = false;
            is_closing_ = false;
            ImGui::CloseCurrentPopup();
            return false;
        }
    } else {
        fade_alpha_ += kFadeSpeed * dt;
        if (fade_alpha_ > 1.0f) {
            fade_alpha_ = 1.0f;
        }
    }

    // Calculate modal size with proper margins
    const ImVec2 modal_size = Layout::GetModalSize(width_ratio_, height_ratio_);

    ImGui::SetNextWindowSize(modal_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((ImGui::GetIO().DisplaySize.x - modal_size.x) * 0.5f,
               (ImGui::GetIO().DisplaySize.y - modal_size.y) * 0.5f),
        ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade_alpha_);
    const bool popup_open = ImGui::BeginPopupModal(
        title_.c_str(), &is_open_,
        flags_ | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (popup_open) {
        // Detect click outside popup to close it
        if (ImGui::IsMouseClicked(0) &&
            !ImGui::IsWindowHovered(
                ImGuiHoveredFlags_ChildWindows |
                ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            Close();
        }

        // Render user content
        if (content_func) {
            content_func();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // If user closed via X button, sync state
    if (!popup_open && is_open_ && !is_closing_) {
        is_open_ = false;
        is_closing_ = false;
        fade_alpha_ = 0.0f;
    }

    return is_open_;
}

bool Modal::RenderWithButtons(const std::function<void()> &content_func,
                              bool *confirmed) {
    if (confirmed) {
        *confirmed = false;
    }

    if (!is_open_) {
        return false;
    }

    // Update fade animation
    const float dt = ImGui::GetIO().DeltaTime;
    if (is_closing_) {
        fade_alpha_ -= kFadeSpeed * dt;
        if (fade_alpha_ <= 0.0f) {
            fade_alpha_ = 0.0f;
            is_open_ = false;
            is_closing_ = false;
            ImGui::CloseCurrentPopup();
            return false;
        }
    } else {
        fade_alpha_ += kFadeSpeed * dt;
        if (fade_alpha_ > 1.0f) {
            fade_alpha_ = 1.0f;
        }
    }

    const ImVec2 modal_size = Layout::GetModalSize(width_ratio_, height_ratio_);

    ImGui::SetNextWindowSize(modal_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((ImGui::GetIO().DisplaySize.x - modal_size.x) * 0.5f,
               (ImGui::GetIO().DisplaySize.y - modal_size.y) * 0.5f),
        ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade_alpha_);
    const bool popup_open = ImGui::BeginPopupModal(
        title_.c_str(), &is_open_,
        flags_ | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (popup_open) {
        // Detect click outside popup to close it
        if (ImGui::IsMouseClicked(0) &&
            !ImGui::IsWindowHovered(
                ImGuiHoveredFlags_ChildWindows |
                ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            if (confirmed) {
                *confirmed = false; // Treat as cancel
            }
            Close();
        }

        // Content area (reserve space for buttons at bottom)
        const float button_area_height = Layout::Modal::BUTTON_AREA_HEIGHT;
        const ImVec2 content_size =
            ImVec2(modal_size.x - Layout::Modal::CONTENT_PADDING, // Padding
                   modal_size.y - button_area_height -
                       Layout::Modal::TITLE_AND_BUTTONS_HEIGHT);

        ImGui::BeginChild("ModalContent", content_size, false);
        if (content_func) {
            content_func();
        }
        ImGui::EndChild();

        // Buttons at bottom
        ImGui::Separator();
        ImGui::Spacing();

        const float btn_width = Layout::Button::MODAL_WIDTH;
        const float btn_height = Layout::Button::MODAL_HEIGHT;
        const float spacing = Layout::Padding::MEDIUM;

        // Center buttons
        const float total_btn_width = btn_width * 2 + spacing;
        ImGui::SetCursorPosX((modal_size.x - total_btn_width) * 0.5f);

        auto &theme = ThemeManager::Instance();

        // OK button
        ImGui::PushStyleColor(ImGuiCol_Button, theme.GetPrimaryColor());
        if (ImGui::Button("\u2713 OK",
                          ImVec2(btn_width, btn_height))) { // \u2713 OK
            if (confirmed) {
                *confirmed = true;
            }
            Close();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Cancel button
        if (ImGui::Button("\u2715 Cancel",
                          ImVec2(btn_width, btn_height))) { // \u2715 Cancel
            if (confirmed) {
                *confirmed = false;
            }
            Close();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // If user closed via X button, sync state
    if (!popup_open && is_open_ && !is_closing_) {
        is_open_ = false;
        is_closing_ = false;
        fade_alpha_ = 0.0f;
    }

    return is_open_;
}
