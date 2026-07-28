# Diagramas Arquiteturais - Interface Gráfica
## Visualização das Refatorações

---

## 📐 1. ARQUITETURA ATUAL vs PROPOSTA

### Arquitetura Atual (Monolítica)

```
┌─────────────────────────────────────────────────────────┐
│                        main.cpp                          │
│                                                          │
│  ┌───────────────┐  ┌───────────────┐  ┌────────────┐  │
│  │   LiveScreen  │  │  GraphScreen  │  │ DashScreen │  │
│  │   (raw ptr)   │  │   (raw ptr)   │  │ (raw ptr)  │  │
│  └───────────────┘  └───────────────┘  └────────────┘  │
│                                                          │
│  🔴 Problems: Raw pointers, memory unsafe              │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   ui_common.cpp (1031 linhas)           │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  DrawTopBar() - 130 linhas, P10=22, DRY violado │   │
│  │                                                   │   │
│  │  • Color conversion boilerplate (3x)              │   │
│  │  • Status indicator logic                         │   │
│  │  • Signal bars (5x loop)                          │   │
│  │  • Table status (3x conditional)                  │   │
│  │  • Freshness display                              │   │
│  │  • Time formatting                                │   │
│  │  • Layout positioning (magic numbers)             │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  DrawBottomNav() - 80 linhas, P10=14             │   │
│  │  Renderização de badges (duplicado)              │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  DrawSensorDetailModal() - 50 linhas             │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘

🔴 Problemas:
   • Monolítico
   • Altamente acoplado
   • Difícil de testar
   • Duplicação de código
```

---

### Arquitetura Proposta (Componentizada)

```
┌────────────────────────────────────────────────────────────────┐
│                     ScreenManager (novo)                       │
│              Gerencia transições de telas com RAII              │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │unique_ptr<>  │  │unique_ptr<>  │  │unique_ptr<>  │         │
│  │ LiveScreen   │  │ GraphScreen  │  │ DashScreen   │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
│                                                                 │
│  ✅ Memory safe, RAII guaranteed                             │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                  UI Components (novos)                         │
│                                                                 │
│  ┌─────────────────────┐  ┌─────────────────────┐             │
│  │TopBar Components     │  │Modal Components      │             │
│  │                      │  │                      │             │
│  │ • StatusIndicator    │  │ • SensorListEditor   │             │
│  │ • SignalBars         │  │ • GenericModal       │             │
│  │ • TableStatus        │  │                      │             │
│  │ • TimeDisplay        │  │ Each: P10 < 5        │             │
│  │ • SessionMgr         │  │ Testável             │             │
│  │                      │  │ Reutilizável         │             │
│  │ Each: P10 < 5        │  └─────────────────────┘             │
│  │ Composable           │                                       │
│  │ Testável             │                                       │
│  └─────────────────────┘                                       │
│                                                                 │
│  ┌─────────────────────┐  ┌─────────────────────┐             │
│  │UI Utilities (refat)  │  │Layout Constants      │             │
│  │                      │  │                      │             │
│  │ • ColorConversion    │  │ • Button sizes       │             │
│  │ • ColorUtils         │  │ • Padding values     │             │
│  │ • Badge              │  │ • Table dimensions   │             │
│  │                      │  │                      │             │
│  │ DRY: -40% dup code   │  │ Layout: Centralized  │             │
│  └─────────────────────┘  └─────────────────────┘             │
└────────────────────────────────────────────────────────────────┘

✅ Benefícios:
   • Modular
   • Bem acoplado
   • Testável
   • Componentizado
   • DRY
```

---

## 🔄 2. FLUXO DE REFATORAÇÃO

### Timeline de Implementação

