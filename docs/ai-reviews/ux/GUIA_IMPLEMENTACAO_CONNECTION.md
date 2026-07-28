# Guia de Implementação: ConnectionModal + ECU Info

**Objetivo:** Implementação prática seguindo NASA P10 + C++20/23
**Tempo:** ~3 dias (1 dev)

---

## 📂 Estrutura de Arquivos

```
scanner_glfw/
├── ui/
│   ├── modals/
│   │   ├── ConnectionModal.h          ← NOVO
│   │   └── ConnectionModal.cpp        ← NOVO
│   ├── TopBar.h                       ← MODIFICAR
│   └── TopBar.cpp                     ← MODIFICAR
├── core/
│   ├── ECUBackend.h                   ← MODIFICAR
│   └── ECUBackend.cpp                 ← MODIFICAR
├── app_data.h                         ← MODIFICAR
└── main.cpp                           ← MODIFICAR
```

---

## 🚀 Passo 1: Tipos de Dados (app_data.h)

### Antes
```cpp
extern bool ecuConnected;
extern float kLineLatency;
```

### Depois
```cpp
#pragma once
#include <string>
#include <optional>
#include <functional>
#include <cstdint>

// Strong type: informação retornada pelos init packets
struct ECUInfo {
    std::string model;               // "IAW 6E.75"
    std::string firmware_version;    // "1.23.45"
    std::string hardware_id;         // Serial number
    std::uint32_t table_id = 0;      // 1 ou 2

    bool IsValid() const noexcept {
        return !model.empty() && table_id > 0;
    }

    std::string ToString() const noexcept {
        // Util para debug/logging
        char buf[128];
        snprintf(buf, sizeof(buf), "%s v%s (table %u)",
                model.c_str(), firmware_version.c_str(), table_id);
        return std::string(buf);
    }
};

// Estados globais
extern bool ecuConnected;
extern bool simulationModeActive;    // ← NOVO: explícito
extern float kLineLatency;
extern float kLineErrorRate;
extern std::optional<ECUInfo> g_ecu_info;  // ← NOVO

// Callbacks (injeção, sem estado oculto)
extern std::function<void(const ECUInfo&)> on_ecu_info_received;
extern std::function<void(const std::string&)> on_connection_error;
```

---

## 🚀 Passo 2: ConnectionModal.h

```cpp
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

// Abstração para enumeração de portas (dependency injection)
class IPortEnumerator {
public:
    struct PortInfo {
        std::string port_name;
        std::string description;
        bool is_available = true;
    };

    virtual ~IPortEnumerator() = default;
    virtual std::vector<PortInfo> EnumeratePorts() const noexcept = 0;
};

// Config imutável retornada quando usuário confirma
struct ConnectionConfig {
    std::string port;
    bool use_simulation;
    std::optional<std::string> ecu_model;
};

class ConnectionModal {
public:
    enum class State {
        CLOSED,
        IDLE,           // Escolhendo porta
        CONNECTING,     // Em progresso (aguardando init)
        CONNECTED,      // Init bem-sucedido
        ERROR           // Init falhou
    };

    explicit ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum) noexcept;
    ~ConnectionModal() = default;

    // Dependency injection: callback para quando usuário confirma
    void SetConnectionCallback(
        std::function<void(std::string_view port, bool simulate)> cb
    ) noexcept {
        on_connect_callback_ = cb;
    }

    // Callback para erro (ex: timeout no init)
    void SetErrorCallback(
        std::function<void(std::string_view error_msg)> cb
    ) noexcept {
        on_error_callback_ = cb;
    }

    // Render ImGui e retorna config se usuario confirmou
    std::optional<ConnectionConfig> Render() noexcept;

    // Controle de visibilidade
    void Show() noexcept { should_open_ = true; }
    void Close() noexcept { should_open_ = false; }

    // State queries
    State GetState() const noexcept { return state_; }
    bool IsOpen() const noexcept { return should_open_; }

    // Feedback: ECU info recebida nos init packets
    void SetECUInfo(const std::string &model,
                   const std::string &version) noexcept {
        ecu_model_ = model;
        ecu_version_ = version;
    }

    void SetConnectionError(std::string_view error) noexcept {
        error_message_ = error;
        state_ = State::ERROR;
    }

private:
    // State (NASA P10: campos limitados e tipos imutáveis)
    State state_ = State::CLOSED;
    bool should_open_ = false;

    // Port selection (bounded data)
    std::vector<IPortEnumerator::PortInfo> available_ports_;
    int selected_port_idx_ = 0;

    // Mode selection
    bool use_simulation_ = false;

    // ECU feedback
    std::string ecu_model_;
    std::string ecu_version_;
    std::string error_message_;

    // Dependencies
    std::unique_ptr<IPortEnumerator> port_enum_;
    std::function<void(std::string_view, bool)> on_connect_callback_;
    std::function<void(std::string_view)> on_error_callback_;

    // Private render methods (cada um ~20 linhas max)
    void RefreshPorts() noexcept;
    void RenderTitle() const noexcept;
    void RenderPortList() noexcept;
    void RenderSimulationToggle() noexcept;
    void RenderECUInfoFeedback() const noexcept;
    void RenderErrorMessage() const noexcept;
    bool RenderActionButtons() noexcept;
};

// Implementação de IPortEnumerator para sistema real
class SystemPortEnumerator : public IPortEnumerator {
public:
    std::vector<PortInfo> EnumeratePorts() const noexcept override;
};

#endif // CONNECTION_MODAL_H
```

