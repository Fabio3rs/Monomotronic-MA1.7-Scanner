# 🎯 RECOMENDAÇÕES DE AJUSTES: UI/UX Scanner

## 📸 Análise das Screenshots

**Status Observado:**
- ECU: DISCONNECTED (modo simulação)
- Tela DASH: Múltiplos widgets com dados
- Tela DTC: 3 DTCs ativos/stored
- Tela LIVE: Sensores com trend + alerta crítico

---

## 🔴 CRÍTICO: Ajustes Urgentes

### **1. Alerta Crítico Inadequado**

**Problema:**
```
Status: "⚠️ ALERTA: Sensores críticos fora da faixa!"
Localização: Bottom bar (full width)
Severidade visual: MUITO alta (fundo vermelho)
```

**Impacto:**
- Usuário vê vermelho em modo SIMULAÇÃO
- Falso positivo prejudica confiança
- Em produção, pode causar alarme desnecessário

**Recomendação:**
```cpp
// scanner_glfw/screens/LiveScreen.cpp

// ANTES: Sempre mostra alerta mesmo em simulação
if (HasCriticalSensors()) {
    RenderCriticalAlert();  // ❌ Vermelho assustador
}

// DEPOIS: Diferencia simulação vs. produção
if (HasCriticalSensors() && !IsSimulationMode()) {
    RenderCriticalAlert();  // ✅ Apenas em ECU real
}

// Ou: Usar cor diferente em simulação
if (HasCriticalSensors()) {
    ImVec4 alert_color = IsSimulationMode()
        ? ImVec4(1.0f, 0.8f, 0.2f, 0.8f)  // Amarelo (simulação)
        : ImVec4(1.0f, 0.2f, 0.2f, 0.8f); // Vermelho (real)
    RenderAlert(alert_color);
}
```

**Prioridade:** 🔴 P0 (hoje)

---

### **2. Status ECU Confuso**

**Problema:**
```
Header mostra: "ECU: DISCONNECTED ? MODO SIMULAÇÃO"
Duas informações conflitantes:
├─ DISCONNECTED (vermelho)
└─ MODO SIMULAÇÃO (laranja)
```

**UX Issue:**
- Usuário não sabe se é erro ou intencional
- "?" sugere incerteza do sistema
- Cores vermelha + laranja = caos visual

**Recomendação:**

```cpp
// scanner_glfw/ui/TopBar.cpp

void RenderECUStatus() {
    const char *status_text;
    ImVec4 status_color;

    if (ecu_backend_.IsConnected()) {
        status_text = "✓ CONECTADO";
        status_color = ImVec4(0.2f, 1.0f, 0.3f, 1.0f);  // Verde
    } else if (IsSimulationMode()) {
        status_text = "🔬 SIMULAÇÃO";
        status_color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f);  // Azul
    } else {
        status_text = "✗ DESCONECTADO";
        status_color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);  // Vermelho
    }

    ImGui::TextColored(status_color, "ECU: %s", status_text);
}
```

**Prioridade:** 🔴 P0 (hoje)

---

### **3. Indicador DTC Não Corresponde**

**Problema:**
```
Screenshot 2 (DTC screen):
├─ Header: "3 Active DTCs"
├─ Tabela mostra: P0171, P0301, P0420
└─ Badge BottomNav: Vermelho com "3"

Mas também tem "Stored":
├─ P0562 (System Voltage Low)
└─ P0420 (Catalyst Efficiency)
```

**Questão:**
- Active = apenas P0171, P0301, P0420? (3)
- Ou Active + Stored combinado?

**Recomendação:**

```cpp
// scanner_glfw/screens/DTCScreen.cpp

void RenderDTCTable() {
    // SEÇÃO 1: Active DTCs
    ImGui::BeginTable("ActiveDTCs", 3);
    for (const auto &dtc : active_dtcs_) {
        // Render com fundo vermelho
    }
    ImGui::EndTable();

    // SEÇÃO 2: Stored DTCs (separado)
    ImGui::BeginTable("StoredDTCs", 3);
    for (const auto &dtc : stored_dtcs_) {
        // Render com fundo amarelo
    }
    ImGui::EndTable();
}

// BottomNav: mostrar APENAS active
badge_count = active_dtcs_.size();  // ✅ Limpo
```

