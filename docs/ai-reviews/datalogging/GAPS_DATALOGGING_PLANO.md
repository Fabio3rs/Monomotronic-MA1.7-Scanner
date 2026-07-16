# Gaps e Melhorias: Sistema de DataLogging

## 🔍 Gaps Identificados vs. Implementado

### **Matriz de Funcionalidades**

```
┌────────────────────────────────────────────────────────────────┐
│                      FUNCIONALIDADE                            │
├─────────────────────────────┬──────────────────────────────────┤
│ CATEGORIA                   │ STATUS                           │
├─────────────────────────────┼──────────────────────────────────┤
│                    DISPLAY & VISUALIZATION                     │
├─────────────────────────────┼──────────────────────────────────┤
│ Real-time log viewer        │ ✅ Implementado                  │
│ Pausar/Resumir              │ ✅ Implementado                  │
│ Auto-scroll                 │ ✅ Implementado                  │
│ Gráficos em tempo real      │ ✅ Implementado                  │
│ Gráficos históricos         │ ❌ NÃO implementado              │
│ Timeline interativa         │ ❌ NÃO implementado              │
│ Zoom/Pan                    │ ✅ Parcialmente                  │
├─────────────────────────────┼──────────────────────────────────┤
│                      CONTROL & RECORDING                       │
├─────────────────────────────┼──────────────────────────────────┤
│ Botão Start/Stop            │ ✅ Implementado (LiveScreen)     │
│ Indicador gravando/parado   │ ✅ BottomNav + label na Live     │
│ Filtro sensores             │ ❌ NÃO implementado              │
│ Taxa amostragem config      │ ❌ NÃO implementado              │
│ Pausa temporária            │ ✅ Implementado (para logs)      │
├─────────────────────────────┼──────────────────────────────────┤
│                         FILE MANAGEMENT                        │
├─────────────────────────────┼──────────────────────────────────┤
│ Salvar arquivo              │ ✅ Implementado (CSV)            │
│ Timestamp automático        │ ✅ Implementado                  │
│ Múltiplos formatos (CSV)    │ ✅ Implementado (CSV)            │
│ Múltiplos formatos (JSON)   │ ❌ NÃO implementado              │
│ Compressão                  │ ❌ NÃO implementado              │
│ Diretório configurável      │ ✅ Parcialmente                  │
│ Nome arquivo customizável   │ ⚠️ Apenas timestamp              │
│ Metadata (duração, etc)     │ ❌ NÃO implementado              │
├─────────────────────────────┼──────────────────────────────────┤
│                         DATA ANALYSIS                          │
├─────────────────────────────┼──────────────────────────────────┤
│ Min/Max/Avg                 │ ❌ NÃO implementado              │
│ Std Deviation               │ ❌ NÃO implementado              │
│ Tendências                  │ ❌ NÃO implementado              │
│ Comparação sessões          │ ❌ NÃO implementado              │
│ Detecção anomalias          │ ❌ NÃO implementado              │
│ Eventos marcados            │ ❌ NÃO implementado              │
├─────────────────────────────┼──────────────────────────────────┤
│                      DATABASE & SEARCH                         │
├─────────────────────────────┼──────────────────────────────────┤
│ Pesquisar logs              │ ❌ NÃO implementado              │
│ Filtrar por timestamp       │ ❌ NÃO implementado              │
│ Filtrar por sensor          │ ❌ NÃO implementado              │
│ Exportar subset             │ ❌ NÃO implementado              │
│ Histórico sessões           │ ❌ NÃO implementado              │
└─────────────────────────────┴──────────────────────────────────┘
```

---

## 📊 Priorização de Gaps

### **Nível 1: CRÍTICO (Bloqueia funcionalidade)** 🔴