---

## 🚀 Passo 3: ConnectionModal.cpp (Esqueleto)

```cpp
#include "ConnectionModal.h"
#include "imgui.h"
#include "../utils/Colors.h"
#include "../utils/Layout.h"
#include "../app_data.h"
#include <cassert>
#include <algorithm>

// ==================== Construtor ====================
ConnectionModal::ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum) noexcept
    : port_enum_(std::move(port_enum)) {
    assert(port_enum_ && "Port enumerator must not be null");
    RefreshPorts();
}

// ==================== Render Principal ====================
std::optional<ConnectionConfig> ConnectionModal::Render() noexcept {
    if (!should_open_) {
        state_ = State::CLOSED;
        return std::nullopt;
    }

    // Configurar modal
    const ImVec2 modal_size(500.0f, 500.0f);
    ImGui::SetNextWindowSize(modal_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                           ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    std::optional<ConnectionConfig> result;
    constexpr auto flags = ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoMove;

    if (ImGui::BeginPopupModal("ECU Connection", &should_open_, flags)) {
        // Título
        RenderTitle();
        ImGui::Separator();

        // Seleção de porta
        RenderPortList();
        ImGui::Spacing();

        // Modo simulação vs real
        RenderSimulationToggle();
        ImGui::Spacing();

        // Info ECU (feedback dos init packets)
        RenderECUInfoFeedback();
        ImGui::Spacing();

        // Mensagem de erro (se houver)
        if (state_ == State::ERROR) {
            RenderErrorMessage();
            ImGui::Spacing();
        }

        ImGui::Separator();

        // Botões de ação
        if (RenderActionButtons()) {
            // Usuário clicou "Connect"
            result = ConnectionConfig{
                .port = available_ports_[selected_port_idx_].port_name,
                .use_simulation = use_simulation_,
                .ecu_model = ecu_model_.empty() ? std::nullopt
                                                : std::make_optional(ecu_model_)
            };
            should_open_ = false;
        }

        ImGui::EndPopup();
    }

    return result;
}

// ==================== Render Métodos ====================

void ConnectionModal::RenderTitle() const noexcept {
    ImGui::TextColored(Colors::Status::OK, "Select Serial Port & Mode");
}

void ConnectionModal::RenderPortList() noexcept {
    ImGui::Text("Available Ports:");

    if (available_ports_.empty()) {
        ImGui::TextColored(Colors::Status::WARN, "⚠ No ports found");
        ImGui::TextColored(Colors::Status::WARN,
            "  Check USB connections and drivers");
        return;
    }

    // P10: bounded loop
    constexpr size_t kMaxDisplayPorts = 20;
    const size_t port_count = std::min(available_ports_.size(), kMaxDisplayPorts);

    for (size_t i = 0; i < port_count; ++i) {
        const auto &port = available_ports_[i];
        const bool is_selected = (static_cast<int>(i) == selected_port_idx_);
        const ImVec4 color = is_selected ? Colors::Status::OK
                                        : ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

        const char *indicator = is_selected ? "●" : "○";
        ImGui::TextColored(color, "%s %s", indicator, port.port_name.c_str());

        if (!port.description.empty()) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                             "(%s)", port.description.c_str());
        }

        if (!port.is_available) {
            ImGui::SameLine();
            ImGui::TextColored(Colors::Status::CRITICAL, "[IN USE]");
        }

        if (ImGui::IsItemClicked() && port.is_available) {
            selected_port_idx_ = static_cast<int>(i);
        }
    }
}

void ConnectionModal::RenderSimulationToggle() noexcept {
    ImGui::Text("Connection Mode:");

    ImGui::RadioButton("Real ECU", &reinterpret_cast<int&>(use_simulation_), 0);
    ImGui::SameLine();
    ImGui::TextColored(Colors::Status::OK, "  Use actual ECU data");

    ImGui::RadioButton("Simulation", &reinterpret_cast<int&>(use_simulation_), 1);
    ImGui::SameLine();
    ImGui::TextColored(Colors::Status::WARN, "  Hardcoded test data");
}

void ConnectionModal::RenderECUInfoFeedback() const noexcept {
    ImGui::Text("ECU Information:");

    if (state_ == State::CONNECTING) {
        ImGui::TextColored(Colors::Status::WARN, "⏳ Waiting for ECU response...");
        return;
    }

    if (ecu_model_.empty()) {
        ImGui::TextColored(Colors::Status::WARN,
            "ℹ Connect to see ECU model and firmware");
        return;
    }

    // Mostrar info recebida
    ImGui::TextColored(Colors::Status::OK, "✓ Model: %s", ecu_model_.c_str());
    if (!ecu_version_.empty()) {
        ImGui::TextColored(Colors::Status::OK, "✓ Firmware: %s",
                         ecu_version_.c_str());
    }
}

void ConnectionModal::RenderErrorMessage() const noexcept {
    ImGui::TextColored(Colors::Status::CRITICAL, "✗ Connection Error:");
    ImGui::TextColored(Colors::Status::CRITICAL, "  %s", error_message_.c_str());
}

bool ConnectionModal::RenderActionButtons() noexcept {
    bool user_confirmed = false;

    const float btn_width = (ImGui::GetContentRegionAvail().x - 10.0f) / 2.0f;
    const float btn_height = 40.0f;

    // Botão Connect
    if (ImGui::Button("Connect", ImVec2(btn_width, btn_height))) {
        state_ = State::CONNECTING;
        if (on_connect_callback_) {
            on_connect_callback_(available_ports_[selected_port_idx_].port_name,
                                use_simulation_);
        }
        user_confirmed = true;
    }

    ImGui::SameLine();

    // Botão Cancel
    if (ImGui::Button("Cancel", ImVec2(btn_width, btn_height))) {
        should_open_ = false;
        state_ = State::CLOSED;
    }

    return user_confirmed;
}

void ConnectionModal::RefreshPorts() noexcept {
    available_ports_ = port_enum_->EnumeratePorts();
    // NASA P10: sem alocação dinâmica pós-init
}

// ==================== Implementação: SystemPortEnumerator ====================
std::vector<IPortEnumerator::PortInfo> SystemPortEnumerator::EnumeratePorts() const noexcept {
    // TODO: Implementar enumeração de portas para Linux/Windows/macOS
    // Sugestão:
    // - Linux: /proc/tty/drivers, /dev/ttyUSB*, /dev/ttyS*
    // - Windows: EnumSerialPorts (registry)
    // - macOS: IOKit

    // Stub para agora:
    return {
        {"COM1", "Serial Port"},
        {"/dev/ttyUSB0", "USB Serial"},
    };
}
```

