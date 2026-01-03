#pragma once

#include "imgui.h"

// C++ Core Guidelines:
// - C.21: RAII - Resource Acquisition Is Initialization
// - R.1: Manage resources automatically using RAII
// - I.12: Check pointer validity before use
// - C++20: Delete copy/move for non-copyable resources

// FontGuard: RAII wrapper for ImGui font push/pop
//
// Usage:
//   {
//       FontGuard font_guard(font_large);
//       if (font_guard.IsValid()) {
//           ImGui::Text("Text with font_large");
//       }
//   }  // PopFont() called automatically when guard goes out of scope
//
// Benefits:
// - Automatic cleanup even if exceptions occur
// - No forgotten PopFont() calls
// - Clear ownership semantics
// - Null pointer safety

class FontGuard {
  public:
    // I.12: Explicit constructor validates pointer
    explicit FontGuard(ImFont *font) : font_(font), pushed_(false) {
        if (font_ != nullptr) {
            ImGui::PushFont(font_);
            pushed_ = true;
        }
    }

    // C.21: Destructor guarantees cleanup (RAII)
    ~FontGuard() {
        if (pushed_) {
            ImGui::PopFont();
        }
    }

    // C++20: Delete copy/move operations (non-copyable resource)
    FontGuard(const FontGuard &) = delete;
    FontGuard &operator=(const FontGuard &) = delete;
    FontGuard(FontGuard &&) = delete;
    FontGuard &operator=(FontGuard &&) = delete;

    // Query validity of the guard
    [[nodiscard]] bool IsValid() const noexcept { return pushed_; }

  private:
    ImFont *font_;
    bool pushed_;
};
