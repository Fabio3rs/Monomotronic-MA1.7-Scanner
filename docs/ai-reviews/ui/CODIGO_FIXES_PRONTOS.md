# 💻 CÓDIGO PRONTO: Fixes Quick-Win

## 🚀 Copie e Cole para Implementar

---

## Fix #1: Alerta Falso em Simulação (15 min)

### **Arquivo: `scanner_glfw/screens/LiveScreen.cpp`**

**Localize:**
```cpp
if (HasCriticalSensors()) {
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "⚠️ ALERTA: Sensores críticos fora da faixa!");
}
```

**Substitua por:**
```cpp
// Não mostrar alerta em modo simulação
if (HasCriticalSensors() && ecu_.IsConnected()) {
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "⚠️ ALERTA: Sensores críticos fora da faixa!");
}
```

**Ou, se quiser diferenciar simulação vs. produção:**
```cpp
if (HasCriticalSensors()) {
    if (ecu_.IsConnected()) {
        // ECU real: alerta vermelho
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                          "⚠️ ALERTA: Sensores críticos fora da faixa!");
    } else if (IsSimulationMode()) {
        // Simulação: aviso em amarelo
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                          "ℹ️ INFO: Sensores em simulação estão fora de range");
    }
}
```

---

## Fix #2: Status ECU Claro (20 min)

### **Arquivo: `scanner_glfw/ui/TopBar.cpp` ou onde status é renderizado**

**Localize:**
```cpp
ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                  "ECU: DISCONNECTED ? MODO SIMULAÇÃO");
```

**Substitua por:**
```cpp
// Função auxiliar (pode ir em ECUBackend.h ou TopBar.h)
struct ECUStatus {
    const char *text;
    ImVec4 color;
};

ECUStatus GetECUStatus(ECUBackend &ecu) {
    if (ecu.IsConnected()) {
        return {"✓ CONECTADO", ImVec4(0.2f, 1.0f, 0.3f, 1.0f)};  // Verde
    } else if (ecu.IsSimulationMode()) {
        return {"🔬 SIMULAÇÃO", ImVec4(0.2f, 0.7f, 1.0f, 1.0f)};  // Azul
    } else {
        return {"✗ DESCONECTADO", ImVec4(1.0f, 0.2f, 0.2f, 1.0f)};  // Vermelho
    }
}

// No Render():
auto status = GetECUStatus(ecu_);
ImGui::TextColored(status.color, "ECU: %s", status.text);
```

---

## Fix #3: Separar DTCs Active/Stored (1 hora)

### **Arquivo: `scanner_glfw/screens/DTCScreen.cpp`**

**Localize:**
```cpp
void DTCScreen::Render() {
    ImGui::Text("3 Active DTCs");

    ImGui::BeginTable("DTCs", 3);
    // ... todas as DTCs juntas (PROBLEMA)
    ImGui::EndTable();
}
```

**Substitua por:**
```cpp
void DTCScreen::Render() {
    // Separar em 2 grupos
    std::vector<DTCInfo> active_dtcs;
    std::vector<DTCInfo> stored_dtcs;

    for (const auto &dtc : all_dtcs_) {
        if (dtc.status == DTCStatus::ACTIVE) {
            active_dtcs.push_back(dtc);
        } else if (dtc.status == DTCStatus::STORED) {
            stored_dtcs.push_back(dtc);
        }
    }

    // Seção 1: Active DTCs
    ImGui::Text("Active DTCs (%zu)", active_dtcs.size());
    ImGui::Separator();

    if (ImGui::BeginTable("ActiveDTCs", 3)) {
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Description");
        ImGui::TableHeadersRow();

        for (const auto &dtc : active_dtcs) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ACTIVE");

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", dtc.code.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", dtc.description.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // Seção 2: Stored DTCs
    ImGui::Text("Stored DTCs (%zu)", stored_dtcs.size());
    ImGui::Separator();

    if (ImGui::BeginTable("StoredDTCs", 3)) {
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Code");
        ImGui::TableSetupColumn("Description");
        ImGui::TableHeadersRow();

        for (const auto &dtc : stored_dtcs) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "STORED");

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", dtc.code.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", dtc.description.c_str());
        }
        ImGui::EndTable();
    }

    // Badge no BottomNav: APENAS active count
    badge_count_ = active_dtcs.size();  // ✅ Correto
}
```

**Atualizar BottomNav:**
```cpp
// scanner_glfw/ui/BottomNav.cpp

// ANTES:
int badge_count = all_dtcs_.size();  // ❌ Active + Stored

// DEPOIS:
int badge_count = active_dtcs_.size();  // ✅ Apenas Active
```

---

## Fix #4: Trend Color Correto (1 hora)

### **Arquivo: `scanner_glfw/screens/LiveScreen.cpp` ou `scanner_glfw/core/SensorState.h`**

**Adicione classe/struct para tracking de trend:**

```cpp
struct SensorTrendAnalyzer {
    std::vector<float> history;
    static constexpr size_t MAX_HISTORY = 30;  // Últimos 30 samples

    void AddSample(float value) {
        history.push_back(value);
        if (history.size() > MAX_HISTORY) {
            history.erase(history.begin());
        }
    }

    ImVec4 GetTrendColor(float critical_low = -10000.0f,
                        float critical_high = 10000.0f) const {
        if (history.size() < 2) {
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Cinza (sem dados)
        }

        float last_value = history.back();
        float first_value = history.front();
        float trend = last_value - first_value;
        float abs_trend = abs(trend);

        // Check crítico primeiro
        if (last_value < critical_low || last_value > critical_high) {
            return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Vermelho (crítico)
        }

        // Depois considerar trend
        if (abs_trend < 0.1f) {
            return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Cinza (estável)
        } else if (trend > 0) {
            return ImVec4(0.2f, 1.0f, 0.3f, 1.0f);  // Verde (aumentando)
        } else {
            return ImVec4(1.0f, 0.7f, 0.2f, 1.0f);  // Laranja (diminuindo)
        }
    }

    const char *GetTrendArrow() const {
        if (history.size() < 2) return "→";

        float trend = history.back() - history.front();
        if (abs(trend) < 0.1f) return "→";
        if (trend > 0) return "↗";
        return "↘";
    }
};
```

