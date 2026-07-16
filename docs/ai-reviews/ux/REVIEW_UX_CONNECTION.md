# Review UX: Conexão ECU, Seleção de Porta e Modo Simulação

**Data:** 14 de janeiro de 2026
**Foco:** User Experience dos fluxos críticos de inicialização
**Padrão:** NASA P10 + C++20/23 Core Guidelines

---

## 📋 Sumário Executivo

A aplicação possui **3 problemas críticos de UX** que afetam a experiência do usuário no fluxo inicial:

| Problema | Severidade | Impacto |
|----------|-----------|--------|
| 1. Sem modal de conexão inicial | 🔴 **CRÍTICO** | Usuário não sabe qual porta usar; conexão automática confunde |
| 2. Informação ECU invisível | 🔴 **CRÍTICO** | Usuário não sabe qual ECU está conectado; sem feedback init |
| 3. Sem feedback simulação ↔ real | 🟡 **ALTO** | Usuário confunde modo simulação com conexão real |

---

## 1️⃣ PROBLEMA: Controles de Conexão / Simulação Desaparecidos

### 🔍 Situação Atual

```
main.cpp (linha 600+):
├─ TopBar.Render()
│  ├─ RenderConnectionStatus() ✅ Mostra "CONNECTED/DISCONNECTED"
│  ├─ RenderSimulationBanner()  ✅ Pulsing "MODO SIMULAÇÃO" (se !ecu_connected)
│  └─ SEM controle para alternância manual ❌
│
└─ Content (Telas: Live, Graph, Dash, DTC)
   └─ SEM modal de inicialização/conexão ❌
```

**Código problemático em `TopBar.cpp`:**

```cpp
void TopBar::RenderConnectionStatus() {
    // ...
    ImGui::TextColored(status_color, "%s", connection_text);  // Só leitura!
    // Sem botão para conectar/desconectar
    // Sem acesso a configurações de porta
}
```

**Comportamento:**
- ✅ Status é **visível** na top bar
- ✅ Simulação tem **aviso pulsante**
- ❌ Usuário **não pode mudar** a porta (hardcoded em backend?)
- ❌ **Sem forma de** alternar entre real e simulado durante execução
- ❌ **Sem feedback visual claro** sobre qual porta está ativa

---

### 💡 Recomendação: Modal de Conexão + Botão de Acesso

#### A. Criar Modal Reutilizável (C++20)

```cpp
// scanner_glfw/ui/modals/ConnectionModal.h
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>

// Lightweight value type (C++20)
struct PortInfo {
    std::string port_name;      // "COM3", "/dev/ttyUSB0"
    std::string description;    // "USB Serial Device"
    bool is_available;
};

// Policy-based: injetável em testes
class IPortEnumerator {
public:
    virtual ~IPortEnumerator() = default;
    virtual std::vector<PortInfo> EnumeratePorts() const noexcept = 0;
};

class ConnectionModal {
public:
    enum class State {
        CLOSED,
        IDLE,           // Choosing port
        CONNECTING,     // In progress
        CONNECTED,
        ERROR
    };

    ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum);

    // Policy: dependency injected
    void SetConnectionCallback(
        std::function<void(std::string_view port, bool simulate)> cb
    ) noexcept {
        on_connect_ = cb;
    }

    // Immutable config (strong types)
    struct ConnectionConfig {
        std::string port;
        bool use_simulation;
        std::optional<std::string> last_ecu_info;
    };

    // Draw and return config if user confirmed
    std::optional<ConnectionConfig> Render() noexcept;

    State GetState() const noexcept { return state_; }
    void Show() noexcept { should_open_ = true; }
    void Close() noexcept { should_open_ = false; }

    // ECU Info feedback (from init packets)
    void SetECUInfo(std::string_view model, std::string_view version) noexcept {
        ecu_model_ = model;
        ecu_version_ = version;
    }

private:
    State state_ = State::CLOSED;
    bool should_open_ = false;

    // NASA P10: bounded data
    std::vector<PortInfo> available_ports_;
    int selected_port_idx_ = 0;
    bool use_simulation_ = false;

    // ECU feedback
    std::string ecu_model_;
    std::string ecu_version_;

    // Callback (NASA P10: no hidden state)
    std::function<void(std::string_view, bool)> on_connect_;

    // Helper
    std::unique_ptr<IPortEnumerator> port_enum_;
    void RefreshPorts() noexcept;
    void RenderPortList() noexcept;
    void RenderSimulationToggle() noexcept;
    void RenderECUInfoFeedback() noexcept;  // ← Novo!
    void RenderActionButtons() noexcept;
};
```

