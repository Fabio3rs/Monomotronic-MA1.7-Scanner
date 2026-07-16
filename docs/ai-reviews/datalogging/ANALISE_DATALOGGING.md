# Análise: Sistema de DataLogging Implementado

## 📊 Status Atual

**Implementação:** ✅ **PARCIALMENTE IMPLEMENTADA**

**Atualização 2026-02-22:** RecordingManager já grava CSV via botão Start/Stop na LiveScreen, com headers, timestamps e delta_ms. Pendências principais: JSON/compressão, replay/análise histórica (gráficos, stats), filtros de sensores e comparação de sessões.

O projeto possui:
- ✅ Tela de Logs (`LogsScreen`)
- ✅ UI para visualização
- ✅ Controles básicos (pause, clear, export)
- ⚠️ Armazenamento limitado (buffer circular da ECU)
- ❌ Datalogging em tempo real durante medições
- ❌ Armazenamento persistente de dados de sensores
- ❌ Análise pós-sessão dos dados

---

## 🏗️ Arquitetura Atual

### **LogsScreen (Tela de Logs)**

```
scanner_glfw/screens/LogsScreen.h/cpp

✅ Implementado:
├─ Tela dedicada para visualização de logs
├─ Auto-refresh a cada 500ms
├─ Pause/Resume
├─ Clear logs (buffer local)
├─ Export para arquivo .txt
├─ Auto-scroll
└─ Fade transitions

⚠️ Limitações:
├─ Mostra apenas logs da ECU (mensagens de comunicação)
├─ Não registra dados de sensores/medições
├─ Buffer é circular (sobrescreve dados antigos)
└─ Sem timeline ou timestamps
```

### **Estrutura da Tela**

```cpp
class LogsScreen : public BaseScreen {
    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

private:
    void RenderTopControls();      // Buttons: Pause/Resume, Clear, Export
    void RenderLogContent();        // Text area com logs
    void RefreshLogs();             // Pull logs da ECU
    void ClearLogs();               // Limpar buffer local
    void ExportLogs();              // Salvar em arquivo

    std::string log_buffer_;        // Buffer para armazenar logs
    bool paused_ = false;           // Pausar refresh?
    bool auto_scroll_ = true;       // Auto-scroll para bottom?
    bool show_export_success_;      // Mostrar confirmação?
    std::chrono::milliseconds REFRESH_INTERVAL = 500ms;
};
```

---

## 🔌 Integração com ECUBackend

### **Método: GetLogs()**

```cpp
// ECUBackend.cpp:268
void ECUBackend::GetLogs(std::string &buffer) {
    if (ecu_ && ecu_->isInitialized()) {
        ecu_->strprintlogging(buffer);  // Pull logs da ECU
    } else {
        buffer = "No logs available (ECU logging buffer is empty).";
    }
}
```

**Fluxo:**
1. `LogsScreen::RefreshLogs()` chama
2. `ECUBackend::GetLogs()`
3. `ECUMonomotronic::strprintlogging()` (backend nativo)
4. Buffer retorna como `std::string`
5. Renderizado em ImGui::TextUnformatted()

---

## 📋 Controles da UI

### **Top Controls (RenderTopControls)**

```
┌─────────────────────────────────────────────────────┐
│  ▶ Resume    │ 🗑 Clear    │ 💾 Export   Auto-scroll ✓ │
│   (ou ⏸ Pause)                          ✓ Exported!   │
└─────────────────────────────────────────────────────┘
```

**Botões:**

| Botão | Ícone | Ação | Implementado |
|-------|-------|------|--------------|
| **Pause/Resume** | ⏸/▶ | Pausar auto-refresh de logs | ✅ |
| **Clear** | 🗑 | Limpar buffer local | ✅ |
| **Export** | 💾 | Salvar em arquivo .txt | ✅ |
| **Auto-scroll** | ✓ | Scroll automático para bottom | ✅ |

### **Log Content Area**

