#pragma once

#include "../utils/Colors.h"
#include "imgui.h"

class ThemeManager {
  public:
    enum class Theme { DARK, LIGHT };

    // Singleton access
    static ThemeManager &Instance() {
        static ThemeManager instance;
        return instance;
    }

    // Theme management
    void SetTheme(Theme theme);
    Theme GetTheme() const { return current_theme_; }
    void ToggleTheme();

    // Color getters (returns colors from current theme)
    ImVec4 GetBackgroundColor() const;
    ImVec4 GetTextColor() const;
    ImVec4 GetPrimaryColor() const;
    ImVec4 GetSecondaryColor() const;
    ImVec4 GetAccentColor() const;
    ImVec4 GetSuccessColor() const;
    ImVec4 GetWarningColor() const;
    ImVec4 GetErrorColor() const;
    ImVec4 GetBorderColor() const;
    ImVec4 GetHoverColor() const;

    // Apply theme to ImGui style
    void ApplyThemeToImGui();

    // Delete copy/move constructors (singleton)
    ThemeManager(const ThemeManager &) = delete;
    ThemeManager &operator=(const ThemeManager &) = delete;
    ThemeManager(ThemeManager &&) = delete;
    ThemeManager &operator=(ThemeManager &&) = delete;

  private:
    ThemeManager() = default;
    ~ThemeManager() = default;

    Theme current_theme_ = Theme::DARK;
};