```
Semana 1        Semana 2        Semana 3        Semana 4
├─────────────────────────────────────────────────────┤

ColorUtils       TopBar          ScreenManager    Testing
(0.5h)          Components      (1.5h)          (2h)
│               (2h)             │               │
├─ helper        │               ├─ unique_ptr    ├─ Unit
│  functions     ├─ Extract      │  ownership     │  tests
│                │  7 methods    │ ├─ WindowMgr  │
│                │                │  (0.75h)      ├─ Integr.
Badge Component │               │               │  tests
(1h)            │               │ const-corr.   │
│               │               │ (1h)          ├─ Perf.
├─ Eliminate    │               │               │  tests
│  duplicates   │               ├─ Bounds       │
│  in 2 files   │               │  validation   └─ Code
│                │               │  (1h)        │  Review
Table Defs      Modal Extract   │
(0.75h)         (1.5h)          TimingManager
│               │               (0.5h)
├─ Centralize    ├─ P10: 18→2    │
│  column setup  │ Extract       └─ Unify
│                │  callbacks      time APIs
│
Layout Const.    GraphScreen
(1h)            Refactor (1h)
│               │
└─ Magic nos     └─ Break into
  eliminated      smaller methods
```

---

## 📊 3. ANTES vs DEPOIS - MÉTRICAS

### Complexidade Cognitiva (NASA P10)

```
ANTES (Acima do recomendado)        DEPOIS (Acima do recomendado)
─────────────────────────────────    ─────────────────────────────────

DrawTopBar          ████████████████ 22    StatusIndicator       ██ 2
                    (CRÍTICO)                                      (OK)

EditSensorModal     ████████████ 18         SensorListEditor      ██ 2
                    (CRÍTICO)                                      (OK)

DashScreen Render   ██████████████████ 28   DashTopControls       ███ 3
                    (CRÍTICO)                                      (OK)
                                            DashWidgetGrid        ███ 3
GraphScreen OnEnter ██████████ 15                                 (OK)
                    (ALTO)

Média de arquivo    ████████████ 15         Média de arquivo      ███ 5
                    (ACIMA)                                        (BELO)

                                    ├─ Target (NASA) = 8 max
```

---

### Duplicação de Código (DRY)

```
ANTES (60+ linhas duplicadas)       DEPOIS (DRY violations eliminadas)
─────────────────────────────────    ─────────────────────────────────

Color Conversion                     ColorUtils Helper
├─ ui_common.cpp (7 linhas)         ├─ Single function
├─ TopBar.cpp (7 linhas)  ←→ DUPE   ├─ Reusable everywhere
├─ BottomNav.cpp (7 linhas) ←→ DUPE │
└─ Other: +10                        └─ Total: 5 linhas


Badge Rendering                      Badge Component
├─ ui_common.cpp (25 linhas)        ├─ Single class
├─ BottomNav.cpp (25 linhas) ←→ DUPE├─ Reusable
└─ (Future: 3x more?)                └─ Total: 20 linhas
                                      (20% size reduction)

EditSensorModal                      SensorListEditorModal
├─ ui_live_screen.cpp (55 linhas)   ├─ Encapsulated
├─ (Future: N screens?)  ←→ ?        ├─ Reusable
└─ Total: 55+ linhas                 └─ Total: 40 linhas
                                      (27% size reduction)

TOTAL DUPLICAÇÃO: ~60 linhas         TOTAL DUPLICAÇÃO: ~10 linhas
Redução: -83% ✅
```

---

### CppCoreGuidelines Compliance

```
ANTES (6 violations)                DEPOIS (0 violations)
─────────────────────────────────    ─────────────────────────────────

R.3 - Raw pointers                   ✅ unique_ptr everywhere
├─ g_live_screen (nullptr)           ├─ ScreenManager owns
├─ g_graph_screen (nullptr)          ├─ RAII guaranteed
├─ g_dash_screen (nullptr)           └─ Memory-safe
├─ g_dtc_screen (nullptr)
├─ g_logs_screen (nullptr)
└─ g_current_screen (nullptr)

C.44 - Encapsulation                 ✅ Bounds validation
├─ simulatedSensors[i] unsafe        ├─ SensorIndexValidator
├─ No bounds checking                └─ IsValid(idx) guard
└─ Potential undefined behavior

C.12 - const-correctness             ✅ Full const correctness
├─ Render methods not const          ├─ const where appropriate
├─ Helper methods not const          └─ Consistent throughout
└─ Loss of const contract

C.47 - Member initialization         ✅ In-class initialization
├─ Some members uninitialized        ├─ fade_alpha_ = 0.0f
└─ Potential undefined behavior      └─ All members explicit

C.39 - Clear contracts               ✅ WindowManager wrapper
├─ GetAppWindow() nullable           ├─ nullptr → handled
└─ Callers unaware                   └─ Explicit contract

ES.4 - Runtime magic numbers         ✅ Named constants
├─ 15+ magic values                  ├─ Layout::Button::*
├─ Scattered in code                 └─ Centralized
└─ Hard to maintain

VIOLATIONS: 6              VIOLATIONS: 0
SCORE: 40/100             SCORE: 100/100 ✅
```

