# Checklist de Implementação - Refatoração UI
## Rastreamento de Progresso

---

## 📌 FASE 1: REFATORAÇÃO DRY (Semana 1)

### Task 1.1: ColorUtils Helper
- [ ] Criar `utils/ColorConversion.h`
  - [ ] Função `GetColorChannel()`
  - [ ] Função `NormalizeChannel()`
  - [ ] Função `ImU32ToImVec4()`
  - [ ] Helper `TextColored()`
- [ ] Adicionar testes unitários
  - [ ] Test: conversão RGB válida
  - [ ] Test: limite de canal (0-255)
  - [ ] Test: normalização (0.0-1.0)
- [ ] Documentar com comentários C++
- [ ] Atualizar `ui_common.cpp` para usar ColorUtils
  - [ ] DrawTopBar() - 3 ocorrências
  - [ ] DrawBottomNav() - 1 ocorrência
  - [ ] GetStatusColor() - mantém chamada mas internamente usa novo
- [ ] Validar compilação
- [ ] Code review

**Estimativa:** 45 minutos
**Status:** ⏳ Não iniciado

---

### Task 1.2: Badge Component
- [ ] Criar `ui/components/Badge.h`
  - [ ] Struct `BadgeConfig`
  - [ ] Classe `Badge` com métodos estáticos
  - [ ] Método `RenderCountBadge()`
  - [ ] Método `RenderCountBadgeAtCorner()`
- [ ] Implementar `ui/components/Badge.cpp`
  - [ ] `RenderCircle()`
  - [ ] `RenderText()`
  - [ ] Validação de entrada
- [ ] Adicionar testes unitários
  - [ ] Test: render com count=0 (não renderiza)
  - [ ] Test: count > max_display limita
  - [ ] Test: posicionamento correto
- [ ] Atualizar usages
  - [ ] `ui_common.cpp:210-240` → usar Badge
  - [ ] `ui/BottomNav.cpp:95-120` → usar Badge
- [ ] Remover código duplicado
- [ ] Validar compilação
- [ ] Code review

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

---

### Task 1.3: Table Column Definitions
- [ ] Criar `ui/TableDefinitions.h`
  - [ ] Struct `ColumnDef`
  - [ ] Constante `SENSOR_COLUMNS`
  - [ ] Função `SetupColumns()`
- [ ] Identificar todas as uses de TableSetupColumn()
  - [ ] `ui_live_screen.cpp:155-165`
  - [ ] Buscar outras
- [ ] Atualizar com nova abordagem
- [ ] Validar compilação
- [ ] Code review

**Estimativa:** 45 minutos
**Status:** ⏳ Não iniciado

---

### Task 1.4: Layout Magic Numbers
- [ ] Expandir `utils/Layout.h` com novos namespaces
  - [ ] `Button::` - larguras e alturas
  - [ ] `Padding::` - espaçamentos
  - [ ] `Table::` - dimensões de coluna
- [ ] Identificar todos os magic numbers em:
  - [ ] `ui_live_screen.cpp`
  - [ ] `ui_common.cpp`
  - [ ] `ui/*.cpp`
- [ ] Substituir magic numbers por constantes
- [ ] Manter nomes descritivos
- [ ] Validar compilação
- [ ] Code review

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

**Subtotal Fase 1:** ~3.25 horas
**Status:** ⏳ Não iniciado

---

## 🔧 FASE 2: REDUÇÃO P10 (Semana 2)

### Task 2.1: TopBar Components
- [ ] Criar `ui/components/TopBarComponents.h`
  - [ ] Classe `ECUStatusIndicator`
  - [ ] Classe `SignalStrengthBars`
  - [ ] Classe `TableStatusDisplay`
  - [ ] Classe `SensorFreshnessDisplay`
  - [ ] Classe `TopBarTimeDisplay`
  - [ ] Classe `SessionManager`