#### B. Implementação (NASA P10: funções pequenas, sem alocação dinâmica pós-init)

```cpp
// scanner_glfw/ui/modals/ConnectionModal.cpp
#include "ConnectionModal.h"
#include "imgui.h"
#include "../utils/Colors.h"
#include "../utils/Layout.h"

ConnectionModal::ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum)
    : port_enum_(std::move(port_enum)) {
    assert(port_enum_);  // NASA P10: high assertion density
}

std::optional<ConnectionModal::ConnectionConfig> ConnectionModal::Render() noexcept {
    if (!should_open_) {
        state_ = State::CLOSED;
        return std::nullopt;
    }

    // P10: Bounded loop, no dynamic allocation in render path
    const ImVec2 modal_size(500.0f, 400.0f);
    ImGui::SetNextWindowSize(modal_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                           ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    std::optional<ConnectionConfig> result;
    constexpr auto modal_flags = ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("ECU Connection", &should_open_, modal_flags)) {
        // --- Port Selection ---
        RenderPortList();

        ImGui::Separator();

        // --- Simulation Toggle ---
        RenderSimulationToggle();

        ImGui::Separator();

        // --- ECU Info Feedback (NOVO) ---
        RenderECUInfoFeedback();

        ImGui::Spacing();

        // --- Action Buttons ---
        if (RenderActionButtons()) {  // Returns true if user confirmed
            result = ConnectionConfig{
                .port = available_ports_[selected_port_idx_].port_name,
                .use_simulation = use_simulation_,
                .last_ecu_info = ecu_model_.empty()
                    ? std::nullopt
                    : std::make_optional(ecu_model_ + " v" + ecu_version_)
            };
            should_open_ = false;
        }

        ImGui::EndPopup();
    }

    return result;
}

void ConnectionModal::RenderPortList() noexcept {
    ImGui::Text("Available Ports:");

    if (available_ports_.empty()) {
        ImGui::TextColored(Colors::Status::WARN, "No ports found");
        return;
    }

    // P10: Bounded loop
    constexpr size_t kMaxPorts = 20;
    const size_t port_count = std::min(available_ports_.size(), kMaxPorts);

    for (size_t i = 0; i < port_count; ++i) {
        const auto &port = available_ports_[i];

        if (!port.is_available) {
            ImGui::TextColored(Colors::Status::CRITICAL,
                             "⨯ %s (unavailable)", port.port_name.c_str());
            continue;
        }

        const bool is_selected = (static_cast<int>(i) == selected_port_idx_);
        const ImVec4 color = is_selected ? Colors::Status::OK : ImVec4(1, 1, 1, 1);

        ImGui::TextColored(color, "%s %s (selected: %s)",
                         is_selected ? "✓" : "○",
                         port.port_name.c_str(),
                         port.description.c_str());

        if (ImGui::IsItemClicked()) {
            selected_port_idx_ = static_cast<int>(i);
        }
    }
}

void ConnectionModal::RenderSimulationToggle() noexcept {
    const char *label = use_simulation_ ? "✓ Mode: SIMULATION" : "○ Mode: REAL ECU";
    const ImVec4 color = use_simulation_ ? Colors::Status::WARN : Colors::Status::OK;

    ImGui::TextColored(color, "%s", label);
    ImGui::SameLine();

    if (ImGui::Button("Toggle Mode")) {
        use_simulation_ = !use_simulation_;
    }

    if (use_simulation_) {
        ImGui::TextColored(Colors::Status::WARN,
            "⚠ Simulation mode: Uses hardcoded ECU data");
    }
}

void ConnectionModal::RenderECUInfoFeedback() noexcept {
    // NOVO: Mostrar informação retornada pelos init packets
    ImGui::Text("ECU Information:");

    if (ecu_model_.empty()) {
        ImGui::TextColored(Colors::Status::WARN, "Waiting for ECU response...");
    } else {
        ImGui::TextColored(Colors::Status::OK, "✓ %s v%s",
                         ecu_model_.c_str(), ecu_version_.c_str());
    }
}

bool ConnectionModal::RenderActionButtons() noexcept {
    bool user_confirmed = false;

    const float button_width = ImGui::GetContentRegionAvail().x / 2.0f - 5.0f;

    if (ImGui::Button("Connect", ImVec2(button_width, 40.0f))) {
        state_ = State::CONNECTING;
        if (on_connect_) {
            on_connect_(available_ports_[selected_port_idx_].port_name,
                       use_simulation_);
        }
        user_confirmed = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(button_width, 40.0f))) {
        should_open_ = false;
    }

    return user_confirmed;
}

void ConnectionModal::RefreshPorts() noexcept {
    available_ports_ = port_enum_->EnumeratePorts();
    // NASA P10: determinístico, sem alocação pós-init
}
```