**Prioridade:** 🟠 P1 (próxima sprint)

---

## 🟠 IMPORTANTE: Melhorias de UX

### **4. LIVE Screen: Trend Não Sincronizado**

**Problema:**
```
Screenshot 3 (LIVE screen):
├─ TPS Trilha 1: 6.5 % (trend →)
├─ RPM Motor: 850 rpm (trend →)
├─ Temp. Ar: 19.6 °C (trend ↓ parcial)
└─ Integrador Lambda: -9.8 % (trend ↓)

Observação: Trend gráfico vs. valor não batem
```

**Issue:**
- Gráfico mostra "estável/subindo"
- Mas sensor está em -9.8% (negativo)
- Contraintuition: cor verde + valor negativo

**Recomendação:**

```cpp
// scanner_glfw/screens/LiveScreen.cpp

struct SensorDisplay {
    std::string name;
    float value;
    std::string unit;
    std::vector<float> trend_history;  // Últimos N samples

    // Método para calcular cor baseada em trend
    ImVec4 GetTrendColor() const {
        if (trend_history.size() < 2) return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

        float delta = trend_history.back() - trend_history.front();

        if (abs(delta) < 0.1f) {
            return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Cinza (estável)
        } else if (delta > 0) {
            return ImVec4(0.2f, 1.0f, 0.3f, 1.0f);  // Verde (aumentando)
        } else {
            return ImVec4(1.0f, 0.7f, 0.2f, 1.0f);  // Amarelo (diminuindo)
        }
    }
};

void RenderSensorRow(const SensorDisplay &sensor) {
    // Valor + cor de trend
    ImGui::TextColored(sensor.GetTrendColor(), "%.1f %s",
                      sensor.value, sensor.unit.c_str());

    // Gráfico de trend (pequeno sparkline)
    ImGui::SameLine();
    RenderSparkline(sensor.trend_history);
}
```

**Prioridade:** 🟠 P1 (próxima sprint)

---

### **5. Botões de Ação Descobertos**

**Problema:**
```
Top bar mostra muitos botões:
├─ Read (amarelo)
├─ Clear (azul)
├─ Edit
├─ Reset to
├─ Rec (branco)
├─ Paus (azul)
└─ Snap (bolts)

Usuário fica confuso: qual usar?
```

**Recomendação:**

```cpp
// scanner_glfw/screens/LiveScreen.cpp

void RenderTopControls() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Ações Principais:");

    // Seção 1: Coleta
    if (ImGui::Button("● START LEITURA", ImVec2(150, 40))) {
        backend_.StartPolling();
    }
    ImGui::SameLine();
    if (ImGui::Button("⏸ PAUSAR", ImVec2(150, 40))) {
        backend_.Pause();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Ferramentas:");

    // Seção 2: Ferramentas
    if (ImGui::Button("📸 SNAPSHOT", ImVec2(100, 30))) {
        TakeSnapshot();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔧 EDITAR", ImVec2(100, 30))) {
        OpenEditDialog();
    }
}
```

**Resultado:**
- Botões agrupados por função
- Labels mais claros
- Ícones com significado visual

**Prioridade:** 🟠 P1

---

## 🟡 DESEJÁVEL: Polish

### **6. Indicador de Simulação Mais Óbvio**

**Problema:**
```
Modo simulação é apenas 2 palavras no header
Usuário pode esquecer que está em simulação
```

**Recomendação:**

```cpp
void TopBar::Render() {
    bool is_sim = IsSimulationMode();

    if (is_sim) {
        // Fundo de aviso (padrão diagonal)
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImVec2(ImGui::GetIO().DisplaySize.x, 100),
            ImGui::GetColorU32(ImVec4(0.2f, 0.15f, 0.0f, 0.2f))
        );

        // Texto piscante
        float blink = sinf(ImGui::GetTime() * 2.0f) * 0.5f + 0.5f;
        ImGui::TextColored(
            ImVec4(1.0f, 0.8f, 0.0f, blink),
            "🔬 MODO SIMULAÇÃO - Dados não são reais"
        );
    }
}
```

