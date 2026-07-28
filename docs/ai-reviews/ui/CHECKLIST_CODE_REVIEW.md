# Checklist Code Review: NASA P10 + C++20/23

**Aplicável a:**
- `ConnectionModal.h/cpp`
- `TopBar.h/cpp` (modificações)
- `ECUBackend.h/cpp` (modificações)
- `app_data.h` (tipos novos)

---

## 🔍 NASA Power of 10 (P10)

### ✅ 1. Avoid Complex Control Flow
```cpp
// ❌ NÃO (recursão)
void ProcessInitPackets(const auto& data) {
    if (data.empty()) return;
    if (state == WAITING) ProcessInitPackets(rest);  // Recursão!
}

// ✅ SIM (loop simples)
void ProcessInitPackets(const std::vector<uint8_t>& data) noexcept {
    assert(!data.empty());
    for (size_t i = 0; i < data.size(); ++i) {
        ProcessByte(data[i]);
    }
}
```

**Checklist:**
- [ ] Sem `goto`
- [ ] Sem recursão
- [ ] Máx 3 níveis de nesting
- [ ] `if/else` simples, sem ternários aninhados

---

### ✅ 2. All Loops Must Have Fixed Bounds

```cpp
// ❌ NÃO (unbounded)
for (auto it = available_ports_.begin(); it != available_ports_.end(); ++it) {
    // Pode ser infinito?
}

// ✅ SIM (bounded)
constexpr size_t kMaxPorts = 20;
const size_t count = std::min(available_ports_.size(), kMaxPorts);
for (size_t i = 0; i < count; ++i) {
    RenderPort(available_ports_[i]);
}
```

**Checklist:**
- [ ] Todos os `for` têm bound `< MAX`
- [ ] Constantes `constexpr` para limites
- [ ] `while` tem condição clara (não `true`)
- [ ] Sem `auto` em loops (use `int i`, `size_t i`)

---

### ✅ 3. No Dynamic Memory Allocation After Initialization

```cpp
// ❌ NÃO (malloc no WorkerLoop!)
void ConnectionModal::RefreshPorts() {
    // NÃO FAZER:
    available_ports_.clear();
    auto ports = new std::vector<PortInfo>();  // Alocação dinâmica!
    // ...
}

// ✅ SIM (pre-allocated)
class ConnectionModal {
    std::vector<IPortEnumerator::PortInfo> available_ports_;  // Pré-alocado
    void RefreshPorts() noexcept {
        available_ports_ = port_enum_->EnumeratePorts();  // Reatribuição, OK
    }
};
```

**Checklist:**
- [ ] Construtor aloca (`std::unique_ptr`, `std::vector` com reserve)
- [ ] Sem `new`/`delete` em métodos
- [ ] Sem alocação em `Render()`, `Update()`, callbacks
- [ ] OK: `vector::clear()`, `=` assignment

---

### ✅ 4. Functions Should Be Short (P10: ~200 LoC max, prefira 50)

```cpp
// ❌ NÃO (700+ linhas)
std::optional<ConnectionConfig> ConnectionModal::Render() {
    // Port selection + simulação + feedback + buttons
    // ... 700 linhas tudo junto ...
}

// ✅ SIM (20 linhas, delegado)
std::optional<ConnectionConfig> ConnectionModal::Render() noexcept {
    if (!should_open_) return std::nullopt;

    if (ImGui::BeginPopupModal("ECU Connection", ...)) {
        RenderTitle();           // ~5 linhas
        RenderPortList();        // ~15 linhas
        RenderSimulationToggle(); // ~10 linhas
        RenderECUInfoFeedback();  // ~8 linhas
        if (RenderActionButtons()) { // ~12 linhas
            // ...
        }
        ImGui::EndPopup();
    }
    return result;
}
```

**Checklist:**
- [ ] Nenhuma função > 50 linhas
- [ ] Max 20 variáveis locais por função
- [ ] Uma responsabilidade por função
- [ ] Nome descritivo (RenderPortList, not DrawUI)