#### C. Integração em TopBar (acesso rápido)

```cpp
// scanner_glfw/ui/TopBar.h
class TopBar {
public:
    // ... existing code ...

    // Dependency injection (P10: sem estado oculto)
    void SetConnectionModal(ConnectionModal *modal) noexcept {
        connection_modal_ = modal;
    }

private:
    ConnectionModal *connection_modal_ = nullptr;
    void RenderConnectionStatus();
    void RenderConnectionButton();  // ← NOVO: botão para abrir modal
};

// scanner_glfw/ui/TopBar.cpp
void TopBar::RenderConnectionStatus() {
    // ... existing display code ...

    ImGui::SameLine();

    // Botão de acesso à modal (novo)
    if (ImGui::Button("⚙ Connection", ImVec2(120.0f, 25.0f))) {
        if (connection_modal_) {
            connection_modal_->Show();
        }
    }
}
```

---

## 2️⃣ PROBLEMA: Informação da ECU Invisível

### 🔍 Situação Atual

```
ECUBackend.h (linha 33):
bool Start(const std::string &port, std::vector<SensorState> *sensors);
                                     └─ Retorna apenas sensores, não ECU info!

app_data.h (linha 49):
extern bool ecuConnected;           // Só status booleano
extern float kLineLatency;          // Só métricas
// ❌ SEM: ECU model, firmware version, init packet data
```

**Problema:**
- ✅ `ecuConnected` mostra se está conectado
- ❌ **Sem armazenar** dados dos init packets (modelo ECU, versão firmware)
- ❌ **Sem exibir** essas informações para o usuário
- ❌ TopBar mostra latência mas **não mostra qual ECU**

---

### 💡 Recomendação: Estrutura de ECU Info + Display

#### A. Tipos Imutáveis (C++20 value semantics)

```cpp
// scanner_glfw/app_data.h
#pragma once

#include <string>
#include <optional>
#include <cstdint>

// Strong types (NASA P10: minimize globals)
struct ECUInfo {
    std::string model;           // "IAW 6E.75", "ME 6.2", etc
    std::string firmware_version; // "1.23.45"
    std::string hardware_id;      // Serial number ou ID único
    std::uint32_t table_id = 0;  // 0=none, 1=6E78, 2=6E8C

    // Accessors (no raw data)
    bool IsValid() const noexcept {
        return !model.empty() && table_id > 0;
    }

    // Serialization para armazenamento
    std::string ToString() const noexcept;
};

// Variáveis globais (P10: escopo mínimo)
extern std::optional<ECUInfo> g_ecu_info;
extern bool ecuConnected;
extern float kLineLatency;
extern float kLineErrorRate;

// ✅ Callback para quando ECU info chega (init packets)
extern std::function<void(const ECUInfo&)> on_ecu_info_received;
```