---

## 🏗️ 4. DEPENDÊNCIAS DE REFATORAÇÃO

### Grafo de Dependências

```
                    ┌─────────────────────────┐
                    │ ColorUtils Helper       │
                    │ (Foundation)            │
                    └───────────┬─────────────┘
                                │
                    ┌───────────┴─────────────┐
                    │                         │
            ┌───────▼────────┐      ┌────────▼──────────┐
            │ Badge Component│      │ TopBar Components│
            │ (DRY fix)      │      │ (P10 reduction)   │
            └────────────────┘      └────────────────┘
                    │                        │
                    │      ┌────────────────┬┘
                    │      │                │
            ┌───────▼──────▼──┐   ┌────────▼──────────┐
            │ Table Definitions│   │ Modal Extraction  │
            │ Layout Constants │   │ (P10 reduction)   │
            └──────────────────┘   └───────────────────┘
                    │
                    │
    ┌───────────────┴────────────────────┐
    │                                    │
┌───▼────────────┐          ┌───────────▼────┐
│ ScreenManager  │          │ WindowManager  │
│ (CppCG fix)    │          │ (CppCG fix)    │
└────────────────┘          └────────────────┘
    │
    │
┌───▼─────────────────┐
│ const-correctness   │
│ Bounds validation   │
│ Member init         │
│ TimingManager       │
└─────────────────────┘

→ Implementar na ordem acima
```

---

## 📈 5. PROGRESSO ESPERADO

### Por Semana

```
Semana 1: Foundation (DRY)
═════════════════════════════════════════════════════════
Tasks: ColorUtils, Badge, Table Defs, Layout Const
Lines removed: ~60 (duplicates)
P10 Reduction: ~5% (small impact)
CppCG: -0% (não toca)
Confidence: ████████░░ 80%
Risk: ████░░░░░░ 40%

Semana 2: Modularization (P10 Reduction)
═════════════════════════════════════════════════════════
Tasks: TopBar Components, Modal Extract, GraphScreen Refactor
Lines removed: ~50 (simplification)
P10 Reduction: ~75% (significant!)
CppCG: -0% (não toca)
Confidence: ██████░░░░ 60% (mais complexo)
Risk: ██████░░░░ 60%

Semana 3: Safety (CppCoreGuidelines)
═════════════════════════════════════════════════════════
Tasks: ScreenManager, WindowManager, const-correctness, etc
Lines removed: ~20 (refactor)
P10 Reduction: ~10% (indirect)
CppCG: +100% (100% compliance!)
Confidence: ████████░░ 80%
Risk: ████░░░░░░ 40%

Semana 4: Polish (Testing & Documentation)
═════════════════════════════════════════════════════════
Tasks: Tests, documentation, code review, final integration
Lines removed: ~0 (tests are new)
Coverage Gained: ~30% (new tests)
Confidence: ██████████ 100%
Risk: ██░░░░░░░░ 20%

TOTAL PROGRESS
═════════════════════════════════════════════════════════
Start:   Duplicação=60, P10=15, CppCG=40%
Semana1: Duplicação=50, P10=14, CppCG=40% ✓ 0%
Semana2: Duplicação=20, P10=5,  CppCG=40% ✓ 60%
Semana3: Duplicação=10, P10=5,  CppCG=100% ✓ 90%
Semana4: Duplicação=10, P10=5,  CppCG=100% ✓ 100%
         (Tests=70%, Docs=100%)
```

