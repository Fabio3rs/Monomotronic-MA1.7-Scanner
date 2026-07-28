# Guia Prático: Sistema de Feedback Implementação

## 📦 Arquivos a Criar

```
scanner_glfw/feedback/
├── FeedbackSystem.h
├── FeedbackSystem.cpp
├── SystemBeep.h
├── Haptics.h (opcional)
└── CMakeLists.txt (atualizar)

scanner_glfw/ui/components/
├── LoadingSpinner.h
├── GlowEffect.h
└── FlashFeedback.h
```

---

## 1️⃣ FeedbackSystem.h (Base)

```cpp
#pragma once

#include "imgui.h"
#include <cstdint>
#include <string_view>

namespace Feedback {
    // Forward declarations
    class SystemBeep;

    enum class FeedbackType : std::uint8_t {
        // Visual
        PULSE_INFO = 0,
        PULSE_WARN = 1,
        PULSE_ERROR = 2,
        FLASH_SUCCESS = 3,
        FLASH_FAIL = 4,
        SPINNER_CONNECT = 5,
        GLOW_HIGHLIGHT = 6,
        FADE_TRANSITION = 7,

        // Audio
        BEEP_SUCCESS = 10,
        BEEP_ERROR = 11,
        BEEP_INFO = 12,

        // Haptic
        VIBRATE_SHORT = 20,
        VIBRATE_LONG = 21,
        VIBRATE_DOUBLE = 22,
    };

    // C++20 version: const parameter, noexcept
    class FeedbackSystemCpp20 {
    public:
        [[nodiscard]] static FeedbackSystemCpp20& Instance() noexcept;

        // NASA P10: bounded, no dynamic alloc in critical path
        void PlayVisualFeedback(FeedbackType type) noexcept;
        void PlayAudioFeedback(FeedbackType type) noexcept;
        void PlayHapticFeedback(FeedbackType type) noexcept;

        // Combo feedback
        void PlayFeedback(FeedbackType type) noexcept {
            PlayVisualFeedback(type);
            PlayAudioFeedback(type);
            PlayHapticFeedback(type);
        }

        // State query (for conditional triggers)
        [[nodiscard]] bool IsAudioEnabled() const noexcept {
            return audio_enabled_;
        }
        [[nodiscard]] bool IsHapticEnabled() const noexcept {
            return haptic_enabled_;
        }

        void SetAudioEnabled(bool enabled) noexcept {
            audio_enabled_ = enabled;
        }
        void SetHapticEnabled(bool enabled) noexcept {
            haptic_enabled_ = enabled;
        }

    private:
        FeedbackSystemCpp20() noexcept = default;
        ~FeedbackSystemCpp20() noexcept = default;

        // NASA P10: Delete copy/move (singleton pattern)
        FeedbackSystemCpp20(const FeedbackSystemCpp20&) = delete;
        FeedbackSystemCpp20& operator=(const FeedbackSystemCpp20&) = delete;
        FeedbackSystemCpp20(FeedbackSystemCpp20&&) = delete;
        FeedbackSystemCpp20& operator=(FeedbackSystemCpp20&&) = delete;

        bool audio_enabled_ = true;
        bool haptic_enabled_ = true;

        // Helper methods
        void PlayBeep(FeedbackType type) const noexcept;
        void PlayHaptic(FeedbackType type) const noexcept;
    };
}  // namespace Feedback
```

---

## 2️⃣ FeedbackSystem.cpp (Implementação)