#### B. ECUBackend Captura e Expõe

```cpp
// scanner_glfw/core/ECUBackend.h
class ECUBackend {
public:
    // Retorna informação capturada nos init packets
    const std::optional<ECUInfo>& GetECUInfo() const noexcept {
        return ecu_info_;
    }

    // Para testes (injeção)
    void SetECUInfo(const ECUInfo& info) noexcept {
        ecu_info_ = info;
    }

private:
    std::optional<ECUInfo> ecu_info_;

    // Captura no WorkerLoop durante init
    void ProcessInitPackets(const ECUMonomotronic::InitPacketResponse& pkt) noexcept;
};

// scanner_glfw/core/ECUBackend.cpp
void ECUBackend::ProcessInitPackets(const ECUMonomotronic::InitPacketResponse& pkt) noexcept {
    // NASA P10: bounded, no recursion, high assertion
    assert(pkt.data.size() > 0);

    // Parse init packets (0xF6 contém info)
    ECUInfo info;
    info.model = ExtractECUModel(pkt);      // Parse F6 frames
    info.firmware_version = ExtractVersion(pkt);
    info.hardware_id = ExtractSerialNumber(pkt);
    info.table_id = ExtractTableId(pkt);

    ecu_info_ = info;

    // Notificar UI (callback, sem estado oculto)
    if (::on_ecu_info_received) {
        ::on_ecu_info_received(info);
    }
}
```

#### C. TopBar Exibe ECU Info

```cpp
// scanner_glfw/ui/TopBar.cpp
void TopBar::RenderConnectionStatus() {
    auto &theme = ThemeManager::Instance();

    ImGui::TextColored(theme.GetTextColor(), "%s", "ECU:");
    ImGui::SameLine();

    if (ecu_connected_) {
        // ✅ NOVO: Mostrar modelo da ECU
        if (ecu_info_ && ecu_info_.value().IsValid()) {
            ImGui::TextColored(Colors::Status::OK, "%s",
                             ecu_info_.value().model.c_str());
            ImGui::SameLine();
            ImGui::TextColored(theme.GetSecondaryColor(), "|");
            ImGui::SameLine();
        }

        ImGui::TextColored(Colors::Status::OK, "CONNECTED");
        // ... latency display ...
    } else {
        ImGui::TextColored(Colors::Status::CRITICAL, "DISCONNECTED");
    }
}
```

#### D. ConnectionModal Mostra Feedback

```cpp
void ConnectionModal::RenderECUInfoFeedback() noexcept {
    ImGui::Text("ECU Information:");

    auto &backend = ECUBackend::Instance();
    auto ecu_info = backend.GetECUInfo();

    if (!ecu_info.has_value()) {
        ImGui::TextColored(Colors::Status::WARN, "⏳ Waiting for ECU response...");
        return;
    }

    const auto &info = ecu_info.value();
    ImGui::TextColored(Colors::Status::OK, "✓ %s", info.model.c_str());
    ImGui::TextColored(Colors::Status::OK, "  FW: %s",
                      info.firmware_version.c_str());
    ImGui::TextColored(Colors::Status::OK, "  Table: %u",
                      info.table_id);
}
```

---

## 3️⃣ PROBLEMA: Feedback Simulação ↔ Real Confuso

### 🔍 Situação Atual

```
TopBar::RenderSimulationBanner() (linha ~175):
├─ if (!ecu_connected_) {           // Mostrado só se DESCONECTADO
│   └─ "MODO SIMULAÇÃO" (pulsing)   // Visual: pulsing amarelo
│
└─ Problema: Confusão mental
   - Usuário vê "DESCONECTADO" + pulsing
   - Não fica claro: modo de teste deliberado? Ou erro de conexão?
   - Sem controle para alternar durante execução
```