| Gap | Impacto | Esforço | Prioridade |
|-----|---------|---------|-----------|
| **Botão Start/Stop Recording** | Sem este, não pode gravar | 1 dia | 🔴 P0 |
| **Armazenar sensores em arquivo** | Dados se perdem | 1 dia | 🔴 P0 |
| **Timestamp para amostras** | Impossível análise temporal | 0.5 dias | 🔴 P0 |
| **Indicador visual "gravando"** | Usuário não sabe status | 0.5 dias | 🔴 P0 |

**Estimativa Fase 1:** 3 dias

---

### **Nível 2: IMPORTANTE (Melhora UX)** 🟠

| Gap | Impacto | Esforço | Prioridade |
|-----|---------|---------|-----------|
| **Gráficos históricos** | Análise pós-sessão | 2 dias | 🟠 P1 |
| **CSV Export** | Análise em Excel | 1 dia | 🟠 P1 |
| **Min/Max/Avg stats** | Visão rápida de range | 1 dia | 🟠 P1 |
| **Comparação sessões** | Diagnóstico iterativo | 2 dias | 🟠 P1 |

**Estimativa Fase 2:** 6 dias

---

### **Nível 3: DESEJÁVEL (Polish)** 🟡

| Gap | Impacto | Esforço | Prioridade |
|-----|---------|---------|-----------|
| **JSON Export** | Integração com ferramentas | 1 dia | 🟡 P2 |
| **Filtro sensores** | Reduzir tamanho arquivo | 1 dia | 🟡 P2 |
| **Compressão** | Economia de espaço | 2 dias | 🟡 P2 |
| **Detecção anomalias** | Alertas inteligentes | 3 dias | 🟡 P2 |

**Estimativa Fase 3:** 7 dias

---

## 🎯 Plano de Ação (MVP - 3 dias)

### **Day 1: Core Recording**

#### **Tarefa 1.1: RecordingManager Base** (3h)

```cpp
// recorder_manager.h
class RecordingManager {
    bool StartRecording(const std::string &filename);
    void StopRecording();
    bool IsRecording() const;

    void LogSample(int sensor_id, float value,
                  const std::string &unit);
};
```

**Checklist:**
- [ ] Classe criada
- [ ] Métodos base implementados
- [ ] Arquivo aberto/fechado corretamente
- [ ] RAII pattern usado
- [ ] Teste de start/stop

#### **Tarefa 1.2: CSV Writer** (3h)

```cpp
// Gerar CSV com headers
timestamp_ms,sensor_id,sensor_name,value,unit
0,0,RPM,1200.0,rpm
10,0,RPM,1250.5,rpm
20,1,MAF,3.5,g/s
```

**Checklist:**
- [ ] Header escrito corretamente
- [ ] Linhas formatadas
- [ ] Flush a cada N amostras
- [ ] Arquivo validável em Excel

#### **Tarefa 1.3: LiveScreen Integration** (2h)

```cpp
// Adicionar botão Start/Stop na UI
void LiveScreen::RenderRecordingButton() {
    if (ImGui::Button("● START", ImVec2(100, 40))) {
        RecordingManager::Instance().StartRecording("session_1");
    }
}
```

**Checklist:**
- [ ] Botão renderiza
- [ ] Status visual muda (red = recording)
- [ ] Clique funciona
- [ ] Mensagem de erro se falha

#### **Tarefa 1.4: Data Capture** (2h)

```cpp
// No Update() da LiveScreen
if (RecordingManager::Instance().IsRecording()) {
    for (auto &sample : new_samples) {
        RecordingManager::Instance().LogSample(
            sample.sensor_id, sample.value, sample.unit
        );
    }
}
```

**Checklist:**
- [ ] Samples capturados
- [ ] Nenhuma amostra perdida
- [ ] Performance aceitável

#### **Tarefa 1.5: Tests** (2h)

```cpp
TEST_F(RecordingTest, WriteCSV) {
    manager->StartRecording("test.csv");
    manager->LogSample(0, 2500.0f, "rpm");
    manager->StopRecording();

    // Verificar arquivo
    std::ifstream file("test.csv");
    EXPECT_TRUE(file.good());
}
```