---

### ✅ 5. Limit Assertions Density (High)

```cpp
// ❌ NÃO (sem assertions)
void ProcessInitPackets(const std::vector<uint8_t>& data) {
    ECUInfo info;
    info.model = data[0];      // E se data[0] não existe?
    // ...
}

// ✅ SIM (assertiva)
void ProcessInitPackets(const std::vector<uint8_t>& data) noexcept {
    assert(!data.empty() && "Init packet data must not be empty");
    assert(data.size() <= 256 && "Init packet too large");

    ECUInfo info;
    if (data.size() > 0) {
        info.model = ExtractECUModel(data);
        assert(!info.model.empty() && "ECU model must be valid");
    }
}
```

**Checklist:**
- [ ] Pre-conditions checked com `assert`
- [ ] Post-conditions checked (retorno válido)
- [ ] Invariantes no construtor (ex: `assert(port_enum_)`)
- [ ] Mensagens descritivas em assertions

---

### ✅ 6. Restrict Scope (Minimize Globals)

```cpp
// ❌ NÃO (muitos globals)
extern bool ecuConnected;
extern bool simulationModeActive;
extern ECUInfo ecu_info;
extern std::vector<PortInfo> available_ports;  // Oops, global!

// ✅ SIM (encapsulado)
class ConnectionModal {
private:
    std::vector<IPortEnumerator::PortInfo> available_ports_;  // Membro
    bool use_simulation_ = false;                              // Membro
};

extern std::optional<ECUInfo> g_ecu_info;  // OK: minimal global
extern bool ecuConnected;                   // OK: status
extern bool simulationModeActive;           // OK: modo
```

**Checklist:**
- [ ] Mínimo de `extern` variáveis (≤ 10)
- [ ] Dados em classes, não globals
- [ ] Callbacks injetáveis, não estado oculto
- [ ] Pré-fix `g_` para globals (ex: `g_ecu_info`)

---

## 🔍 C++20/23 Core Guidelines

### ✅ 1. RAII (Resource Acquisition Is Initialization)

```cpp
// ❌ NÃO (raw pointer)
class ConnectionModal {
    IPortEnumerator *port_enum_;  // Quem deleta?
};

// ✅ SIM (unique_ptr)
class ConnectionModal {
    std::unique_ptr<IPortEnumerator> port_enum_;
};

// No construtor:
ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum) noexcept
    : port_enum_(std::move(port_enum)) {
    assert(port_enum_);
}
```

**Checklist:**
- [ ] Sem raw pointers (`*`) para ownership
- [ ] Use `std::unique_ptr`, `std::shared_ptr`
- [ ] `std::optional<T>` para "maybe values"
- [ ] Containers (`vector`, `string`) para coleções

---

### ✅ 2. Value Types & Immutability

```cpp
// ❌ NÃO (estado mutável)
struct ECUInfo {
    std::string model;
    void SetModel(const std::string& m) { model = m; }  // Mutável
};

// ✅ SIM (value semantics, construtor)
struct ECUInfo {
    std::string model;
    std::string firmware_version;

    // Construído uma vez, não mutável
    ECUInfo(std::string m, std::string v)
        : model(std::move(m)), firmware_version(std::move(v)) {}

    bool IsValid() const noexcept { return !model.empty(); }
};

// Retornado como optional
std::optional<ConnectionConfig> GetConfig() const noexcept {
    return ConnectionConfig{port, use_simulation};
}
```

**Checklist:**
- [ ] Estruturas são value types (`struct`, não `class` com getters)
- [ ] Sem setters (inicializar via construtor)
- [ ] Use `const` para imutabilidade
- [ ] `noexcept` em getters

---

### ✅ 3. Dependency Injection (No Hidden Singletons)