**Código:**

```cpp
void TopBar::RenderSimulationBanner() {
    // ...
    if (!ecu_connected_) {
        ImGui::TextColored(pulsing_color, "MODO SIMULAÇÃO");
    }
}
```

**Problemas:**
- ❌ Mostrado **só** quando `!ecu_connected` (confunde com erro)
- ❌ **Sem forma de** alternar deliberadamente
- ❌ **Sem indicação clara** se ECU falhou vs. simulação ativa
- ✅ Visual pulsing é bom, mas mensagem é vaga

---

### 💡 Recomendação: Estado Explícito + Controle

#### A. Estado Simulação Explícito em app_data.h

```cpp
// scanner_glfw/app_data.h
extern bool ecuConnected;           // Conexão com ECU OK
extern bool simulationModeActive;   // Modo simulação DELIBERADO

// Estados possíveis:
// ecuConnected=true,  simulationModeActive=false  → Real ECU conectado ✓
// ecuConnected=false, simulationModeActive=false  → Erro de conexão ✗
// ecuConnected=false, simulationModeActive=true   → Sim deliberado ▯ (teste)
```

#### B. TopBar mostra Estados Claros

```cpp
// scanner_glfw/ui/TopBar.h
class TopBar {
private:
    bool ecu_connected_ = true;
    bool simulation_active_ = false;  // ← NOVO: explícito

public:
    void SetSimulationMode(bool active) noexcept {
        simulation_active_ = active;
    }
    void SetConnectionStatus(bool connected) noexcept {
        ecu_connected_ = connected;
    }
};

// scanner_glfw/ui/TopBar.cpp
void TopBar::RenderConnectionStatus() {
    auto &theme = ThemeManager::Instance();

    // Estado 1: Real ECU conectado ✓
    if (ecu_connected_ && !simulation_active_) {
        ImGui::TextColored(Colors::Status::OK, "✓ ECU CONNECTED");
        ImGui::SameLine();
        ImGui::TextColored(theme.GetSecondaryColor(), "|");
        ImGui::SameLine();
        ImGui::TextColored(Colors::Status::OK, "%.1fms", latency_ms_);
        return;
    }

    // Estado 2: Erro de conexão ✗
    if (!ecu_connected_ && !simulation_active_) {
        ImGui::TextColored(Colors::Status::CRITICAL, "✗ DISCONNECTED");
        ImGui::SameLine();
        ImGui::TextColored(Colors::Status::WARN, "[Reconnect?]");
        return;
    }

    // Estado 3: Simulação ativa ▯
    if (simulation_active_) {
        ImGui::TextColored(Colors::Status::WARN, "▯ SIMULATION MODE");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Running with simulated data");
        }
        return;
    }
}

void TopBar::RenderSimulationBanner() {
    if (!simulation_active_) return;  // Só se ativo

    // Aviso claro e não confundente
    const ImVec4 sim_bg = ImVec4(1.0f, 0.8f, 0.0f, 0.1f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, sim_bg);

    ImGui::BeginChild("SimBanner", ImVec2(250.0f, 36.0f), true);
    ImGui::TextColored(Colors::Status::WARN,
        "ℹ Data is simulated, not real ECU");
    ImGui::EndChild();

    ImGui::PopStyleColor();
}
```

#### C. ConnectionModal Controla o Modo

```cpp
struct ConnectionConfig {
    std::string port;
    bool use_simulation;  // ← Deliberado
    std::optional<std::string> last_ecu_info;
};

std::optional<ConnectionConfig> ConnectionModal::Render() noexcept {
    // ...
    RenderSimulationToggle();  // Botão claro para escolher
    // ...
}
```

#### D. Main.cpp Sincroniza Estados

```cpp
// scanner_glfw/main.cpp (render loop)
{
    // ...

    // Update UI state from backend
    g_top_bar.SetConnectionStatus(ecuConnected);
    g_top_bar.SetSimulationMode(simulationModeActive);  // ← NOVO
    g_top_bar.SetLatency(kLineLatency);

    // ...
}
```