- [ ] Implementar `ui/components/ECUStatusIndicator.cpp`
  - [ ] Método `Render()`
  - [ ] Lógica simples (P10: ~2)

- [ ] Implementar `ui/components/SignalStrengthBars.cpp`
  - [ ] Método `Render()`
  - [ ] Método `CalculateSignalStrength()`
  - [ ] Método `RenderBar()`
  - [ ] Valores das constantes bem documentados

- [ ] Implementar outros componentes (análogo)

- [ ] Refatorar `DrawTopBar()` em `ui_common.cpp`
  - [ ] Remover 100+ linhas de implementação
  - [ ] Chamar componentes
  - [ ] P10 deve ser ~2

- [ ] Testes unitários
  - [ ] Test SignalStrengthBars: latência boa
  - [ ] Test SignalStrengthBars: latência ruim
  - [ ] Test TableStatusDisplay: cada tabela

- [ ] Validar compilação
- [ ] Visual regression testing

**Estimativa:** 2 horas
**Status:** ⏳ Não iniciado

---

### Task 2.2: Modal Extraction
- [ ] Criar `ui/modals/SensorListEditorModal.h`
  - [ ] Typedef `OnConfirmCallback`
  - [ ] Método `Open()`
  - [ ] Método `Close()`
  - [ ] Método `Render()`
  - [ ] Getter `GetCurrentSelection()`
  - [ ] `SetOnConfirm()`

- [ ] Implementar `ui/modals/SensorListEditorModal.cpp`
  - [ ] `RenderSensorTree()` - P10: ~5
  - [ ] `RenderButtons()` - P10: ~2
  - [ ] `IsSensorSelected()` - P10: ~1
  - [ ] `ToggleSensorSelection()` - P10: ~2

- [ ] Integrar em `LiveScreen`
  - [ ] Adicionar member `sensor_list_modal_`
  - [ ] Setup callback em `OnEnter()`
  - [ ] Chamar `Render()` no loop principal

- [ ] Remover código antigo de `ui_live_screen.cpp`
  - [ ] Modal popup antigo (~55 linhas)
  - [ ] Container manipulation antigo

- [ ] Testes unitários
  - [ ] Test: modal abre/fecha
  - [ ] Test: seleção persiste
  - [ ] Test: callback chamado

- [ ] Validar compilação
- [ ] Visual testing

**Estimativa:** 1.5 horas
**Status:** ⏳ Não iniciado

---

### Task 2.3: GraphScreen Refactor
- [ ] Analisar `GraphScreen::OnEnter()` atual
- [ ] Quebrar em métodos privados:
  - [ ] `LoadStateFromManager()`
  - [ ] `SyncPinnedSensorsToGraph()`
  - [ ] `InitializeGraphMetadata()`
  - [ ] `CacheColorConversions()`

- [ ] Cada método deve ter P10 < 5
- [ ] Testes para cada método
- [ ] Documentação clara

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

**Subtotal Fase 2:** ~4.5 horas
**Status:** ⏳ Não iniciado

---

## 🛡️ FASE 3: CppCoreGuidelines (Semana 3)

### Task 3.1: ScreenManager
- [ ] Criar `core/ScreenManager.h`
  - [ ] Enum `Screen`
  - [ ] Classe `ScreenManager`
  - [ ] Getter `Instance()`
  - [ ] Método `GetCurrentScreen()`
  - [ ] Método `SetCurrentScreen()`
  - [ ] Método `GetScreen(Screen)`

- [ ] Implementar `core/ScreenManager.cpp`
  - [ ] Singleton pattern
  - [ ] `unique_ptr` para ownership
  - [ ] `TransitionToScreen()` com validação
  - [ ] `Initialize()`, `Update()`, `Render()`, `Shutdown()`

- [ ] Validar RAII
  - [ ] Sem memory leaks
  - [ ] Destrutores chamados em ordem