```cpp
// ❌ NÃO (singleton global)
class ConnectionModal {
    static ConnectionModal& Instance() {
        static ConnectionModal instance;
        return instance;
    }
};

// ✅ SIM (injetado)
class ConnectionModal {
    explicit ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum) noexcept;
};

// Uso:
auto port_enum = std::make_unique<SystemPortEnumerator>();
ConnectionModal modal(std::move(port_enum));
```

**Checklist:**
- [ ] Sem singletons (static Instance())
- [ ] Dependências passadas no construtor
- [ ] Interfaces (`IPortEnumerator`) para testes
- [ ] Callbacks via `std::function<>`, não estado global

---

### ✅ 4. Smart Pointers, No Raw `new/delete`

```cpp
// ❌ NÃO
auto port_enum = new SystemPortEnumerator();
ConnectionModal modal(port_enum);
// delete modal;  // Oops, quem deleta?

// ✅ SIM
auto port_enum = std::make_unique<SystemPortEnumerator>();
ConnectionModal modal(std::move(port_enum));
// Automático, sem leak
```

**Checklist:**
- [ ] Zero `new` (use `std::make_unique`, `std::make_shared`)
- [ ] Zero `delete` (RAII cuida)
- [ ] `T*` para non-owning (ex: callback params)
- [ ] `const T*` para dados imutáveis

---

### ✅ 5. `noexcept` Everywhere (ou documentado)

```cpp
// ❌ NÃO
std::optional<ConnectionConfig> Render() {  // Pode lançar? Não fica claro
    // ...
}

// ✅ SIM (não lança)
std::optional<ConnectionConfig> Render() noexcept {
    // Garantido: sem exceção
}

// ✅ SIM (com razão documentada)
void SetConnectionCallback(std::function<...> cb) noexcept {
    on_connect_callback_ = cb;  // std::function pode lançar, mas OK
}

// ✅ OK: callback pode lançar (é externo)
std::function<void(std::string_view, bool)> on_connect_callback_;
```

**Checklist:**
- [ ] Getters: `noexcept`
- [ ] Setters simples: `noexcept`
- [ ] Métodos UI (`Render`): `noexcept`
- [ ] Callbacks: documentado se can throw

---

### ✅ 6. Strong Types & Semantics

```cpp
// ❌ NÃO (tipos genéricos)
bool Connect(const std::string& port, const std::string& ecu_model);

// ✅ SIM (strong types)
struct PortName {
    std::string value;
};

struct ECUModel {
    std::string value;
};

bool Connect(const PortName& port, const ECUModel& model);
```

**Para este projeto (pode ser simples):**
```cpp
struct ConnectionConfig {
    std::string port;              // OK: claro
    bool use_simulation;            // OK: booleano
    std::optional<std::string> ecu_model;
};

struct ECUInfo {
    std::string model;
    std::string firmware_version;
};
```

**Checklist:**
- [ ] `std::string` ao invés de `const char*`
- [ ] `std::optional<T>` para "maybe"
- [ ] `struct` para grupos de dados relacionados
- [ ] Nomes descritivos (ECUInfo, not Data)

---

## 🔍 Específicos para Este Projeto

### ✅ ImGui Integration

```cpp
// ❌ NÃO (ImGui calls em lógica)
void ConnectionModal::ProcessInput() {
    if (ImGui::IsKeyPressed('C')) {
        Connect();
    }
}

// ✅ SIM (ImGui separado de lógica)
std::optional<ConnectionConfig> Render() noexcept {
    if (ImGui::Button("Connect")) {
        return ConnectionConfig{...};
    }
    return std::nullopt;  // UI sem effects colaterais
}
```

**Checklist:**
- [ ] ImGui calls só em `Render()` ou `RenderXXX()` helpers
- [ ] Lógica em métodos puros (`SetECUInfo`, `GetState`)
- [ ] `noexcept` em Render (ImGui é robusto)

---

### ✅ Thread Safety (ECUBackend)