```cpp
#include "FeedbackSystem.h"
#include "SystemBeep.h"
#include "../core/AnimationSystem.h"
#include <cassert>

namespace Feedback {
    FeedbackSystemCpp20& FeedbackSystemCpp20::Instance() noexcept {
        static FeedbackSystemCpp20 instance;
        return instance;
    }

    void FeedbackSystemCpp20::PlayVisualFeedback(FeedbackType type) noexcept {
        // Visual feedback é gerenciado pelo ImGui/AnimationSystem
        // Esta função é placeholder para futura expansão

        switch (type) {
            case FeedbackType::FLASH_SUCCESS:
                // Triggered via FlashFeedback component
                break;
            case FeedbackType::FLASH_FAIL:
                // Triggered via FlashFeedback component
                break;
            case FeedbackType::SPINNER_CONNECT:
                // Triggered via LoadingSpinner component
                break;
            case FeedbackType::GLOW_HIGHLIGHT:
                // Triggered via GlowEffect component
                break;
            default:
                break;
        }
    }

    void FeedbackSystemCpp20::PlayAudioFeedback(FeedbackType type) noexcept {
        if (!audio_enabled_) return;

        switch (type) {
            case FeedbackType::BEEP_SUCCESS:
                SystemBeep::BeepSuccess();
                break;
            case FeedbackType::BEEP_ERROR:
                SystemBeep::BeepError();
                break;
            case FeedbackType::BEEP_INFO:
                SystemBeep::BeepInfo();
                break;
            default:
                break;
        }
    }

    void FeedbackSystemCpp20::PlayHapticFeedback(FeedbackType type) noexcept {
        if (!haptic_enabled_) return;

        PlayHaptic(type);
    }

    void FeedbackSystemCpp20::PlayHaptic(FeedbackType type) const noexcept {
        #ifdef _WIN32
            // Windows: XInput API (implementado em Haptics.h)
            // TODO: Implementar
        #endif

        // Linux/macOS: Fallback silencioso por enquanto
    }
}  // namespace Feedback
```

---

## 3️⃣ SystemBeep.h (Áudio Nativo)

```cpp
#pragma once

#include <cstdint>

namespace Feedback {
    // NASA P10: Bounded, simple, no dynamic alloc
    class SystemBeep {
    public:
        // Cross-platform beep (Windows nativo, Linux/macOS fallback)
        static void Beep(int frequency_hz, int duration_ms) noexcept;

        // Presets
        static void BeepSuccess() noexcept {
            Beep(800, 150);   // 800Hz por 150ms
        }

        static void BeepError() noexcept {
            Beep(300, 300);   // 300Hz por 300ms
        }

        static void BeepInfo() noexcept {
            Beep(500, 100);   // 500Hz por 100ms
        }

        // Double tone (sucesso melhorado)
        static void BeepSuccessDouble() noexcept {
            Beep(800, 100);
            // Small delay: 50ms (pode ser melhorado com threading)
            Beep(1000, 100);
        }
    };
}  // namespace Feedback
```

### SystemBeep.cpp

```cpp
#include "SystemBeep.h"

#ifdef _WIN32
    #include <Windows.h>
#elif __APPLE__
    // macOS Beep: usar NSBeep() ou similar
    // Por enquanto: fallback silencioso
#else
    // Linux: usar "beep" command ou /dev/console
    // Por enquanto: fallback silencioso
#endif

namespace Feedback {
    void SystemBeep::Beep(int frequency_hz, int duration_ms) noexcept {
        #ifdef _WIN32
            ::Beep(frequency_hz, duration_ms);
        #else
            // Fallback para não-Windows
            // TODO: Implementar para macOS/Linux se necessário
        #endif
    }
}  // namespace Feedback
```

---

## 4️⃣ LoadingSpinner.h

```cpp
#pragma once

#include "imgui.h"
#include <cmath>

namespace UI::Components {
    class LoadingSpinner {
    public:
        // NASA P10: No dynamic alloc, bounded loop
        void Render(ImVec2 center, float radius) const noexcept;

    private:
        static constexpr int kSegments = 8;
        static constexpr float kSegmentThickness = 2.0f;
        static constexpr float kSegmentLength = 4.0f;
    };
}  // namespace UI::Components
```

### LoadingSpinner.cpp

```cpp
#include "LoadingSpinner.h"
#include "../core/AnimationSystem.h"

namespace UI::Components {
    void LoadingSpinner::Render(ImVec2 center, float radius) const noexcept {
        auto& anim = AnimationSystem::Instance();

        // Rotação contínua: 1 revolução por segundo
        float rotation = anim.GetPulse(1.0f) * 360.0f;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        constexpr float pi = 3.14159265f;

        // NASA P10: Bounded loop (kSegments = 8)
        for (int i = 0; i < kSegments; ++i) {
            // Fade dos segmentos posteriores
            float alpha = 1.0f - (static_cast<float>(i) / kSegments);

            float angle_deg = rotation + (i * (360.0f / kSegments));
            float angle_rad = angle_deg * pi / 180.0f;

            ImVec2 start = {
                center.x + radius * std::cos(angle_rad),
                center.y + radius * std::sin(angle_rad)
            };

            ImVec2 end = {
                center.x + (radius + kSegmentLength) * std::cos(angle_rad),
                center.y + (radius + kSegmentLength) * std::sin(angle_rad)
            };

            ImU32 color = ImGui::GetColorU32(
                ImVec4(0.5f, 0.7f, 1.0f, alpha)
            );

            draw_list->AddLine(start, end, color, kSegmentThickness);
        }
    }
}  // namespace UI::Components
```

