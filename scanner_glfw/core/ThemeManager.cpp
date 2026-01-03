#include "ThemeManager.h"

void ThemeManager::SetTheme(Theme theme) {
    if (current_theme_ != theme) {
        current_theme_ = theme;
        ApplyThemeToImGui();
    }
}

void ThemeManager::ToggleTheme() {
    if (current_theme_ == Theme::DARK) {
        SetTheme(Theme::LIGHT);
    } else {
        SetTheme(Theme::DARK);
    }
}

ImVec4 ThemeManager::GetBackgroundColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Background
                                           : Colors::Light::Background;
}

ImVec4 ThemeManager::GetTextColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Text
                                           : Colors::Light::Text;
}

ImVec4 ThemeManager::GetPrimaryColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Primary
                                           : Colors::Light::Primary;
}

ImVec4 ThemeManager::GetSecondaryColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Secondary
                                           : Colors::Light::Secondary;
}

ImVec4 ThemeManager::GetAccentColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Accent
                                           : Colors::Light::Accent;
}

ImVec4 ThemeManager::GetSuccessColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Success
                                           : Colors::Light::Success;
}

ImVec4 ThemeManager::GetWarningColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Warning
                                           : Colors::Light::Warning;
}

ImVec4 ThemeManager::GetErrorColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Error
                                           : Colors::Light::Error;
}

ImVec4 ThemeManager::GetBorderColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Border
                                           : Colors::Light::Border;
}

ImVec4 ThemeManager::GetHoverColor() const {
    return (current_theme_ == Theme::DARK) ? Colors::Dark::Hover
                                           : Colors::Light::Hover;
}

void ThemeManager::ApplyThemeToImGui() {
    ImGuiStyle &style = ImGui::GetStyle();

    const ImVec4 bg_color = GetBackgroundColor();
    const ImVec4 text_color = GetTextColor();
    const ImVec4 primary_color = GetPrimaryColor();
    const ImVec4 secondary_color = GetSecondaryColor();
    const ImVec4 border_color = GetBorderColor();
    const ImVec4 hover_color = GetHoverColor();

    // Window colors
    style.Colors[ImGuiCol_WindowBg] = bg_color;
    style.Colors[ImGuiCol_ChildBg] = bg_color;
    style.Colors[ImGuiCol_PopupBg] = bg_color;
    style.Colors[ImGuiCol_Border] = border_color;

    // Text colors
    style.Colors[ImGuiCol_Text] = text_color;
    style.Colors[ImGuiCol_TextDisabled] =
        ImVec4(secondary_color.x, secondary_color.y, secondary_color.z, 0.6f);

    // Frame colors (inputs, etc.)
    style.Colors[ImGuiCol_FrameBg] = hover_color;
    style.Colors[ImGuiCol_FrameBgHovered] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.4f);
    style.Colors[ImGuiCol_FrameBgActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.6f);

    // Title bar
    style.Colors[ImGuiCol_TitleBg] = bg_color;
    style.Colors[ImGuiCol_TitleBgActive] = hover_color;
    style.Colors[ImGuiCol_TitleBgCollapsed] = bg_color;

    // Menu bar
    style.Colors[ImGuiCol_MenuBarBg] = hover_color;

    // Scrollbar
    style.Colors[ImGuiCol_ScrollbarBg] = bg_color;
    style.Colors[ImGuiCol_ScrollbarGrab] = secondary_color;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(secondary_color.x, secondary_color.y, secondary_color.z, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = primary_color;

    // Checkbox / Radio button
    style.Colors[ImGuiCol_CheckMark] = primary_color;

    // Slider
    style.Colors[ImGuiCol_SliderGrab] = primary_color;
    style.Colors[ImGuiCol_SliderGrabActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.8f);

    // Button
    style.Colors[ImGuiCol_Button] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = primary_color;

    // Header (collapsing header, tree node)
    style.Colors[ImGuiCol_Header] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.3f);
    style.Colors[ImGuiCol_HeaderHovered] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.5f);
    style.Colors[ImGuiCol_HeaderActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.7f);

    // Separator
    style.Colors[ImGuiCol_Separator] = border_color;
    style.Colors[ImGuiCol_SeparatorHovered] = primary_color;
    style.Colors[ImGuiCol_SeparatorActive] = primary_color;

    // Resize grip
    style.Colors[ImGuiCol_ResizeGrip] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.2f);
    style.Colors[ImGuiCol_ResizeGripHovered] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.4f);
    style.Colors[ImGuiCol_ResizeGripActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.6f);

    // Tab
    style.Colors[ImGuiCol_Tab] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.2f);
    style.Colors[ImGuiCol_TabHovered] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.6f);
    style.Colors[ImGuiCol_TabActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.4f);
    style.Colors[ImGuiCol_TabUnfocused] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.1f);
    style.Colors[ImGuiCol_TabUnfocusedActive] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.3f);

    // Table
    style.Colors[ImGuiCol_TableHeaderBg] = hover_color;
    style.Colors[ImGuiCol_TableBorderStrong] = border_color;
    style.Colors[ImGuiCol_TableBorderLight] =
        ImVec4(border_color.x, border_color.y, border_color.z, 0.5f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_TableRowBgAlt] =
        ImVec4(hover_color.x, hover_color.y, hover_color.z, 0.3f);

    // Text selection
    style.Colors[ImGuiCol_TextSelectedBg] =
        ImVec4(primary_color.x, primary_color.y, primary_color.z, 0.35f);

    // Drag drop target
    style.Colors[ImGuiCol_DragDropTarget] = primary_color;

    // Navigation highlight
    style.Colors[ImGuiCol_NavHighlight] = primary_color;
    style.Colors[ImGuiCol_NavWindowingHighlight] =
        ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);

    // Modal window dim
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
}