```cpp
// ❌ NÃO (race condition)
void ECUBackend::ProcessInitPackets(const std::vector<uint8_t>& data) {
    ecu_info_ = ECUInfo{...};  // Main thread?? Backend thread??
}

// ✅ SIM (mutex protege)
void ECUBackend::ProcessInitPackets(const std::vector<uint8_t>& data) noexcept {
    std::lock_guard<std::mutex> lock(ecu_info_mutex_);
    ecu_info_ = ECUInfo{...};

    // Callback fora do mutex
    if (::on_ecu_info_received) {
        ::on_ecu_info_received(ecu_info_.value());
    }
}
```

**Checklist:**
- [ ] `ecu_info_` protegido por mutex (BackendThread modifica)
- [ ] Main thread lê via `GetECUInfo()` (const, thread-safe)
- [ ] Callbacks fora de locks (evita deadlock)

---

## 📝 Template de Code Review

```markdown
## Code Review: ConnectionModal.h

### NASA P10
- [ ] No recursion ✓
- [ ] All loops bounded ✓
- [ ] No dynamic alloc after init ✓
- [ ] Functions < 50 LoC ✓
- [ ] High assertion density ✓

### C++20/23
- [ ] RAII (unique_ptr) ✓
- [ ] Value types ✓
- [ ] No raw pointers ✓
- [ ] noexcept ✓
- [ ] std::optional, std::function ✓

### ImGui & Threading
- [ ] ImGui calls isolated ✓
- [ ] Thread-safe ECU info ✓
- [ ] Callbacks testable ✓

### Testing
- [ ] MockPortEnumerator works ✓
- [ ] All states tested ✓
- [ ] Error paths covered ✓

### Build
- [ ] Compiles with -Wall -Wextra -Werror ✓
- [ ] No warnings ✓
- [ ] clang-tidy clean ✓

### Final
- [ ] Code review approved ✓
- [ ] Tests pass ✓
- [ ] READY TO MERGE ✓
```

---

## 🚨 Common Violations to Watch

### ❌ Anti-Pattern 1: Global Mutable State
```cpp
// Nope:
static ConnectionModal* g_modal = nullptr;
g_modal->Show();  // From anywhere, hidden dependency

// Yes:
ConnectionModal modal;
g_top_bar.SetConnectionModal(&modal);  // Explicit
```

### ❌ Anti-Pattern 2: Raw Loops Without Bounds
```cpp
// Nope:
while (!available_ports_.empty()) {
    // Remove, process, can be infinite?
}

// Yes:
constexpr size_t kMaxPorts = 20;
for (size_t i = 0; i < std::min(available_ports_.size(), kMaxPorts); ++i) {
    ProcessPort(available_ports_[i]);
}
```

### ❌ Anti-Pattern 3: Mixing Data & Behavior
```cpp
// Nope:
struct ECUInfo {
    void SetModel(const std::string& m) { model = m; }  // Mutation
};

// Yes:
struct ECUInfo {
    std::string model;  // Value type, immutable
};
```

### ❌ Anti-Pattern 4: No Dependency Injection
```cpp
// Nope:
class ConnectionModal {
    std::unique_ptr<SystemPortEnumerator> port_enum_;  // Hard-coded!
};

// Yes:
class ConnectionModal {
    std::unique_ptr<IPortEnumerator> port_enum_;  // Interface!
    explicit ConnectionModal(std::unique_ptr<IPortEnumerator> pe)
        : port_enum_(std::move(pe)) {}
};
```

---

## ✅ Final Checklist (PR Merge)

- [ ] Código segue NASA P10
- [ ] Código segue C++20/23 Guidelines
- [ ] Compila sem warnings (`-Wall -Wextra -Werror`)
- [ ] clang-tidy clean (`cppcoreguidelines-*`)
- [ ] Testes passam (GoogleTest)
- [ ] Code review aprovado
- [ ] Integrado em main.cpp
- [ ] Documentação atualizada (GUIA_IMPLEMENTACAO_CONNECTION.md)

**Status:** 🟡 READY FOR IMPLEMENTATION

---

**Documento para revisores de código. Use como guia ao revisar PRs.**