---

## 🎯 6. MAPA DE IMPACTO

### Mudanças em Cada Arquivo

```
MODIFICADOS:
┌────────────────────────────────────────────┐
│ ui_common.cpp (1031 → 800 linhas)          │
│ ├─ Remove DrawTopBar: -130 linhas          │
│ ├─ Usar ColorUtils: -20 linhas             │
│ ├─ Usar Badge: -30 linhas                  │
│ └─ Limpar UI calls: -30 linhas             │
│ Impacto: ALTO - Core file                  │
└────────────────────────────────────────────┘

┌────────────────────────────────────────────┐
│ main.cpp (794 → 750 linhas)                │
│ ├─ Remove raw pointers: -50 linhas         │
│ ├─ Use ScreenManager: +10 linhas           │
│ └─ Init/Cleanup simpler: -5 linhas         │
│ Impacto: CRÍTICO - Ownership fix           │
└────────────────────────────────────────────┘

┌────────────────────────────────────────────┐
│ ui_live_screen.cpp (318 → 300 linhas)     │
│ ├─ Remove inline modal: -55 linhas         │
│ ├─ Use SensorListEditorModal: +5 linhas    │
│ └─ Cleaner flow: -5 linhas                 │
│ Impacto: MÉDIO - Cleaner                   │
└────────────────────────────────────────────┘

┌────────────────────────────────────────────┐
│ screens/GraphScreen.cpp (1289 → 1200 l.)  │
│ ├─ Refactor OnEnter: -89 linhas            │
│ └─ Break into methods: +30 linhas          │
│ Impacto: MÉDIO - P10 reduction             │
└────────────────────────────────────────────┘

NOVOS:
┌────────────────────────────────────────────┐
│ utils/ColorConversion.h (+30 linhas)       │
│ ui/components/Badge.h/cpp (+60 linhas)     │
│ ui/components/TopBarComponents.h/cpp       │
│   (+200 linhas, split from mono)           │
│ ui/modals/SensorListEditorModal.h/cpp      │
│   (+70 linhas, split from mono)            │
│ core/ScreenManager.h/cpp (+80 linhas)      │
│ ...outros componentes                      │
│                                            │
│ Total novo: ~500 linhas (componentes)      │
│ Substituindo: ~130 linhas (refatoradas)    │
│ Net: +370 linhas (for modularity) ✅       │
└────────────────────────────────────────────┘

NET LINES:
─────────────────────────────────────
Before:    ~4500 linhas (main UI files)
After:     ~4700 linhas (mais modular)
Change:    +200 linhas (+4%)

Mas:
✅ Duplicação: -60 linhas (-83%)
✅ Modularity: +500 linhas (reutilizáveis)
✅ P10 avg: 15 → 5 (-66%)
✅ CppCG: 40% → 100% compliance
```

---

## 🚀 7. CHECKLIST VISUAL

### Status Geral

```
┌──────────────────────────────────────────────┐
│         REFACTORING STATUS TRACKER           │
├──────────────────────────────────────────────┤
│                                              │
│ DRY Violations ░░░░░░░░░░░░░░░░░░░░ 0%      │
│               ▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░ 30%    │ (Semana 1)
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░ 70%    │ (Semana 2)
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░ 95%    │ (Semana 3)
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 100%   │ (Semana 4)
│                                              │
│ P10 Reduction ░░░░░░░░░░░░░░░░░░░░ 0%       │
│              ▓░░░░░░░░░░░░░░░░░░░░░░ 10%    │ (Semana 1)
│              ▓▓▓▓░░░░░░░░░░░░░░░░░░░ 20%    │ (Semana 2)
│              ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░ 90%    │ (Semana 3)
│              ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 100%   │ (Semana 4)
│                                              │
│ CppCG Compliance ░░░░░░░░░░░░░░░░░░░░ 0%    │
│                 ░░░░░░░░░░░░░░░░░░░░░░ 0%   │ (Semana 1-2)
│                 ▓▓▓▓▓▓▓░░░░░░░░░░░░░░░ 30%  │ (Semana 3)
│                 ▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░ 50%  │
│                 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░ 90%   │
│                 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 100%  │ (Semana 4)
│                                              │
│ Test Coverage  ░░░░░░░░░░░░░░░░░░░░ 40%     │
│               ▓▓▓▓░░░░░░░░░░░░░░░░░░ 40%    │ (Semana 1-2)
│               ▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░ 60%   │ (Semana 3)
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░ 70%   │
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░ 95%    │ (Semana 4)
│               ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓ 100%   │
│                                              │
└──────────────────────────────────────────────┘

Legend: ░ = Not done  ▓ = Done/In progress  ▓ = Completed
```