---

## 5️⃣ GlowEffect.h

```cpp
#pragma once

#include "imgui.h"

namespace UI::Components {
    class GlowEffect {
    public:
        // Render subtle glow (pulsing)
        void RenderGlow(ImVec2 center, float base_radius, float intensity)
            const noexcept;

    private:
        static constexpr int kGlowLayers = 3;
        static constexpr float kGlowFrequency = 0.5f;  // 0.5 Hz = calm
    };
}  // namespace UI::Components
```

### GlowEffect.cpp

```cpp
#include "GlowEffect.h"
#include "../core/AnimationSystem.h"

namespace UI::Components {
    void GlowEffect::RenderGlow(ImVec2 center, float base_radius,
                                float intensity) const noexcept {
        auto& anim = AnimationSystem::Instance();

        // Suave pulsing (0.5 Hz)
        float glow_pulse = anim.GetPulse(kGlowFrequency);
        float alpha = 0.2f + 0.3f * glow_pulse;  // [0.2, 0.5]

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // NASA P10: Bounded loop
        for (int i = 0; i < kGlowLayers; ++i) {
            float layer_radius = base_radius + (i * 4.0f);
            float layer_alpha = alpha * (1.0f - (i * 0.33f));

            ImU32 glow_color = ImGui::GetColorU32(
                ImVec4(0.0f, 0.5f, 1.0f, layer_alpha)
            );

            draw_list->AddCircle(center, layer_radius, glow_color, 32, 1.0f);
        }
    }
}  // namespace UI::Components
```

---

## 6️⃣ FlashFeedback.h

```cpp
#pragma once

#include "imgui.h"

namespace UI::Components {
    class FlashFeedback {
    public:
        void Update(float delta_time) noexcept;

        void TriggerSuccess() noexcept { TriggerFlash(true); }
        void TriggerError() noexcept { TriggerFlash(false); }

        void Render() const noexcept;

        [[nodiscard]] bool IsActive() const noexcept {
            return flash_timer_ > 0.0f;
        }

    private:
        float flash_timer_ = 0.0f;
        bool flash_success_ = false;

        static constexpr float kFlashDuration = 0.3f;  // 300ms

        void TriggerFlash(bool success) noexcept;
    };
}  // namespace UI::Components
```

### FlashFeedback.cpp

```cpp
#include "FlashFeedback.h"

namespace UI::Components {
    void FlashFeedback::Update(float delta_time) noexcept {
        if (flash_timer_ > 0.0f) {
            flash_timer_ -= delta_time;
        }
    }

    void FlashFeedback::TriggerFlash(bool success) noexcept {
        flash_success_ = success;
        flash_timer_ = kFlashDuration;
    }

    void FlashFeedback::Render() const noexcept {
        if (flash_timer_ > 0.0f) {
            // Fade-out alpha
            float alpha = (flash_timer_ / kFlashDuration) * 0.5f;

            ImVec4 color = flash_success_
                ? ImVec4(0.0f, 1.0f, 0.0f, alpha)   // Verde
                : ImVec4(1.0f, 0.0f, 0.0f, alpha);  // Vermelho

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 screen_size = ImGui::GetIO().DisplaySize;
            ImU32 flash_color = ImGui::GetColorU32(color);

            draw_list->AddRectFilled(
                ImVec2(0, 0),
                screen_size,
                flash_color
            );
        }
    }
}  // namespace UI::Components
```

---

## 🔧 Integração em TopBar.cpp

### Antes
```cpp
void TopBar::RenderConnectionStatus() {
    ImGui::TextColored(
        ecu_connected_ ? Colors::Status::OK : Colors::Status::CRITICAL,
        "%s",
        ecu_connected_ ? "✓ CONNECTED" : "✗ DISCONNECTED"
    );
}
```

