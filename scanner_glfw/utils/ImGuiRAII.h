#pragma once

#include "imgui.h"
#include <utility>

namespace UI {

// RAII wrapper for ImGui::PushStyleVar / PopStyleVar
class StyleVarGuard {
  public:
    StyleVarGuard() = default;
    explicit StyleVarGuard(ImGuiStyleVar idx, float val) { push(idx, val); }
    explicit StyleVarGuard(ImGuiStyleVar idx, const ImVec2 &val) {
        push(idx, val);
    }

    void push(ImGuiStyleVar idx, float val) {
        if (count_ < kMax) {
            ImGui::PushStyleVar(idx, val);
            ++count_;
        }
    }

    void push(ImGuiStyleVar idx, const ImVec2 &val) {
        if (count_ < kMax) {
            ImGui::PushStyleVar(idx, val);
            ++count_;
        }
    }

    ~StyleVarGuard() { release(); }

    // Non-copyable
    StyleVarGuard(const StyleVarGuard &) = delete;
    StyleVarGuard &operator=(const StyleVarGuard &) = delete;

    // Movable
    StyleVarGuard(StyleVarGuard &&other) noexcept : count_(other.count_) {
        other.count_ = 0;
    }
    StyleVarGuard &operator=(StyleVarGuard &&other) noexcept {
        if (this != &other) {
            release();
            count_ = other.count_;
            other.count_ = 0;
        }
        return *this;
    }

    [[nodiscard]] int count() const noexcept { return count_; }

    void release() {
        if (count_ > 0) {
            ImGui::PopStyleVar(count_);
            count_ = 0;
        }
    }

  private:
    static constexpr int kMax = 16;
    int count_ = 0;
};

// RAII wrapper for ImGui::PushStyleColor / PopStyleColor
class StyleColorGuard {
  public:
    StyleColorGuard() = default;
    explicit StyleColorGuard(ImGuiCol idx, const ImVec4 &col) {
        push(idx, col);
    }
    explicit StyleColorGuard(ImGuiCol idx, ImU32 col) { push(idx, col); }

    void push(ImGuiCol idx, const ImVec4 &col) {
        if (count_ < kMax) {
            ImGui::PushStyleColor(idx, col);
            ++count_;
        }
    }

    void push(ImGuiCol idx, ImU32 col) {
        if (count_ < kMax) {
            ImGui::PushStyleColor(idx, col);
            ++count_;
        }
    }

    ~StyleColorGuard() { release(); }

    StyleColorGuard(const StyleColorGuard &) = delete;
    StyleColorGuard &operator=(const StyleColorGuard &) = delete;

    StyleColorGuard(StyleColorGuard &&other) noexcept : count_(other.count_) {
        other.count_ = 0;
    }
    StyleColorGuard &operator=(StyleColorGuard &&other) noexcept {
        if (this != &other) {
            release();
            count_ = other.count_;
            other.count_ = 0;
        }
        return *this;
    }

    [[nodiscard]] int count() const noexcept { return count_; }

    void release() {
        if (count_ > 0) {
            ImGui::PopStyleColor(count_);
            count_ = 0;
        }
    }

  private:
    static constexpr int kMax = 16;
    int count_ = 0;
};

// RAII wrapper for ImGui::PushItemWidth / PopItemWidth
class ItemWidthGuard {
  public:
    explicit ItemWidthGuard(float width) { ImGui::PushItemWidth(width); }
    ~ItemWidthGuard() { ImGui::PopItemWidth(); }

    ItemWidthGuard(const ItemWidthGuard &) = delete;
    ItemWidthGuard &operator=(const ItemWidthGuard &) = delete;
};

} // namespace UI