---

## 🎓 8. KNOWLEDGE TRANSFER

### Documentação por Tópico

```
┌─────────────────────────────────────────┐
│        ARQUIVOS DE REFERÊNCIA            │
├─────────────────────────────────────────┤
│                                          │
│ REVIEW_UI_ARCHITECTURE.md                │
│ ├─ 20 páginas                            │
│ ├─ Violações detalhadas                  │
│ ├─ DRY, P10, CppCG                       │
│ └─ Recomendações específicas             │
│                                          │
│ REFACTORING_PRACTICAL_GUIDE.md          │
│ ├─ 15 páginas                            │
│ ├─ Código completo                       │
│ ├─ Passo-a-passo                         │
│ └─ Padrões de implementação              │
│                                          │
│ IMPLEMENTATION_CHECKLIST.md              │
│ ├─ 10 páginas                            │
│ ├─ Tarefas detalhadas                    │
│ ├─ Estimativas de tempo                  │
│ └─ Definition of Done                    │
│                                          │
│ EXECUTIVE_SUMMARY.md                     │
│ ├─ 8 páginas                             │
│ ├─ Overview executivo                    │
│ ├─ ROI e métricas                        │
│ └─ Recomendações                         │
│                                          │
│ THIS FILE (Diagramas)                    │
│ ├─ Visual overviews                      │
│ ├─ Métricas gráficas                     │
│ └─ Mapa de impacto                       │
│                                          │
└─────────────────────────────────────────┘

Comece por: EXECUTIVE_SUMMARY.md
Depois: REVIEW_UI_ARCHITECTURE.md
Para implementar: REFACTORING_PRACTICAL_GUIDE.md
Track progress: IMPLEMENTATION_CHECKLIST.md
Visualize: THIS FILE
```

---

## 📞 Suporte e Contato

```
┌────────────────────────────────────────┐
│      ESCALATION MATRIX                 │
├────────────────────────────────────────┤
│                                        │
│ Technical Questions:                   │
│ → Review REVIEW_UI_ARCHITECTURE.md     │
│ → Reference REFACTORING_PRACTICAL_GUIDE│
│ → Ask tech lead in daily standup       │
│                                        │
│ Implementation Blocker:                │
│ → Check IMPLEMENTATION_CHECKLIST.md    │
│ → Ask Architect                        │
│ → Create bug if blocker                │
│                                        │
│ Resource Request:                      │
│ → Update IMPLEMENTATION_CHECKLIST.md   │
│ → Ask Project Manager                  │
│ → Negotiate sprint capacity             │
│                                        │
│ Timeline Concern:                      │
│ → Review this diagram (expectations)   │
│ → Discuss with Scrum Master            │
│ → Adjust scope if needed                │
│                                        │
│ Quality Question:                      │
│ → Read EXECUTIVE_SUMMARY.md            │
│ → Check test coverage metrics          │
│ → Ask QA Lead                          │
│                                        │
└────────────────────────────────────────┘
```

---

**Preparado:** 13 de janeiro de 2026
**Versão:** 1.0 FINAL
**Status:** ✅ PRONTO PARA IMPLEMENTAÇÃO