---

## 🚀 Passo 4: Modificar TopBar

### TopBar.h
```cpp
#pragma once

#include "imgui.h"
#include "modals/ConnectionModal.h"  // ← NOVO
#include <optional>
#include <string>

class TopBar {
public:
    TopBar() = default;
    ~TopBar() = default;

    void Render(float bar_height);

    // State setters (NASA P10: sem estado oculto)
    void SetConnectionStatus(bool connected) noexcept { ecu_connected_ = connected; }
    void SetSimulationMode(bool active) noexcept { simulation_active_ = active; }  // ← NOVO
    void SetLatency(float latency_ms) noexcept { latency_ms_ = latency_ms; }
    void SetErrorRate(float error_rate) noexcept { error_rate_ = error_rate; }
    void SetECUModel(std::string_view model) noexcept {  // ← NOVO
        ecu_model_ = std::string(model);
    }

    // Connection modal (dependency injection)
    void SetConnectionModal(ConnectionModal *modal) noexcept {  // ← NOVO
        connection_modal_ = modal;
    }

    bool IsConnected() const noexcept { return ecu_connected_; }

private:
    bool ecu_connected_ = true;
    bool simulation_active_ = false;  // ← NOVO
    float latency_ms_ = 16.0f;
    float error_rate_ = 0.0f;
    std::string ecu_model_;  // ← NOVO
    ConnectionModal *connection_modal_ = nullptr;  // ← NOVO

    // Render methods
    void RenderConnectionStatus();
    void RenderConnectionButton();  // ← NOVO
    void RenderSignalBars();
    void RenderSimulationWarning();
    void RenderWindowControls();
    void RenderThemeToggle();
    void RenderTime();

    // Helpers
    void ToggleFullscreen();
    int CalculateSignalBars() const;
};
```