**Usar no Render:**
```cpp
void LiveScreen::RenderSensorRow(const std::string &sensor_name,
                                 float value,
                                 const std::string &unit,
                                 const SensorTrendAnalyzer &trend) {
    ImVec4 trend_color = trend.GetTrendColor();

    ImGui::TableNextRow();

    // Coluna 1: Status color (baseado em valor crítico)
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("●");
    ImGui::SameLine();
    ImGui::TextColored(trend_color, " ");  // Apenas a cor visual

    // Coluna 2: Nome do sensor
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%s", sensor_name.c_str());

    // Coluna 3: Valor com cor de trend
    ImGui::TableSetColumnIndex(2);
    ImGui::TextColored(trend_color, "%.1f %s", value, unit.c_str());

    // Coluna 4: Trend visual
    ImGui::TableSetColumnIndex(3);
    ImGui::TextColored(trend_color, "%s", trend.GetTrendArrow());
}
```

---

## Fix #5: Organizar Botões (2 horas)

### **Arquivo: `scanner_glfw/screens/LiveScreen.cpp`**

**Localize:**
```cpp
void LiveScreen::RenderTopControls() {
    ImGui::Button("Custom");
    ImGui::Button("Default");
    ImGui::Button("Edit");
    ImGui::Button("Reset");
    ImGui::Button("Rec");
    ImGui::Button("Pause");
    ImGui::Button("Snap");
    // ... todos misturados
}
```

**Substitua por:**
```cpp
void LiveScreen::RenderTopControls() {
    // ─── Seção 1: Presets ───
    ImGui::Text("Presets:");
    if (ImGui::Button("●  Custom", ImVec2(120, 35))) {
        LoadPreset("custom");
    }
    ImGui::SameLine();
    if (ImGui::Button("○ Default", ImVec2(120, 35))) {
        LoadPreset("default");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ─── Seção 2: Coleta de Dados ───
    ImGui::Text("Coleta:");
    bool recording = IsRecording();

    if (!recording) {
        if (ImGui::Button("● START LEITURA", ImVec2(180, 40))) {
            StartRecording();
        }
    } else {
        if (ImGui::Button("⏸ PAUSAR LEITURA", ImVec2(180, 40))) {
            StopRecording();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ─── Seção 3: Ferramentas ───
    ImGui::Text("Ferramentas:");

    if (ImGui::Button("✎ EDITAR", ImVec2(110, 35))) {
        OpenEditDialog();
    }
    ImGui::SameLine();
    if (ImGui::Button("📸 SNAPSHOT", ImVec2(110, 35))) {
        TakeSnapshot();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 RESET", ImVec2(110, 35))) {
        ResetToDefaults();
    }
}
```

---

## ✅ Checklist de Implementação

### **Prioridade 1 (Hoje - 35 min)**
- [ ] Fix #1: Alerta em simulação (15 min)
- [ ] Fix #2: Status ECU (20 min)
- [ ] Test ambos

### **Prioridade 2 (Esta semana - 3 horas)**
- [ ] Fix #3: DTC separado (1h)
- [ ] Fix #4: Trend color (1h)
- [ ] Fix #5: Botões agrupados (1h)
- [ ] Test & build

### **Verificação após implementar:**

```bash
# 1. Build limpo?
cmake --build build/ -- -j4

# 2. Sem warnings?
clang-tidy src/screens/LiveScreen.cpp -checks="cppcoreguidelines-*"

# 3. Roda sem crashes?
./build/scanner_glfw/example_glfw_vulkan

# 4. Visual OK?
# Verificar screenshots lado a lado
```

---

## 🎬 Antes e Depois

### **ANTES (com problemas):**
```
⚠️ ALERTA: Sensores críticos fora...  (mesmo em simulação)
ECU: DISCONNECTED ? MODO SIMULAÇÃO     (confuso)
Active DTCs: 3 (mas mostra 5 na tabela)
Integrador Lambda: -9.8 % (verde, mas negativo!)
[Custom] [Default] [Edit] [Reset] [Rec] [Pause] [Snap]
```

### **DEPOIS (fixes aplicados):**
```
(sem alerta em simulação - correto!)
ECU: 🔬 SIMULAÇÃO              (claro e azul)
Active DTCs: 3
├─ P0171, P0301, C0040
Stored DTCs: 2
├─ P0420, P0562

Integrador Lambda: -9.8 % (laranja ↘, correto!)

Presets:
  [● Custom] [○ Default]
Coleta:
  [● START LEITURA] [⏸ PAUSAR]
Ferramentas:
  [✎ EDITAR] [📸 SNAPSHOT] [🔄 RESET]
```

---

## 💡 Pro Tips

1. **Test incrementalmente:** Fix um de cada vez
2. **Mantenha backup:** `git commit` antes de cada fix
3. **Use ImGui::Spacing():** Para separar visualmente seções
4. **Cores consistentes:** Defina em constantes globais
5. **Tooltips:** Adicione ao hover para botões confusos

---

**Pronto para copiar e colar! Boa sorte com os fixes!** 🚀