```
┌─────────────────────────────────────────────────────┐
│ [ScrollBar]                                         │
│                                                     │
│ 12:34:56 ECU Init started...                       │
│ 12:34:57 F4 Init packet received                   │
│ 12:34:58 Sensor table 1 loaded                     │
│ 12:34:59 Polling active                            │
│ 12:35:00 Sample 1: RPM=1200, MAF=3.5               │
│                                                     │
│ [Auto-scroll] [Scrollbar at bottom]                │
└─────────────────────────────────────────────────────┘
```

---

## 📤 Exportação de Logs

### **Implementação (ExportLogs)**

```cpp
void LogsScreen::ExportLogs() {
    if (log_buffer_.empty()) {
        fprintf(stderr, "WARNING: No logs to export\n");
        return;
    }

    // Gerar nome com timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream filename;
    filename << "ecu_logs_"
             << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
             << ".txt";

    // Salvar no HOME ou /tmp
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";
    std::string filepath = std::string(home) + "/" + filename.str();

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << log_buffer_;
        file.close();

        show_export_success_ = true;
        export_message_timer_ = 3.0f;  // 3 segundos
    }
}
```

**Resultado:**
```
~/ecu_logs_20260114_143052.txt

Conteúdo:
12:34:56 ECU Init started...
12:34:57 F4 Init packet received
12:34:58 Sensor table 1 loaded
...
```

---

## 🔄 Fluxo de Dados (Visualização)

```
┌─────────────────────────────────────────────────────┐
│                   main.cpp                          │
│              (Render loop)                          │
└────────────────┬────────────────────────────────────┘
                 │
                 ├─► LogsScreen::Render()
                 │
                 └─► LogsScreen::Update(delta_time)
                     │
                     ├─ Checkar REFRESH_INTERVAL (500ms)
                     │
                     └─► RefreshLogs()
                         │
                         └─► ECUBackend::GetLogs()
                             │
                             └─► ECUMonomotronic::strprintlogging()
                                 │
                                 └─ Buffer preenchido
                                    (dados em std::string)
                                    │
                                    └─► RenderLogContent()
                                        │
                                        └─ ImGui::TextUnformatted()
                                           exibe logs
```

---

## 🎯 O Que Está Implementado

### ✅ Feito

1. **Tela Dedicada (LogsScreen)**
   - Renderização em ImGui
   - Fade transitions suave
   - Gestures (swipe left para DTC)

2. **Controles Básicos**
   - Pause/Resume auto-refresh
   - Clear logs (buffer local)
   - Export para arquivo
   - Auto-scroll
   - Checkbox para controlar auto-scroll

3. **Exportação**
   - Filename com timestamp
   - Salva em HOME directory
   - Confirmação visual (3 segundos)
   - Fallback para /tmp

4. **Integração com ECU**
   - Pull logs via `ECUBackend::GetLogs()`
   - Auto-refresh a cada 500ms
   - Status "ECU not connected" se offline

---

## ⚠️ O Que **NÃO** Está Implementado

### ❌ Datalogging de Sensores

**Problema Crítico:** Não há gravação de dados de **sensores/medições** durante operação

```
O que temos:
├─ Logs de comunicação ECU (init packets, ACKs)
└─ Mensagens de debug/status

O que FALTA:
├─ Histórico de valores de sensores (RPM, MAF, etc)
├─ Timestamps para cada amostra
├─ Arquivo de dados para análise pós-sessão
└─ Visualização de gráficos históricos
```

### ❌ Armazenamento Persistente

```
Atual:
├─ Buffer circular (sobrescreve)
├─ Apenas durante sessão (RAM)
└─ Se ECU desconectar = dados perdidos

Necessário:
├─ Arquivo CSV/JSON durante gravação
├─ Timestamps precisos
├─ Análise de tendências
└─ Gráficos pós-sessão
```

### ❌ Controle de Gravação

```
Faltam:
├─ Botão "Start Recording" / "Stop Recording"
├─ Status visual (gravando / parado)
├─ Tamanho do arquivo em tempo real
├─ Filtro de sensores (gravar quais?)
└─ Taxa de amostragem configurável
```

### ❌ Análise de Dados