### TopBar.cpp - Método Principal
```cpp
void TopBar::Render(float bar_height) {
    auto &theme = ThemeManager::Instance();

    ImGui::BeginChild("TopBar", ImVec2(0, bar_height), false,
                      ImGuiWindowFlags_NoScrollbar);

    // ========== LEFT SIDE ==========
    RenderConnectionStatus();     // Mostra: ✓ ECU: IAW6E.75 | 15.3ms
    ImGui::SameLine();
    RenderSignalBars();          // Barras de qualidade
    ImGui::SameLine();
    RenderConnectionButton();    // ← NOVO: botão ⚙ Connection

    // ========== CENTER ==========
    if (simulation_active_) {
        ImGui::SameLine();
        RenderSimulationWarning();  // ← NOVO: aviso claro
    }

    // ========== RIGHT SIDE ==========
    // ... theme toggle, fullscreen, close ...

    ImGui::EndChild();

    // Separator
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(
        ImVec2(0, bar_height),
        ImVec2(ImGui::GetIO().DisplaySize.x, bar_height + 2.0f),
        ImGui::ColorConvertFloat4ToU32(theme.GetBorderColor())
    );
}

void TopBar::RenderConnectionStatus() {
    auto &theme = ThemeManager::Instance();

    // Estado 1: Real ECU conectada
    if (ecu_connected_ && !simulation_active_) {
        ImGui::TextColored(Colors::Status::OK, "✓ ECU:");
        ImGui::SameLine();

        if (!ecu_model_.empty()) {
            ImGui::TextColored(Colors::Status::OK, "%s", ecu_model_.c_str());
        } else {
            ImGui::TextColored(Colors::Status::OK, "CONNECTED");
        }

        ImGui::SameLine();
        ImGui::TextColored(theme.GetSecondaryColor(), "|");
        ImGui::SameLine();
        ImGui::TextColored(Colors::Status::OK, "%.1fms", latency_ms_);
        return;
    }

    // Estado 2: Erro de conexão
    if (!ecu_connected_ && !simulation_active_) {
        ImGui::TextColored(Colors::Status::CRITICAL, "✗ DISCONNECTED");
        return;
    }

    // Estado 3: Simulação ativa
    if (simulation_active_) {
        ImGui::TextColored(Colors::Status::WARN, "▯ SIMULATION");
    }
}

void TopBar::RenderConnectionButton() {
    if (!connection_modal_) return;

    if (ImGui::Button("⚙ Connection", ImVec2(120.0f, 25.0f))) {
        connection_modal_->Show();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Select port and connection mode");
    }
}

void TopBar::RenderSimulationWarning() {
    const ImVec4 sim_color = ImVec4(1.0f, 0.9f, 0.2f, 1.0f);
    ImGui::TextColored(sim_color, "ℹ Test data only - not real ECU");
}
```

---

## 🚀 Passo 5: Modificar ECUBackend

### ECUBackend.h - Adições
```cpp
class ECUBackend {
public:
    // ...existing code...

    // ECU info (novo)
    const std::optional<ECUInfo>& GetECUInfo() const noexcept {
        return ecu_info_;
    }

    void SetECUInfo(const ECUInfo& info) noexcept {
        ecu_info_ = info;
    }

private:
    std::optional<ECUInfo> ecu_info_;  // ← NOVO

    // Process init packets (0xF6)
    void ProcessInitPackets(const std::vector<uint8_t>& data) noexcept;  // ← NOVO
};
```