**Checklist:**
- [ ] Testes compilam
- [ ] Todos passam
- [ ] Coverage > 80%

---

### **Day 2: Analysis & Export**

#### **Tarefa 2.1: Stats Calculator** (2h)

```cpp
struct SessionStats {
    float min_value, max_value, avg_value;
    float std_deviation, median;
};

SessionStats CalculateStats(const std::vector<float> &values);
```

**Checklist:**
- [ ] Min/Max corretos
- [ ] Average correto
- [ ] Std dev correto
- [ ] Testes de edge cases

#### **Tarefa 2.2: GraphScreen Enhancement** (3h)

```cpp
// Carregar arquivo CSV e plotar
void GraphScreen::LoadSessionFile(const std::string &filename);
void GraphScreen::RenderHistoricalGraph();
```

**Checklist:**
- [ ] Arquivo carregado
- [ ] Dados parseados
- [ ] Gráfico plotado
- [ ] Zoom/Pan funciona

#### **Tarefa 2.3: Multi-format Export** (2h)

```cpp
// CSV está pronto
// Adicionar JSON export
void ExportJSON(const std::string &filename,
                const std::vector<SensorSnapshot> &data);
```

**Checklist:**
- [ ] JSON bem-formado
- [ ] Validável por json.org
- [ ] Contém metadata

#### **Tarefa 2.4: Session Management** (2h)

```cpp
// Listar sessões gravadas
std::vector<SessionInfo> ListSessions();
void DeleteSession(const std::string &filename);
void RenameSession(const std::string &old_name,
                  const std::string &new_name);
```

**Checklist:**
- [ ] Lista funciona
- [ ] Delete funciona
- [ ] Rename funciona

---

### **Day 3: Polish & Testing**

#### **Tarefa 3.1: NASA P10 Compliance** (2h)

**Checklist de Segurança:**
- [ ] Sem loops infinitos
- [ ] Buffer bounded (max size)
- [ ] Assertions em cada função
- [ ] Error handling completo
- [ ] No dynamic allocation in loops

**Validação:**
```bash
clang-tidy -checks="cppcoreguidelines-*" src/RecordingManager.cpp
```

#### **Tarefa 3.2: Performance Test** (1h)

```cpp
TEST_F(RecordingPerf, LogThroughput) {
    auto start = std::chrono::steady_clock::now();

    manager->StartRecording("perf_test.csv");

    // Log 10k samples
    for (int i = 0; i < 10000; i++) {
        manager->LogSample(0, 1000.0f + i, "rpm");
    }

    manager->StopRecording();
    auto elapsed = std::chrono::steady_clock::now() - start;

    // Deve completar em < 5 segundos
    EXPECT_LT(elapsed.count(), 5e9);
}
```

#### **Tarefa 3.3: Integration Test** (2h)

```cpp
TEST_F(RecordingIntegration, FullWorkflow) {
    // 1. Start recording
    manager->StartRecording("workflow_test.csv");
    EXPECT_TRUE(manager->IsRecording());

    // 2. Log samples
    for (int i = 0; i < 100; i++) {
        manager->LogSample(0, 1500.0f + i, "rpm");
    }

    // 3. Stop
    manager->StopRecording();
    EXPECT_FALSE(manager->IsRecording());

    // 4. Verify file
    auto stats = CalculateStats("workflow_test.csv");
    EXPECT_FLOAT_EQ(stats.min_value, 1500.0f);
    EXPECT_FLOAT_EQ(stats.max_value, 1599.0f);
}
```

#### **Tarefa 3.4: Documentation** (1h)

- [ ] README atualizado
- [ ] API documentada
- [ ] Exemplos de uso
- [ ] Troubleshooting

---

## 📋 Dependências Entre Tasks