- [ ] Integrar em `main.cpp`
  - [ ] Remover 5 raw pointers globais
  - [ ] Usar `ScreenManager::Instance()`
  - [ ] Cleanup automático

- [ ] Testes unitários
  - [ ] Test: transição entre screens
  - [ ] Test: callbacks OnEnter/OnExit

- [ ] Code review

**Estimativa:** 1.5 horas
**Status:** ⏳ Não iniciado

---

### Task 3.2: WindowManager
- [ ] Criar `core/WindowManager.h`
  - [ ] Método `GetWindow()` com validação
  - [ ] Método `HasWindow()`
  - [ ] Método `SetWindow()`
  - [ ] Assertions para nullptr

- [ ] Atualizar `TopBar.cpp`
  - [ ] Usar `WindowManager` ao invés de `GetAppWindow()`
  - [ ] Adicionar verificação de nullptr

- [ ] Remover função global `GetAppWindow()`
  - [ ] Se não mais necessária

- [ ] Testes unitários
  - [ ] Test: erro se window é nullptr

**Estimativa:** 45 minutos
**Status:** ⏳ Não iniciado

---

### Task 3.3: const-Correctness
- [ ] Revisar `ui/TopBar.cpp`
  - [ ] Marcar métodos `const` quando apropriado
  - [ ] Verificar se modificam estado

- [ ] Revisar `ui/BottomNav.cpp`
  - [ ] `UpdateBadgeCounts()` - não const (modifica)
  - [ ] `Render()` - não const (modifica)
  - [ ] Renderizers privados - const

- [ ] Revisar `screens/*.cpp`
  - [ ] `Render()` - geralmente const
  - [ ] `Update()` - não const
  - [ ] Helpers privados - const quando possível

- [ ] Compilar com `-Wextra -Wconst` para validar

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

---

### Task 3.4: Validação de Bounds
- [ ] Revisar `ui_live_screen.cpp`
  - [ ] Loops sobre `sensors_to_display`
  - [ ] Adicionar `IsSensorValid(idx)` check
  - [ ] Ou usar wrapper com validação

- [ ] Criar `utils/SensorIndexValidator.h`
  - [ ] Método `IsValid()`
  - [ ] Getter `GetSensor()` com default
  - [ ] Assertions em debug

- [ ] Aplicar em locais de risco
  - [ ] `simulatedSensors[i]` accesses
  - [ ] ID generation

- [ ] Testes unitários
  - [ ] Test: índice inválido retorna valor seguro

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

---

### Task 3.5: Inicialização de Membros
- [ ] Revisar todos os `.h` de classe
  - [ ] `BaseScreen.h`
  - [ ] `ui/TopBar.h`
  - [ ] `ui/BottomNav.h`
  - [ ] etc.

- [ ] Adicionar inicializadores em linha (C++11)
  - [ ] `float fade_alpha_ = 0.0f;`
  - [ ] `bool is_active_ = false;`
  - [ ] Etc.

- [ ] Compilar com `-Wmissing-field-initializers`

**Estimativa:** 30 minutos
**Status:** ⏳ Não iniciado

---

### Task 3.6: TimingManager
- [ ] Criar `core/TimingManager.h`
  - [ ] Singleton
  - [ ] Método `GetCurrentTime()`
  - [ ] Método `GetPulseAnimation()`
  - [ ] Método `GetTimeSince()`

- [ ] Centralizar todas as chamadas de tempo
  - [ ] Remover `glfwGetTime()` direto
  - [ ] Remover `ImGui::GetTime()` direto
  - [ ] Usar `TimingManager`

- [ ] Testes unitários
  - [ ] Test: tempo é monotônico

**Estimativa:** 1 hora
**Status:** ⏳ Não iniciado

**Subtotal Fase 3:** ~5.25 horas
**Status:** ⏳ Não iniciado

---

## ✅ VERIFICAÇÃO FINAL (Semana 4)