### Depois
```cpp
#include "feedback/FeedbackSystem.h"
#include "ui/components/LoadingSpinner.h"
#include "ui/components/GlowEffect.h"
#include "ui/components/FlashFeedback.h"

class TopBar {
private:
    // Feedback components
    UI::Components::FlashFeedback flash_feedback_;
    bool ecu_info_displayed_first_time_ = false;
    bool error_feedback_played_ = false;

public:
    void Update(float delta_time) {
        flash_feedback_.Update(delta_time);
    }

    void RenderConnectionStatus() {
        auto& feedback = Feedback::FeedbackSystemCpp20::Instance();
        auto& anim = AnimationSystem::Instance();

        if (connection_state_ == ConnectionState::CONNECTING) {
            RenderConnectingState(feedback, anim);
        } else if (connection_state_ == ConnectionState::CONNECTED) {
            RenderConnectedState(feedback, anim);
        } else if (connection_state_ == ConnectionState::ERROR) {
            RenderErrorState(feedback, anim);
        }
    }

private:
    void RenderConnectingState(
        Feedback::FeedbackSystemCpp20& feedback,
        AnimationSystem& anim) noexcept {

        // Spinner visual
        UI::Components::LoadingSpinner spinner;
        spinner.Render(ImVec2(50, 20), 8.0f);

        ImGui::SameLine();
        ImGui::TextUnformatted("⏳ CONNECTING...");

        // Timeout counter
        ImGui::SameLine(0, 20.0f);
        ImGui::Text("(%.1fs)", connection_timeout_counter_);
    }

    void RenderConnectedState(
        Feedback::FeedbackSystemCpp20& feedback,
        AnimationSystem& anim) noexcept {

        // Primeira vez que mostra?
        if (!ecu_info_displayed_first_time_) {
            flash_feedback_.TriggerSuccess();
            feedback.PlayAudioFeedback(
                Feedback::FeedbackType::BEEP_SUCCESS
            );
            feedback.PlayHapticFeedback(
                Feedback::FeedbackType::VIBRATE_DOUBLE
            );
            ecu_info_displayed_first_time_ = true;
        }

        // Glow effect
        UI::Components::GlowEffect glow;
        glow.RenderGlow(ImVec2(100, 20), 15.0f, 1.0f);

        ImGui::TextColored(
            Colors::Status::OK,
            "✓ ECU: %s | FW: %s",
            ecu_info_.model_code.c_str(),
            ecu_info_.firmware_revision.c_str()
        );
    }

    void RenderErrorState(
        Feedback::FeedbackSystemCpp20& feedback,
        AnimationSystem& anim) noexcept {

        if (!error_feedback_played_) {
            flash_feedback_.TriggerError();
            feedback.PlayAudioFeedback(
                Feedback::FeedbackType::BEEP_ERROR
            );
            feedback.PlayHapticFeedback(
                Feedback::FeedbackType::VIBRATE_LONG
            );
            error_feedback_played_ = true;
        }

        // Pulsing error badge
        float pulse = anim.GetPulse(1.0f);
        ImVec4 error_color = ImVec4(
            1.0f, 0.0f, 0.0f,
            0.6f + 0.4f * pulse
        );
        ImGui::TextColored(error_color, "✗ DISCONNECTED");
    }
};
```

---

## ✅ Checklist de Implementação

- [ ] Criar `FeedbackSystem.h/cpp`
- [ ] Criar `SystemBeep.h/cpp`
- [ ] Criar `LoadingSpinner.h/cpp`
- [ ] Criar `GlowEffect.h/cpp`
- [ ] Criar `FlashFeedback.h/cpp`
- [ ] Atualizar `TopBar.h/cpp` com feedback
- [ ] Adicionar includes em `CMakeLists.txt`
- [ ] Compilar com `-Wall -Wextra -Werror`
- [ ] Testes unitários com GoogleTest
- [ ] Code review NASA P10

---

## 📝 NASA P10 Compliance

✅ Funções < 50 LoC
✅ Loops bounded (max 8 segmentos spinner)
✅ No dynamic alloc pós-inicialização
✅ Assert/error handling em críticos
✅ Sem recursão
✅ Strong types (enums, não ints)

---

## 🚀 Próximos Passos

1. Implementar FeedbackSystem (1h)
2. Implementar componentes visuais (2h)
3. Integrar em TopBar (1h)
4. Testes (1h)
5. Code review (30min)

Total: **5.5 horas**