```
Day 1:
┌─────────────────────────────────────────┐
│ 1.1: RecordingManager Base              │
└──────────────┬──────────────────────────┘
               │
      ┌────────┴────────┐
      ▼                 ▼
┌──────────────┐   ┌──────────────┐
│ 1.2: CSV     │   │ 1.3: UI      │
│ Writer       │   │ Integration  │
└──────┬───────┘   └──────┬───────┘
       │                  │
       └──────────┬───────┘
                  ▼
          ┌──────────────┐
          │ 1.4: Data    │
          │ Capture      │
          └──────┬───────┘
                 ▼
          ┌──────────────┐
          │ 1.5: Tests   │
          └──────────────┘

Day 2:
┌──────────────────────────────────┐
│ 2.1: Stats Calculator            │
└───────┬────────────────────────┬─┘
        │                        │
        ▼                        ▼
┌──────────────┐        ┌──────────────┐
│ 2.2: Graph   │        │ 2.3: Export  │
│ Screen       │        │ Formats      │
└──────┬───────┘        └──────┬───────┘
       │                       │
       └───────────┬───────────┘
                   ▼
           ┌──────────────┐
           │ 2.4: Session │
           │ Management   │
           └──────────────┘

Day 3: Parallel
┌──────────────────────────────────────────┐
│ 3.1: NASA P10 │ 3.2: Perf │ 3.3: Integ   │
│ Compliance    │ Test      │ Test & 3.4   │
└──────────────────────────────────────────┘
```

---

## 🧪 Test Coverage Goals

```
File                      Coverage Goal    Current
─────────────────────────────────────────────────
RecordingManager.cpp      > 90%            0%
DataLogFile.cpp           > 85%            0%
GraphScreen.cpp (enhanced) > 80%           ~60%
LiveScreen.cpp (modified)  > 75%           ~70%

Overall:                  > 85%            ~40%
```

---

## 📈 Velocity Estimate

```
Task              Estimated    Actual    Variance
────────────────────────────────────────────────
1.1: RecordManager  3h         ?
1.2: CSV Writer     3h         ?
1.3: UI Integration 2h         ?
1.4: Data Capture   2h         ?
1.5: Tests          2h         ?
────────────────────────────────────────────────
  Subtotal Day 1:  12h

2.1: Stats Calc     2h         ?
2.2: Graph Screen   3h         ?
2.3: Multi Export   2h         ?
2.4: Session Mgmt   2h         ?
────────────────────────────────────────────────
  Subtotal Day 2:   9h

3.1: NASA P10       2h         ?
3.2: Perf Test      1h         ?
3.3: Integration    2h         ?
3.4: Documentation  1h         ?
────────────────────────────────────────────────
  Subtotal Day 3:   6h
────────────────────────────────────────────────
  TOTAL:           27h ≈ 3-4 working days
```

---

## ✅ Definition of Done

```
Feature: DataLogging System

DONE CRITERIA:
✅ 1. Code follows C++20/23 + NASA P10 standards
       └─ No raw new/delete, RAII only
       └─ Bounded loops (max buffer size)
       └─ Assertions on all inputs
       └─ No recursion

✅ 2. All automated tests pass
       └─ Unit tests (RecordingManager, DataLogFile)
       └─ Integration tests (full workflow)
       └─ Performance tests (throughput > 10k samples/sec)

✅ 3. Build is warning-free
       └─ clang-tidy clean
       └─ -Wall -Wextra -Werror passes
       └─ UBSAN/ASAN clean

✅ 4. Documentation complete
       └─ API documented
       └─ Usage examples
       └─ Test coverage > 85%

✅ 5. Code review approved
       └─ Standards checklist passed
       └─ Performance acceptable
       └─ Security review passed

IF ANY ITEM ABOVE IS RED → NOT DONE
```

---

## 📚 References

**Documentos criados:**
1. `ANALISE_DATALOGGING.md` - Status atual
2. `GUIA_DATALOGGING_IMPLEMENTACAO.md` - Código exemplo
3. Este documento - Gaps e plano

**Próximos passos:**
1. Revisar este plano com time
2. Ajustar estimativas baseado em feedback
3. Iniciar Day 1 tarefas
4. Daily standups para tracking