### Code Review Checklist
- [ ] Todos os arquivos têm headers de licença/comentários
- [ ] Sem compilador warnings (`-Wall -Wextra -Werror`)
- [ ] Formatação consistente (style guide do projeto)
- [ ] Nomes de variáveis descritivos
- [ ] Funções bem documentadas (comentários / doc strings)
- [ ] Sem `TODO` pendentes (ou justificados)
- [ ] Testes passam (100%)
- [ ] No memory leaks (valgrind ou asan)

### Performance Checklist
- [ ] Sem regressions de FPS
- [ ] No hitches/stutters
- [ ] Memory footprint aceitável
- [ ] GPU load aceitável

### Documentation
- [ ] README atualizado com nova arquitetura
- [ ] Exemplos de uso de novos componentes
- [ ] Diagrama de dependências atualizado
- [ ] Migration guide para código antigo

### Git
- [ ] Commits atomizados e descritivos
- [ ] PR description clara
- [ ] Testes no CI/CD passam
- [ ] Code review aprovado (2+ reviewers)

---

## 📊 Status Global

| Fase | Descrição | Tarefas | Status | ETA |
|------|-----------|---------|--------|-----|
| 1 | DRY Refactor | 4 | ⏳ 0/4 | Semana 1 |
| 2 | P10 Reduction | 3 | ⏳ 0/3 | Semana 2 |
| 3 | CppCG | 6 | ⏳ 0/6 | Semana 3 |
| 4 | Final Check | Multi | ⏳ Pendente | Semana 4 |
| **TOTAL** | **Refactor UI** | **13** | **⏳ 0%** | **~20h** |

---

## 📝 Notas de Implementação

### Ordem Recomendada
1. **ColorUtils** (foundation - necessário para Badge)
2. **Badge** (DRY - rápido, impacto imediato)
3. **ScreenManager** (CppCG - importante para resto)
4. **TopBar Components** (P10 - visível no código)
5. **Modal Extraction** (P10 + DRY)
6. **Resto das tarefas** (em paralelo se possível)

### Build & Test
```bash
# Build com warnings como erros
cmake -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror"
cmake --build build

# Testes
./build/tests/ui_tests

# Validar memory
valgrind --leak-check=full ./build/scanner_glfw
```

### Git Workflow
```bash
# Cada tarefa = 1 commit atômico
git checkout -b feature/colourutils-helper
git commit -am "feat: Add ColorUtils helper functions (Task 1.1)"
# ... code review
git merge --squash main

# Ou PR para cada subtarefa
```

### Comunicação
- Daily standup: 15 min status
- Code review: 24h máximo
- Blockers: report ASAP

---

## 🎯 Definição de Pronto (Definition of Done)

Cada tarefa é considerada **PRONTA** quando:

- [ ] Código escrito e compilado
- [ ] Testes passam (>90% coverage)
- [ ] Code review aprovado
- [ ] Documentação atualizada
- [ ] Sem regressions de performance
- [ ] Commits limpos e descritivos
- [ ] Merged para branch principal

---

## 📌 Dependências Críticas

```
ColorUtils
    ↓
Badge (usa ColorUtils)
    ↓
ScreenManager (independente)
    ↓
TopBar Components (usa ColorUtils)
    ↓
Modal Extraction (usa ColorUtils)
    ↓
Resto em paralelo
```

---

## 🚨 Riscos e Mitigação

| Risco | Probabilidade | Impacto | Mitigação |
|-------|--------------|--------|-----------|
| Build quebra | Média | Alto | Compilação frequente, CI/CD |
| Memory leak | Baixa | Crítico | valgrind/asan em testes |
| Visual regression | Média | Médio | Screenshot tests |
| Performance drop | Baixa | Médio | Profiling contínuo |
| Escopo creep | Média | Médio | Manter checklist focado |

---

**Última atualização:** 13 de janeiro de 2026
**Responsável:** Tim de Desenvolvimento UI
**Versão:** 1.0