**Prioridade:** 🟡 P2

---

### **7. Histórico de Snapshots**

**Problema:**
```
Botão "Snap" existe mas:
├─ Não há lista de snapshots
├─ Não pode comparar
├─ Snapshot não é gravado
```

**Recomendação:**

Implementar após DataLogging (uma extensão natural):
```cpp
// scanner_glfw/screens/SnapshotsScreen.h (nova tela)

class SnapshotsScreen : public BaseScreen {
public:
    void AddSnapshot(const SessionSnapshot &snap);
    void RenderComparison(const SessionSnapshot &snap1,
                         const SessionSnapshot &snap2);
};
```

**Prioridade:** 🟡 P3 (após DataLogging)

---

## 📋 Sumário de Recomendações

| # | Ajuste | Tipo | Prioridade | Esforço | Impacto |
|---|--------|------|-----------|---------|---------|
| 1 | Alerta crítico em simulação | Fix | 🔴 P0 | 30 min | Alto |
| 2 | Status ECU ambíguo | Fix | 🔴 P0 | 30 min | Alto |
| 3 | Indicador DTC confuso | Improvement | 🟠 P1 | 2h | Médio |
| 4 | Trend color mismatch | Bug | 🟠 P1 | 1h | Médio |
| 5 | Botões desorganizados | UX | 🟠 P1 | 2h | Médio |
| 6 | Simulação óbvia | Polish | 🟡 P2 | 1h | Baixo |
| 7 | Histórico snapshots | Feature | 🟡 P3 | 3 dias | Médio |

---

## 🚀 Plano de Ação Recomendado

### **Hoje (1h)**
- [ ] Fix: Alerta crítico em simulação
- [ ] Fix: Status ECU ambíguo
- [ ] Test: Ambos os fixes

### **Esta Semana (5h)**
- [ ] Improvement: DTC active/stored separado
- [ ] Bug fix: Trend color
- [ ] UX: Organizar botões

### **Próxima Sprint (3 dias)**
- [ ] Polish: Simulação piscante
- [ ] Feature: DataLogging (já documentado)
- [ ] Feature: Snapshots (extensão de DataLogging)

---

## 💡 Quick Wins (fáceis de fazer)

### **Quick Win #1: Alerta em Simulação** (15 min)

```cpp
// scanner_glfw/screens/LiveScreen.cpp linha ~150

// ANTES:
if (HasCriticalSensors()) {
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "⚠️ ALERTA: Sensores críticos fora da faixa!");
}

// DEPOIS:
if (HasCriticalSensors()) {
    if (!IsSimulationMode()) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                          "⚠️ ALERTA: Sensores críticos fora da faixa!");
    }
}
```

### **Quick Win #2: Status Claro** (15 min)

```cpp
// scanner_glfw/ui/TopBar.cpp linha ~50

// ANTES:
ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                  "ECU: DISCONNECTED ? MODO SIMULAÇÃO");

// DEPOIS:
ImVec4 color;
const char *text;

if (ecu_.IsConnected()) {
    text = "ECU: ✓ CONECTADO";
    color = ImVec4(0.2f, 1.0f, 0.3f, 1.0f);
} else if (IsSimulationMode()) {
    text = "ECU: 🔬 SIMULAÇÃO";
    color = ImVec4(0.2f, 0.7f, 1.0f, 1.0f);
} else {
    text = "ECU: ✗ DESCONECTADO";
    color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
}

ImGui::TextColored(color, "%s", text);
```

---

## 🔧 Próximas Etapas

1. **Escolha prioridade:** P0 fixes hoje ou aguarda sprint?
2. **Code review:** Quer que eu crie PR com esses fixes?
3. **DataLogging:** Depois de ajustes, começa implementação?

Recomendação: **Faça os 2 P0s hoje (30 min), depois DataLogging (1 semana).**