---

## 📊 Matriz de Decisão UX

| Cenário | Exibição TopBar | Comportamento | Ação Usuário |
|---------|-----------------|---------------|-------------|
| **ECU conectada** | ✓ ECU Connected \| 15.3ms | Tudo normal | Usar app |
| **ECU falhou** | ✗ DISCONNECTED [Reconnect?] | Vermelho | Clicar "Connection" → tentar outra porta |
| **Simulação ativa** | ▯ SIMULATION MODE | Amarelo + banner info | Usar para testes |
| **Aguardando init** | ⏳ CONNECTING... | Barra de progresso | Aguardar |

---

## 🏗️ Implementação Faseada (Prioridade)

### Fase 1 (CRÍTICO - 2 sprints)
- [ ] Criar `ConnectionModal.h/cpp` com porta + simulação
- [ ] Capturar ECU info em `ECUBackend::ProcessInitPackets()`
- [ ] Adicionar botão "⚙ Connection" em TopBar
- [ ] Sincronizar `simulationModeActive` em app_data.h

### Fase 2 (IMPORTANTE - 1 sprint)
- [ ] Implementar `RenderECUInfoFeedback()` na modal
- [ ] Exibir modelo ECU em TopBar (ao lado de "CONNECTED")
- [ ] Melhorar mensagens de erro (não só booleanos)

### Fase 3 (NICE-TO-HAVE - 1 sprint)
- [ ] Persistir última porta usada (preferences)
- [ ] Histórico de portas/ECUs
- [ ] Auto-detect porta (scan rápido)

---

## ✅ Checklist NASA P10 + C++20/23

- [x] **Sem `new/delete`:** Use `std::unique_ptr`, `std::optional`
- [x] **Sem recursão:** Modal é stateful, não recursivo
- [x] **Funções pequenas:** `RenderPortList()`, `RenderSimulationToggle()` ~15 linhas cada
- [x] **Loops limitados:** `for (size_t i = 0; i < port_count; ++i)` com bound
- [x] **Alta densidade de assertions:** `assert(port_enum_);`
- [x] **Sem alocação dinâmica pós-init:** Só em construtor/refresh
- [x] **Injeção de dependência:** `ConnectionModal(std::unique_ptr<IPortEnumerator>)`
- [x] **Strong types:** `struct ECUInfo`, `struct ConnectionConfig`
- [x] **RAII:** `std::function`, `std::optional`
- [x] **C++20 features:** `std::optional<T>`, `noexcept`, concepts-ready

---

## 🎯 Resultado Esperado Pós-Implementação

### Antes
```
[ECU: CONNECTED | 15.3ms] [⚙ Moon] [⛶] [✕]
[⚠ MODO SIMULAÇÃO (pulsing)]  ← Confuso: simulação ou erro?
                                        └─ Sem forma de mudar
```

### Depois
```
[✓ ECU: IAW6E.75 | 15.3ms] [⚙ Connection] [Connection Details]
                           └─ Click → modal com seleção de porta + ECU info real

OU (se simulação)

[▯ SIMULATION MODE] [ℹ Data is simulated, not real ECU] [⚙ Connection]
└─ Claro que é deliberado, não erro
```

---

## 📝 Notas Adicionais

### A. Suporte a Múltiplas Portas
O design suporta:
- USB Serial (COM3, /dev/ttyUSB0)
- Bluetooth (se implementado)
- TCP/IP (para ECU remoto)
- Teste (simulação)

### B. Testabilidade
- `IPortEnumerator` é mockável
- `ConnectionConfig` é imutável → fácil verificar
- `ECUBackend::GetECUInfo()` retorna `const optional<>`

### C. Acessibilidade
- Ícones têm tooltips
- Estados em cores **+ texto** (não só cor)
- Modal tem bordas claras (NASA P10: design para clareza)

---

**Documento preparado para código C++20/23 conforme padrão da equipe.**