```
Não há:
├─ Gráficos de histórico
├─ Exportar em formatos (CSV, JSON)
├─ Comparação de sessões
├─ Estatísticas (min/max/avg)
└─ Marcadores de eventos
```

---

## 📊 Tabela Comparativa

| Feature | Implementado | Tipo | Status |
|---------|--------------|------|--------|
| **Tela Logs** | Sim | UI | ✅ |
| **Visualizar logs ECU** | Sim | Dados | ✅ |
| **Export .txt** | Sim | File I/O | ✅ |
| **Auto-refresh 500ms** | Sim | Timing | ✅ |
| **Pause/Resume** | Sim | Control | ✅ |
| **Clear** | Sim | Control | ✅ |
| **Gravar sensores** | **NÃO** | ❌ | ❌ |
| **Arquivo CSV/JSON** | **NÃO** | ❌ | ❌ |
| **Start/Stop gravação** | **NÃO** | ❌ | ❌ |
| **Gráficos histórico** | **NÃO** | ❌ | ❌ |
| **Análise pós-sessão** | **NÃO** | ❌ | ❌ |
| **Timestamps** | **NÃO** | ❌ | ❌ |
| **Filtro sensores** | **NÃO** | ❌ | ❌ |

---

## 🚀 Proposta: Sistema de DataLogging Completo

### **O Que Falta**

1. **Recording Manager** (controlar gravação)
2. **DataLog File** (armazenar samples)
3. **Logger Thread** (background writing)
4. **Export Formats** (CSV, JSON)
5. **Analysis Tools** (gráficos, stats)

### **Timeline de Implementação**

```
Fase 1 (2 dias): Recording + CSV Export
  ├─ RecordingManager (start/stop)
  ├─ DataLogFile (CSV writer)
  └─ UI controls na LiveScreen

Fase 2 (1 dia): Análise Básica
  ├─ GraphScreen upgrade (histórico)
  ├─ Min/Max/Avg display
  └─ Export formats (JSON, etc)

Fase 3 (1 dia): Polish
  ├─ Testes
  ├─ NASA P10 compliance
  └─ Code review

Total: 4 dias
```

---

## 💾 Código Exemplo: Onde Adicionar

### **app_data.h**

```cpp
// Adicionar struct para datalogging
struct RecordingSession {
    bool recording = false;
    std::string filename;
    std::chrono::system_clock::time_point start_time;
    uint64_t sample_count = 0;

    // Configuração
    bool record_sensors = true;
    std::set<int> sensor_filter;  // Se vazio = todos
};

extern RecordingSession g_recording;
```

### **LiveScreen.cpp (onde adicionar gravação)**

```cpp
// Atual (sem gravação):
void LiveScreen::Update(float delta_time) {
    auto &backend = ECUBackend::Instance();
    std::vector<SensorState> samples;

    if (backend.DrainSamples(samples)) {
        // Atualizar UI apenas
        UpdateSensorUI(samples);
    }
}

// Proposto (com gravação):
void LiveScreen::Update(float delta_time) {
    auto &backend = ECUBackend::Instance();
    std::vector<SensorState> samples;

    if (backend.DrainSamples(samples)) {
        UpdateSensorUI(samples);

        // NOVO: Gravar se ativo
        if (g_recording.recording) {
            logger_->LogSamples(samples,
                               std::chrono::system_clock::now());
        }
    }
}
```

---

## 🎓 Conclusão

**Status Atual:**
- ✅ Logging de ECU (comunicação) - **IMPLEMENTADO**
- ❌ DataLogging de sensores - **NÃO IMPLEMENTADO**

**Impacto para o Usuário:**
- Pode ver logs de comunicação ECU ✅
- **Não pode gravar dados de medições** ❌
- **Não pode analisar histórico de sensores** ❌

**Recomendação:**
Implementar sistema de datalogging completo (Fase 1: 2 dias)

Isso permitiria:
1. Gravar sessões de diagnóstico
2. Análise pós-sessão
3. Comparação de múltiplas sessões
4. Gráficos de tendências
5. Detecção de anomalias