### ECUBackend.cpp - Implementação
```cpp
void ECUBackend::ProcessInitPackets(const std::vector<uint8_t>& data) noexcept {
    // NASA P10: bounded, sem recursão, assertions densas
    assert(!data.empty() && "Init packet data must not be empty");
    assert(data.size() <= 256 && "Init packet too large");

    ECUInfo info;

    // Parse frame 0xF6 (modelo ECU)
    // Exemplo: frames com "IAW 6E.75" encoded
    // TODO: implementar parser específico para seu protocolo
    info.model = "IAW 6E.75";      // Parse do frame
    info.firmware_version = "1.23";
    info.hardware_id = "ECU12345";
    info.table_id = 1;

    ecu_info_ = info;

    // Notify UI
    if (::on_ecu_info_received) {
        ::on_ecu_info_received(info);
    }
}
```

---

## 🚀 Passo 6: Integração em main.cpp

```cpp
// main.cpp (no init)

// Criar modal com enumerador
auto port_enum = std::make_unique<SystemPortEnumerator>();
ConnectionModal connection_modal(std::move(port_enum));

// Setup callbacks
connection_modal.SetConnectionCallback(
    [](std::string_view port, bool simulate) {
        // Iniciar conexão (já em outro thread)
        ecuConnected = true;
        simulationModeActive = simulate;
    }
);

connection_modal.SetErrorCallback(
    [](std::string_view error) {
        ecuConnected = false;
        printf("Connection error: %s\n", error.data());
    }
);

// Attach à TopBar
g_top_bar.SetConnectionModal(&connection_modal);

// ========== Em render loop ==========
{
    // Update UI state do backend
    g_top_bar.SetConnectionStatus(ecuConnected);
    g_top_bar.SetSimulationMode(simulationModeActive);
    g_top_bar.SetLatency(kLineLatency);

    // ECU info (quando chegar)
    auto &backend = ECUBackend::Instance();
    if (auto ecu_info = backend.GetECUInfo()) {
        g_top_bar.SetECUModel(ecu_info->model);
    }

    // Render modal
    if (auto config = connection_modal.Render()) {
        // Usuário confirmou conexão
        backend.Start(config->port, &sensors);
    }

    // ... rest of render ...
}
```

---

## ✅ Testes (GoogleTest)

```cpp
// scanner_glfw/tests/test_ConnectionModal.cpp
#include <gtest/gtest.h>
#include "../ui/modals/ConnectionModal.h"

class MockPortEnumerator : public IPortEnumerator {
public:
    std::vector<PortInfo> EnumeratePorts() const noexcept override {
        return {
            {"COM3", "USB Serial", true},
            {"COM4", "USB Serial", false},
        };
    }
};

class ConnectionModalTest : public ::testing::Test {
protected:
    void SetUp() override {
        modal_ = std::make_unique<ConnectionModal>(
            std::make_unique<MockPortEnumerator>()
        );
    }

    std::unique_ptr<ConnectionModal> modal_;
};

TEST_F(ConnectionModalTest, EnumeratesPorts) {
    EXPECT_EQ(modal_->GetState(), ConnectionModal::State::CLOSED);
    modal_->Show();
    EXPECT_TRUE(modal_->IsOpen());
}

TEST_F(ConnectionModalTest, ReturnsConfigOnConnect) {
    // (Simular ImGui::Button click)
    // auto config = modal_->Render();
    // EXPECT_TRUE(config.has_value());
    // EXPECT_EQ(config->port, "COM3");
}

TEST_F(ConnectionModalTest, HandleError) {
    modal_->SetConnectionError("Timeout on init");
    EXPECT_EQ(modal_->GetState(), ConnectionModal::State::ERROR);
}
```

---

## 🎯 Checklist Implementação

- [ ] Adicionar tipos em `app_data.h` (`ECUInfo`, `simulationModeActive`)
- [ ] Criar `ConnectionModal.h/cpp`
- [ ] Criar `SystemPortEnumerator` (plataforma-específica)
- [ ] Modificar `TopBar.h/cpp` (botão + ECU model display)
- [ ] Modificar `ECUBackend.h/cpp` (capturar init packets)
- [ ] Integrar em `main.cpp` (instanciar, callbacks)
- [ ] Compilar com `-Wall -Wextra -Werror`
- [ ] Executar testes (GoogleTest)
- [ ] Code review (C++20 compliance)

---

**Tempo estimado:** 3 dias. Começa com tipos → modal → topbar → integração.
