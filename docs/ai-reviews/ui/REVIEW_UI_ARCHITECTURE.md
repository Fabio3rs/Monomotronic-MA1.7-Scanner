# Review Arquitetura da Interface Gráfica
## DRY, NASA P10 & CppCoreGuidelines

**Data:** 13 de janeiro de 2026
**Revisor:** GitHub Copilot
**Escopo:** scanner_glfw/* (ImGui + Vulkan UI)

---

## SUMÁRIO EXECUTIVO

| Aspecto | Status | Severidade | Prioridade |
|---------|--------|-----------|-----------|
| **DRY** | ⚠️ Violações encontradas | MÉDIA | ALTA |
| **NASA P10** | ⚠️ Alguns hotspots | MÉDIA | MÉDIA |
| **CppCoreGuidelines** | ⚠️ Desvios identificados | BAIXA-MÉDIA | ALTA |
| **Padrão geral** | ✅ Bem estruturado | - | - |

---

## 1. VIOLAÇÕES DRY (Don't Repeat Yourself)

### 1.1 Color Conversion Boilerplate Repetido

**Localização:** `ui_common.cpp:30-40, 50-60, 75-85`

```cpp
// ❌ PADRÃO REPETIDO (3+ vezes)
ImU32 status_color = ecuConnected ? IM_COL32(0, 255, 0, 255) : IM_COL32(128, 128, 128, 255);
ImGui::TextColored(
    ImVec4(((status_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
           ((status_color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
           ((status_color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
           ((status_color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f),
    "ECU:");
```

**Problema:**
- Conversão ImU32 → ImVec4 feita manualmente em múltiplos locais
- Código repetido de 7 linhas aparece 3+ vezes
- Difícil de manter e propenso a erros

**Recomendação:**
```cpp
// ✅ HELPER FUNCTION
inline ImVec4 ImU32ToVec4(ImU32 color) {
    return ImVec4(
        ((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
        ((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f
    );
}

// Uso:
ImGui::TextColored(ImU32ToVec4(status_color), "ECU:");
```

---

### 1.2 Status Color Logic Duplicado

**Localização:**
- `ui_common.cpp:GetStatusColor()` (1-15)
- `utils/Colors.h:Status::*` (constantes)
- `ui_common.cpp:40-75` (inline color selection)

```cpp
// ❌ GetStatusColor
ImU32 GetStatusColor(SensorStatus status) {
    switch (status) {
    case SensorStatus::OK:        return IM_COL32(0, 255, 0, 255);
    case SensorStatus::WARN:      return IM_COL32(255, 165, 0, 255);
    case SensorStatus::CRITICAL:  return IM_COL32(255, 0, 0, 255);
    case SensorStatus::STALE:     return IM_COL32(160, 170, 180, 255);
    }
    return IM_COL32(255, 255, 255, 255);
}

// ❌ Duplicado em TopBar.cpp:
ImU32 lat_color = kLineLatency < kLatencyGoodMs ? IM_COL32(0, 255, 0, 255)
                  : kLineLatency < kLatencyWarnMs ? IM_COL32(255, 165, 0, 255)
                  : IM_COL32(255, 0, 0, 255);
```

**Problema:**
- Mesma lógica em dois arquivos diferentes
- Se Colors::Status mudar, função também precisa mudar
- Sem sincronização = bugs potenciais

**Recomendação:**
```cpp
// colors.cpp
ImVec4 GetColorForStatus(SensorStatus status, bool use_light_theme = false) {
    if (use_light_theme) {
        static constexpr std::array<ImVec4, 4> status_colors = {
            Colors::Light::Success,
            Colors::Light::Warning,
            Colors::Light::Error,
            Colors::Light::Secondary,
        };
        return status_colors[static_cast<int>(status)];
    }
    // Dark theme...
}
```

---

### 1.3 Badge Rendering Duplicado

**Localização:**
- `ui_common.cpp:210-240` (DTC badge)
- `ui/BottomNav.cpp:95-120` (DTC badge repetido)

```cpp
// ❌ Code appearing twice with minor changes
if (dtc_count > 0) {
    ImVec2 badge_center = ImVec2(dtc_btn_max.x - 20, dtc_btn_min.y + 12);
    float badge_radius = 16;
    dl->AddCircleFilled(badge_center, badge_radius, IM_COL32(255, 0, 0, 255));

    char count_text[8];
    snprintf(count_text, sizeof(count_text), "%d", dtc_count > 99 ? 99 : dtc_count);
    ImVec2 text_size = ImGui::CalcTextSize(count_text);
    ImVec2 text_pos = ImVec2(badge_center.x - text_size.x * 0.5f,
                             badge_center.y - text_size.y * 0.5f);
    dl->AddText(text_pos, IM_COL32(255, 255, 255, 255), count_text);
}
```

**Problema:**
- Lógica de renderização de badge repetida
- Valores mágicos (20, 12, 16, 99, 255, 255, 255)
- Mudanças em um lugar precisam ser replicadas em outro

**Recomendação:**
```cpp
// ui_components/Badge.h
namespace UI {
class Badge {
    static void RenderCountBadge(
        ImVec2 position,
        int count,
        int max_display = 99,
        ImU32 bg_color = IM_COL32(255, 0, 0, 255),
        ImU32 text_color = IM_COL32(255, 255, 255, 255),
        float radius = 16.0f
    );
};
}
```

---

### 1.4 Table Column Setup Repetido

**Localização:**
- `ui_live_screen.cpp:155-165` (Live table columns)
- Potencialmente repetido em outros screens

```cpp
// ❌ Hard-coded column setup
ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 60);
ImGui::TableSetupColumn("Nome", ImGuiTableColumnFlags_WidthStretch);
ImGui::TableSetupColumn("Valor", ImGuiTableColumnFlags_WidthFixed, 140);
ImGui::TableSetupColumn("Fixar", ImGuiTableColumnFlags_WidthFixed, 64);
ImGui::TableSetupColumn("Tendência", ImGuiTableColumnFlags_WidthFixed, 160);
ImGui::TableHeadersRow();
```

**Problema:**
- Definição de colunas espalhada
- Sem reutilização entre screens
- Valores de largura como magic numbers

**Recomendação:**
```cpp
// ui/SensorTable.h ou novo arquivo ui/TableDefinitions.h
namespace UI::Table {
    struct ColumnDef {
        const char* label;
        ImGuiTableColumnFlags flags;
        float width;
    };

    const std::vector<ColumnDef> SENSOR_COLUMNS = {
        {"Status", ImGuiTableColumnFlags_WidthFixed, 60.0f},
        {"Nome", ImGuiTableColumnFlags_WidthStretch, 0.0f},
        {"Valor", ImGuiTableColumnFlags_WidthFixed, 140.0f},
        // ...
    };

    void SetupColumns(const std::vector<ColumnDef>& cols) {
        for (const auto& col : cols) {
            ImGui::TableSetupColumn(col.label, col.flags, col.width);
        }
        ImGui::TableHeadersRow();
    }
}
```

---

### 1.5 Button Layout Magic Numbers

**Localização:** Vários arquivos
- `ui_live_screen.cpp:30-40` - Button sizing
- `ui/BottomNav.cpp:45-55` - Button widths
- `TopBar.cpp:25-55` - Layout positioning

```cpp
// ❌ Magic numbers espalhados
if (ImGui::Button("All Sensors" : "Custom", ImVec2(140, 60))) { }
if (ImGui::Button("\u270E Edit", ImVec2(100, 60))) { }
if (ImGui::Button("\u25CF Rec", ImVec2(100, 60))) { }

// BottomNav
const float btn_width = total_width / 5.0f;
const float btn_height = bar_height;

// TopBar
const float left_vertical_offset = 2.0f;
const float right_vertical_offset = -10.0f;
```

**Problema:**
- Inconsistência entre screens
- Difícil manutenção de design system
- Quebra de responsividade se viewport mudar

**Recomendação:**
```cpp
// utils/Layout.h (já existe, mas pode ser expandido)
namespace Layout {
    namespace Button {
        constexpr float STANDARD_HEIGHT = 60.0f;
        constexpr float TOGGLE_WIDTH = 140.0f;
        constexpr float ACTION_WIDTH = 100.0f;
        constexpr float EDIT_BUTTON_WIDTH = 100.0f;
        // ...
    }

    namespace Padding {
        constexpr float TOP_BAR_LEFT = 2.0f;
        constexpr float TOP_BAR_RIGHT = -10.0f;
        // ...
    }
}
```

---

## 2. NASA P10 (Complexidade Cognitiva)

### 2.1 DrawTopBar() - Alta Complexidade

**Localização:** `ui_common.cpp:18-130`

```cpp
void DrawTopBar(float bar_height) {
    // 10+ passos lógicos
    // 5+ condicionais aninhados
    // Múltiplas conversões de cor
    // Layout positioning complexo
    // Signal strength calculation
    // Table status selection
    // Time formatting
    // ...
}
```

**Análise P10:**
- **Complexidade Cognitiva:** ~22 (MUITO ALTA)
  - Viewport setup: +1
  - Font push: +1
  - Status color selection: +2 (condicional)
  - Signal strength bars (loop): +3 (for loop + condicional interno)
  - Latency color selection: +3 (if-else-if)
  - Table color selection: +3 (if-else-if)
  - Age color selection: +3 (if-else-if)
  - Time formatting: +1
  - Button positioning: +2 (cálculos complexos)
  - Session manager popup: +1

**Recomendação:** Quebrar em 4 componentes menores

```cpp
// Novo arquivo: ui/components/StatusIndicator.h
class StatusIndicator {
    void Render(bool ecu_connected);
};

// Novo arquivo: ui/components/SignalBars.h
class SignalBars {
    void Render(float latency, float error_rate);
};

// Novo arquivo: ui/components/TimeDisplay.h
class TimeDisplay {
    void Render();
};

// ui_common.cpp (refatorado)
void DrawTopBar(float bar_height) {
    ImGui::BeginChild("TopBar", ...);

    StatusIndicator status;
    status.Render(ecuConnected);

    SignalBars signal;
    signal.Render(kLineLatency, kLineErrorRate);

    TimeDisplay time;
    time.Render();

    ImGui::EndChild();
}
```

**Benefício:** Reduz P10 de ~22 para 4-5 por função.

---

### 2.2 DashScreen::Render() - Altíssima Complexidade

**Localização:** `screens/DashScreen.cpp:50-100+`

**Análise inicial:**
- Widget grid rendering (loop)
- Gesture handling
- Context menu logic
- Modal rendering (3+ tipos)
- State management
- Fade animations

**Complexidade estimada:** ~28+

**Recomendação:**
```cpp
// Desacoplar em padrão Strategy
class DashScreen::ScreenRenderer {
    virtual void RenderTopControls() = 0;
    virtual void RenderWidgetGrid() = 0;
    virtual void RenderModals() = 0;
};

// Implementações específicas
class DashScreenPhase1Renderer : public ScreenRenderer { };
class DashScreenPhase2Renderer : public ScreenRenderer { };

void DashScreen::Render() {
    renderer_->RenderTopControls();
    renderer_->RenderWidgetGrid();
    renderer_->RenderModals();
}
```

---

### 2.3 LiveScreen::DrawModal EditCustomList

**Localização:** `ui_live_screen.cpp:70-125`

```cpp
if (ImGui::BeginPopupModal("Edit Custom List", NULL, ...)) {
    // ... size calculations
    // ... font push/pop
    // ... nested loops (categories + sensors)
    // ... checkbox state management
    // ... list manipulation (push_back/erase)
    // ... button layout
    ImGui::EndPopup();
}
```

**Problemas:**
- P10: ~18 (condicional + loops aninhados)
- Manipulação de container inline
- Sem validação

**Recomendação:**
```cpp
// ui/modals/EditSensorListModal.h
class EditSensorListModal {
    void Open();
    void Close();
    bool Render();
    const std::vector<int>& GetSelectedSensors() const;

private:
    std::vector<int> temp_selection_;
    void ApplySelection();
};

// ui_live_screen.cpp (refatorado)
void DrawLiveScreen() {
    if (ImGui::Button("Edit")) {
        edit_modal_.Open();
    }

    if (edit_modal_.Render()) {
        customSensorList = edit_modal_.GetSelectedSensors();
    }
}
```

---

### 2.4 GraphScreen::OnEnter() - Complexidade Moderada

**Localização:** `screens/GraphScreen.cpp:25-60`

```cpp
void GraphScreen::OnEnter() {
    SetActive(true);

    graph_frozen_ = graphFrozen;

    auto &state = StateManager::Instance();
    if (state.IsInitialized()) {
        graphSensorIndices = state.LoadGraphSensors();

        // Nested loops + conditionals
        std::vector<int> pinned = state.LoadPinnedSensors();
        for (int sensor_idx : pinned) {
            auto it = std::find(graphSensorIndices.begin(), ...);
            bool already_in_graph = (it != graphSensorIndices.end());
            if (!already_in_graph && graphSensorIndices.size() < 4) {
                graphSensorIndices.push_back(sensor_idx);
            }
        }

        if (!pinned.empty()) {
            state.SaveGraphSensors(graphSensorIndices);
        }
    }

    auto_scale_y_.resize(...);
    last_plot_min_y_.resize(...);
    // ...
}
```

**P10: ~15 (moderado)**

**Recomendação:**
```cpp
void GraphScreen::OnEnter() {
    SetActive(true);
    LoadStateFromManager();
    SyncPinnedSensorsToGraph();
    InitializeGraphMetadata();
    ApplySubscriptions();
    CacheColorConversions();
}

private:
    void LoadStateFromManager() {
        auto &state = StateManager::Instance();
        if (!state.IsInitialized()) return;
        graphSensorIndices = state.LoadGraphSensors();
    }

    void SyncPinnedSensorsToGraph() {
        auto pinned = StateManager::Instance().LoadPinnedSensors();
        for (int idx : pinned) {
            AddPinnedSensorToGraph(idx);
        }
    }
```

---

## 3. CppCoreGuidelines Violations

### 3.1 Uso Impróprio de `glfwGetTime()` e `ImGui::GetTime()`

**Localização:**
- `ui_live_screen.cpp:190` - `glfwGetTime()`
- `ui_common.cpp:110` - `ImGui::GetTime()`

```cpp
// ❌ Mistura de APIs de tempo
float pulse = 0.5f + 0.5f * sinf((float)glfwGetTime() * 5.0f);
double freshest = 0.0;
double now_sec = ImGui::GetTime();
```

**Problema (C.1 - Respeitar escopos):**
- Duas APIs diferentes para mesma coisa
- Sem sincronização = comportamento imprevisível
- Acoplamento a GLFW e ImGui

**Recomendação (C.22 - Make objects members of a class):**
```cpp
// core/TimingManager.h (novo)
class TimingManager {
    static TimingManager& Instance();

    double GetCurrentTime() const;
    float GetPulseAnimation(float frequency = 5.0f) const;
    double GetTimeSinceLastUpdate(double reference_time) const;

private:
    TimingManager() = default;
    // Cache time for frame consistency
};

// Uso:
float pulse = 0.5f + 0.5f * sinf(TimingManager::Instance().GetPulseAnimation() * M_PI);
```

---

### 3.2 Conversão de Tipo Perigosa

**Localização:** `ui_common.cpp:115`

```cpp
// ❌ Conversão implícita e casting perigoso
ImGui::TextColored(
    ImVec4(((status_color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f, ...),
    ...
);
```

**Problema (ES.4 - Use constexpr for values known at compile time):**
- Operações de bit em tempo de execução
- Conversão int → float sem proteção
- Magic numbers

**Recomendação:**
```cpp
// ES.4: Use constexpr
constexpr ImU32 EXTRACT_CHANNEL(ImU32 color, int shift) {
    return (color >> shift) & 0xFFU;
}

// ES.46: Preferir funções sobre macros
inline float NormalizeColorChannel(ImU32 channel) {
    return channel / 255.0f;
}

// Uso (mais seguro):
auto R = NormalizeColorChannel(EXTRACT_CHANNEL(status_color, IM_COL32_R_SHIFT));
auto G = NormalizeColorChannel(EXTRACT_CHANNEL(status_color, IM_COL32_G_SHIFT));
// ...
ImGui::TextColored(ImVec4(R, G, B, A), "Text");
```

---

### 3.3 Falta de Validação de Bounds

**Localização:** `ui_live_screen.cpp:145-180`

```cpp
// ❌ Sem validação
for (int i : sensors_to_display) {
    auto &s = simulatedSensors[i];  // Sem verificação se i é válido!
    // ...

    ImGui::TableNextColumn();
    ImGui::PushID(i + 1000);  // Magic number + sem validação
    // ...
}
```

**Problema (C.44 - Prefira classes que encapsulam):**
- Acesso direto ao vetor sem bounds check
- ID mágico sem proteção
- Pode causar undefined behavior

**Recomendação:**
```cpp
// Wrapper com validação (COR.1 - C.33)
class SensorIndexValidator {
public:
    bool IsValid(int idx) const {
        return idx >= 0 && idx < static_cast<int>(simulatedSensors.size());
    }

    const SensorState& GetSensor(int idx) const {
        static const SensorState dummy{};
        return IsValid(idx) ? simulatedSensors[idx] : dummy;
    }
};

// Uso:
SensorIndexValidator validator;
for (int i : sensors_to_display) {
    if (!validator.IsValid(i)) continue;  // Seguro

    auto &s = validator.GetSensor(i);
    ImGui::PushID(GenerateUniqueId(i));  // Função segura para ID
}
```

---

### 3.4 `const`-correctness Incompleta

**Localização:**
- `ui/TopBar.cpp` - Muitos métodos sem `const`
- `ui/BottomNav.cpp` - Métodos de render não marcados `const`

```cpp
// ❌ Sem const
void TopBar::RenderConnectionStatus() {  // Deveria ser const
    // ...
}

void BottomNav::UpdateBadgeCounts() {  // Deveria ser const
    logging_active_ = loggingActive;
}
```

**Problema (C.12 - Respeitar const-correctness):**
- Quebra de contrato de interface
- Difficultà de otimização
- Não permite uso em contextos const

**Recomendação:**
```cpp
// Correto
class TopBar {
public:
    void Render(float bar_height) const;  // Não modifica estado

private:
    void RenderConnectionStatus() const;
    void RenderSignalBars() const;
    void RenderTime() const;
};

class BottomNav {
public:
    void Render(float bar_height);  // Modifica badges

private:
    void UpdateBadgeCounts();  // Helper privado
    void RenderTabButton(...) const;
};
```

---

### 3.5 Inicialização de Membro Incompleta

**Localização:** `screens/BaseScreen.h` e subclasses

```cpp
// ❌ Inicialização implícita
class BaseScreen {
protected:
    float fade_alpha_;  // Pode não ser inicializado!
    // ...
};

// ✅ Correto (C.47)
class BaseScreen {
protected:
    float fade_alpha_ = 0.0f;
    bool is_active_ = false;
    // ...
};
```

**Problema (C.47 - Definir e inicializar data members):**
- undefined behavior
- Debugging difícil

---

### 3.6 Ownership Ambíguo com Raw Pointers

**Localização:** `main.cpp:55-100`

```cpp
// ❌ Raw pointers globais
static LiveScreen *g_live_screen = nullptr;
static GraphScreen *g_graph_screen = nullptr;
static DashScreen *g_dash_screen = nullptr;
static DTCScreen *g_dtc_screen = nullptr;
static LogsScreen *g_logs_screen = nullptr;
static BaseScreen *g_current_screen = nullptr;
```

**Problema (R.3 - Função deve expressar ownership):**
- Quem é responsável por deletar?
- Sem RAII
- Memory leak potencial

**Recomendação (C.33 - use std::unique_ptr):**
```cpp
// ✅ Seguro e explícito (R.3, R.22)
class ScreenManager {
private:
    std::unique_ptr<LiveScreen> live_screen_;
    std::unique_ptr<GraphScreen> graph_screen_;
    std::unique_ptr<DashScreen> dash_screen_;
    std::unique_ptr<DTCScreen> dtc_screen_;
    std::unique_ptr<LogsScreen> logs_screen_;

    BaseScreen* current_screen_ = nullptr;  // Non-owning view

public:
    ScreenManager()
        : live_screen_(std::make_unique<LiveScreen>()),
          graph_screen_(std::make_unique<GraphScreen>()),
          // ...
    { }

    BaseScreen* GetCurrentScreen() const { return current_screen_; }
    void SetCurrentScreen(Screen screen_id);
};

// main.cpp
static ScreenManager screen_manager;
```

---

### 3.7 Ausência de Verificação de nullptr

**Localização:** `TopBar.cpp:GetAppWindow()` e usages

```cpp
// ❌ Sem proteção
GLFWwindow *GetAppWindow() { return g_AppWindow; }  // Pode ser nullptr!

// Uso em TopBar.cpp:
extern GLFWwindow *GetAppWindow();
// ... usado sem verificação
```

**Problema (C.39 - Evitar funções abstratas sem propósito):**
- Violação de contrato implícito
- Undefined behavior se nullptr

**Recomendação:**
```cpp
// C.39 + E.30
class WindowManager {
public:
    static WindowManager& Instance();

    GLFWwindow* GetWindow() const {
        return window_;
    }

    bool HasWindow() const {
        return window_ != nullptr;
    }

    void SetWindow(GLFWwindow* w) {
        window_ = w;  // Poderia adicionar assertions
    }

private:
    GLFWwindow* window_ = nullptr;
};

// Uso seguro:
if (auto* window = WindowManager::Instance().GetWindow()) {
    // Use window
} else {
    // Handle error
    assert(false && "Window not initialized!");
}
```

---

## 4. Recomendações Prioritárias

### Tier 1: CRÍTICO (Fazer Primeiro)

| Problema | Arquivo | Linhas | Tipo | Esforço | Ganho |
|----------|---------|--------|------|---------|-------|
| Color conversion DRY | utils/Colors.h | NEW | DRY | 30min | Alto |
| Badge rendering | ui/components/ | NEW | DRY | 1h | Alto |
| TopBar P10 split | ui/components/ | NEW | P10 | 2h | Alto |
| Raw pointer ownership | main.cpp | 55-100 | CppCG | 1.5h | Crítico |

### Tier 2: IMPORTANTE (Próximas)

| Problema | Arquivo | Tipo | Esforço |
|----------|---------|------|---------|
| Table column definitions | ui/TableDefinitions.h | NEW | DRY |
| Modal extraction | ui/modals/ | NEW | P10 |
| const-correctness | ui/*.cpp | REFACTOR | 1.5h |
| Time API unification | core/TimingManager.h | NEW | P10 |

### Tier 3: DESEJÁVEL (Futuro)

| Problema | Arquivo | Tipo | Esforço |
|----------|---------|------|---------|
| Layout magic numbers | utils/Layout.h | EXPAND | 1h |
| Bounds validation | ui/validators/ | NEW | 2h |
| Complete init | screens/*.h | REFACTOR | 1h |

---

## 5. Plano de Implementação Sugerido

### Fase 1: Refatoração DRY (2-3 horas)

```
1. Criar utils/ColorUtils.h
   - ImU32ToVec4()
   - GetStatusColor() unificado

2. Criar ui/components/Badge.h
   - RenderCountBadge()
   - Remover duplicações

3. Criar ui/TableDefinitions.h
   - SetupColumns()
   - Constantes de coluna

4. Atualizar Layout.h
   - Adicionar constantes de button/padding
```

### Fase 2: Redução P10 (3-4 horas)

```
1. Criar ui/components/StatusIndicator.h
   - Isolar DrawTopBar complexo

2. Criar ui/modals/EditSensorListModal.h
   - Encapsular modal complexity

3. Refatorar GraphScreen::OnEnter()
   - Quebrar em 4 métodos privados

4. Criar core/TimingManager.h
   - Centralizar timing
```

### Fase 3: CppCoreGuidelines (4-5 horas)

```
1. Criar core/ScreenManager.h
   - Substituir raw pointers
   - RAII e unique_ptr

2. Criar core/WindowManager.h
   - Centralizar acesso a GLFWwindow

3. Atualizar const-correctness
   - Marcar métodos const

4. Adicionar validações
   - Bounds checking
   - nullptr guards
```

---

## 6. Exemplos de Código - Implementação Rápida

### 6.1 ColorUtils.h (CRIAR)

```cpp
#pragma once
#include "imgui.h"

namespace UI::Colors {

    // ES.4: constexpr para valores conhecidos em compile-time
    constexpr ImU32 CHANNEL_MASK = 0xFFU;

    // ES.46: Preferir funções sobre macros
    inline float NormalizeColorChannel(ImU32 channel) {
        return (channel & CHANNEL_MASK) / 255.0f;
    }

    // C.21: Retornar ImVec4 por valor (é pequeno)
    inline ImVec4 ImU32ToVec4(ImU32 color) {
        return ImVec4(
            NormalizeColorChannel(color >> IM_COL32_R_SHIFT),
            NormalizeColorChannel(color >> IM_COL32_G_SHIFT),
            NormalizeColorChannel(color >> IM_COL32_B_SHIFT),
            NormalizeColorChannel(color >> IM_COL32_A_SHIFT)
        );
    }

    // Wrapper seguro (C.44)
    class SafeU32Color {
        ImU32 color_;
    public:
        SafeU32Color(ImU32 c) : color_(c) {}
        ImVec4 ToVec4() const { return ImU32ToVec4(color_); }
        ImU32 ToU32() const { return color_; }
    };
}
```

### 6.2 StatusIndicator.h (CRIAR)

```cpp
#pragma once
#include "imgui.h"

namespace UI::Components {

    class StatusIndicator {
    public:
        void Render(bool ecu_connected) const;

    private:
        void RenderLabel() const;
        void RenderStatus(bool ecu_connected) const;
    };
}
```

### 6.3 EditSensorListModal.h (CRIAR)

```cpp
#pragma once
#include <vector>

namespace UI::Modals {

    class EditSensorListModal {
    public:
        explicit EditSensorListModal(const char* title);

        void Open();
        void Close();
        bool Render();

        const std::vector<int>& GetSelectedSensors() const {
            return temp_selection_;
        }

    private:
        void ApplySelection();
        void CancelSelection();

        std::string title_;
        std::vector<int> temp_selection_;
        bool is_open_ = false;
    };
}
```

---

## 7. Checklist de Verificação

### Para Cada Arquivo Refatorado:

- [ ] Não há duplicação de lógica (DRY)
- [ ] Complexidade cognitiva < 10 por função (NASA P10)
- [ ] `const` correctness implementado (CppCG C.12)
- [ ] `unique_ptr` para ownership (CppCG R.3)
- [ ] Inicialização de membros completa (CppCG C.47)
- [ ] Sem raw pointers globais (CppCG R.1)
- [ ] Funções são coesas (SRP)
- [ ] Testes unitários para novos componentes
- [ ] Documentação atualizada

---

## 8. Conclusão

A interface gráfica está **bem arquitetada** mas com oportunidades de **melhoria em manutenibilidade e robustez**.

**Prioridades:**
1. ✅ **Imediato:** Eliminar DRY violations (2-3h)
2. ✅ **Semana:** Reduzir P10 em hotspots (3-4h)
3. ✅ **Semana:** Aplicar CppCoreGuidelines (4-5h)

**Benefícios esperados:**
- Redução de 30-40% em linhas duplicadas
- Complexidade cognitiva média < 8 (vs ~15-22 atual)
- 100% compliance CppCoreGuidelines
- Manutenibilidade + 50%

---

**Próximos Passos:**
1. Revisar e aprovar este documento
2. Criar branch `refactor/ui-improvements`
3. Implementar Tier 1 (ColorUtils, Badge, TopBar split)
4. Adicionar testes unitários
5. Update documentation

