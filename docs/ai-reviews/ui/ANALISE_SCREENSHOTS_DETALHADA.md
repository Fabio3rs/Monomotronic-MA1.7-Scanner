# 🎬 ANÁLISE DAS SCREENSHOTS: Encontrados 7 Ajustes

## 📸 Screenshot 1: DASH Screen

```
┌─────────────────────────────────────────────┐
│ ECU: DISCONNECTED ? MODO SIMULAÇÃO   16:44  │
├─────────────────────────────────────────────┤
│ [+ Add Widget] [Reset]                      │
├─────────────────────────────────────────────┤
│                                             │
│ Temp. Água    │ TPS Trilha │ Ângulo Borb.  │
│ -0.0 °C       │    22.1 %  │    55.4 °     │
│ ████░         │ ▼▼░░▼░░░░  │ ╱╱╱╱╱╱╱╱╱     │
│               │   0   100  │               │
│               │            │               │
├──────────────┼────────────┼─────────────────┤
│ Flags Sistem │ Correção A │ Correção Longo │
│ 110.0 flags  │ 632.0 up   │ 38.2 %         │
│ ╲╲╲╲╱        │ ╱╱╱╱╱╱░░░░ │ ░░░░░░░░░░     │
│              │            │                │
├──────────────┼────────────┼──────────────────┤
│ Correção Ad. │ Bloco Adapt│ Bateria (RAM)   │
│ 1872.0 ms    │ 92.0 raw   │ 15.9 V          │
│ ░░░░░░░░░░░░ │ ╱╱╱╱╱╱╱╱╱╱ │ ░░░░░░░░░       │
│              │            │                │
├──────────────┼────────────┼──────────────────┤
│ Adaptação Pu │ RPM Motor  │                 │
│ 0.7 ratio    │ 1000 rpm   │                 │
│ ░░░░░░░░░░░░ │ 🟢🟢🟢     │                 │
│              │            │                 │
└──────────────┴────────────┴──────────────────┘
```

**Issues Encontrados:**

### ✅ O Que Funciona Bem
- Grid layout (3 colunas) é intuitivo
- Widgets têm nome + valor + gráfico
- Cores de fundo diferenciam grupos

### ❌ Problemas

**Problema 1.1: Header Confuso**
```
"ECU: DISCONNECTED ? MODO SIMULAÇÃO"

Questões:
├─ "?" no meio é confuso (truncado?)
├─ DISCONNECTED em vermelho → usuário fica preocupado
├─ MODO SIMULAÇÃO em laranja → redundante
└─ Duas cores vermelha + laranja = caos visual

Solução: Apenas um status
├─ Verde: "✓ CONECTADO"
├─ Azul: "🔬 SIMULAÇÃO"
└─ Vermelho: "✗ DESCONECTADO"
```

**Problema 1.2: Tamanho Inconsistente de Widgets**
```
Observação: RPM Motor (último widget) parece menor

Pode ser:
├─ Layout não preencheu corretamente
├─ Widget espaçamento inconsistente
└─ Responsividade em resolução diferente

Verificar: ImGui::GetWindowSize() vs. widget size
```

**Problema 1.3: Gráficos Não Sincronizados**
```
Gráfico de TPS Trilha 1:
├─ Título: 22.1 %
├─ Range: 0 - 100
└─ Gráfico: mostra descida (não matches 22.1)

Verificar: Dados mock vs. escala do gráfico
```

---

## 📸 Screenshot 2: DTC Screen

```
┌──────────────────────────────────────────┐
│ ECU: DISCONNECTED    16:45              │
├──────────────────────────────────────────┤
│ [Read] [Clear]     3 Active DTCs        │
├──────────────────────────────────────────┤
│                                          │
│ Status    Code       Description         │
│ ────────────────────────────────────────  │
│ ACTIVE    P0171      System Too Lean     │
│ ACTIVE    P0301      Cylinder 1 Misfire │
│ Stored    P0420      Catalyst System ... │
│ Stored    P0562      System Voltage Low  │
│ ACTIVE    C0040      Right Front Wheel..│
│                                          │
└──────────────────────────────────────────┘
        BADGE: 3 (vermelho) ← DTC
```

**Issues Encontrados:**

### ❌ Problema 2.1: Contagem Incorreta

```
Header: "3 Active DTCs"
Tabela mostra:
├─ ACTIVE: P0171 ✓
├─ ACTIVE: P0301 ✓
├─ Stored: P0420 (❌ NÃO está em ACTIVE)
├─ Stored: P0562 (❌ NÃO está em ACTIVE)
└─ ACTIVE: C0040 ✓

Contagem correta: 3 ACTIVE (P0171, P0301, C0040)
                 2 STORED (P0420, P0562)

Problema: "3 Active" está certo, mas design confunde
          porque Stored aparecem na mesma tabela
```

### ❌ Problema 2.2: Separação de Status

```
Solução:

┌─────────────────────────────────────────┐
│ Active DTCs (3) ← APENAS estes contam   │
├─────────────────────────────────────────┤
│ P0171 - System Too Lean          [█ P0]  │
│ P0301 - Cylinder 1 Misfire       [█ P0]  │
│ C0040 - Right Front Wheel        [█ C0]  │
│                                          │
│ Stored DTCs (2) ← Seção separada        │
├─────────────────────────────────────────┤
│ P0420 - Catalyst System          [░ P0]  │
│ P0562 - System Voltage Low       [░ P0]  │
│                                          │
│ [Read] [Clear Active] [Clear All]        │
└─────────────────────────────────────────┘

BottomNav badge: "3" (apenas ACTIVE)
```

### ❌ Problema 2.3: Botão "Read" é Confuso

```
O que "Read" faz?
├─ Relê DTCs da ECU? (redundante, já está sendo feito)
├─ Abre editor?
├─ Exporta?
└─ ???

Solução: Renomear ou indicar com ícone
├─ "🔄 Refresh" = relê DTCs
├─ "📋 Details" = abre descrição completa
└─ "💾 Export" = salva em arquivo
```

---

## 📸 Screenshot 3: LIVE Screen

```
┌───────────────────────────────────────────┐
│ ECU: DISCONNECTED    16:45:22             │
├───────────────────────────────────────────┤
│ [○ Custom] [Default] [✎ Edit] [Reset to] │
│ [● Rec] [‖ Paus] [⚡ Snap]                │
├───────────────────────────────────────────┤
│                                           │
│ Sta... Nome             Valor    Trend   │
│ ──────────────────────────────────────── │
│ 🟢 Temp. Água (ECT)    1.9 °C   ────    │
│ 🟢 TPS Trilha 1        6.5 %    ────    │
│ 🟢 RPM Motor          850.0 rpm ────    │
│ 🟢 Temp. Ar (IAT)     19.6 °C   ↘       │
│ 🟢 Integrador Lambda   -9.8 %   ↘↘      │
│                                           │
│ ⚠️ ALERTA: Sensores críticos fora ...   │
│                                          │
├──────────────────────────────────────────┤
│ [LIVE]  [GRAPH]  [DASH]  [DTC] [LOGS]   │
│                ↑ (fundo azul claro)       │
└──────────────────────────────────────────┘
```

**Issues Encontrados:**

### ❌ Problema 3.1: Alerta Falso em Simulação

```
Problema:
├─ Modo simulação gera dados mock
├─ Dados mock podem estar "fora de range"
├─ Alerta vermelho aparece inapropriado
├─ Usuário fica assustado

Exemplo: Lambda = -9.8% (negativo!)
└─ Cor verde (🟢) mas valor negativo (confuso)

Solução: Não mostrar alerta em simulação
├─ Detectar: IsSimulationMode()
├─ Se true: não renderizar alerta vermelho
├─ Mostrar apenas em ECU real (IsConnected())
```

### ❌ Problema 3.2: Trend Color Não Bate

```
Observação:
├─ Integrador Lambda: -9.8 % (negativo, vermelho esperado)
├─ Mas tem verde (🟢) na coluna Status
├─ Trend mostra ↘↘ (diminuindo, ok)
├─ Mas não há cor de warning

Issue: Cor verde + valor negativo = confuso

Solução:
├─ Trend ↗ = verde (aumentando, bom)
├─ Trend → = cinza (estável, ok)
├─ Trend ↘ = laranja (diminuindo, atenção)
├─ Valor CRÍTICO = vermelho, sobrescreve tudo
```

### ❌ Problema 3.3: Botões Desorganizados

```
Top buttons:
├─ [○ Custom] [Default]  ← Preset selection
├─ [✎ Edit] [Reset to]   ← Actions (confuso)
└─ [● Rec] [‖ Paus] [⚡ Snap] ← Recording controls

Problema: Misturado, sem grupos visuais

Solução:
Seção 1: Presets
├─ [○ Custom] [Default]
│
Seção 2: Coleta
├─ [● START] [⏸ PAUSAR]
│
Seção 3: Ferramentas
├─ [✎ EDITAR] [⚡ SNAPSHOT] [🔧 RESET]
```

### ⚠️ Problema 3.4: Coluna "Sta..." Truncada

```
Vê-se: "Sta... Nome"

Deve ser: Status (completo)

Solução: Aumentar largura coluna ou usar icon só
├─ Option 1: Remover "Sta" → apenas círculo colorido
├─ Option 2: Expandir coluna (mais espaço)
└─ Option 3: Tooltip ao hover
```

---

## 🔍 Análise Comparativa: 3 Telas

```
Métrica              DASH    DTC     LIVE
─────────────────────────────────────────
Widgets organizado   ✅      ✅      ⚠️
Status claro         ⚠️      ⚠️      ❌
Dados precisos       ⚠️      ✅      ❌ (simulação)
Botões agrupados     N/A     ⚠️      ❌
Trend/indicador      ✅      ✅      ❌
Cores consistentes   ✅      ✅      ⚠️
```

---

## 📊 Matriz de Severidade

| Problema | Tela | Severidade | Impacto | Fix |
|----------|------|-----------|---------|-----|
| Alerta falso em simulação | LIVE | 🔴 P0 | Alto | 15 min |
| Status ECU ambíguo | Global | 🔴 P0 | Alto | 20 min |
| Contagem DTC errada | DTC | 🟠 P1 | Médio | 1h |
| Trend color mismatch | LIVE | 🟠 P1 | Médio | 1h |
| Botões desorganizados | LIVE | 🟠 P1 | Médio | 2h |
| Coluna truncada | LIVE | 🟡 P2 | Baixo | 30 min |
| Gráficos mock | DASH | 🟡 P2 | Baixo | 1h |

---

## ✨ Recomendações Rápidas

### **Para Fazer Hoje (1 hora)**

```cpp
// 1. Remover alerta em simulação
if (HasCriticalSensors() && !IsSimulationMode()) {
    RenderCriticalAlert();
}

// 2. Limpar status ECU
const char *ecu_status = GetECUStatusText();  // "✓ CONECTADO" ou "🔬 SIMULAÇÃO"
ImVec4 ecu_color = GetECUStatusColor();
ImGui::TextColored(ecu_color, "ECU: %s", ecu_status);

// 3. Separar DTCs Active/Stored
RenderDTCSection("ACTIVE DTCs", active_dtcs_, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
RenderDTCSection("STORED DTCs", stored_dtcs_, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
```

### **Para Fazer Esta Semana (5 horas)**

- [ ] Reorganizar botões LIVE (agrupar por função)
- [ ] Fix coluna truncada (usar ícones)
- [ ] Melhorar trend color logic
- [ ] Verificar mock data em DASH

### **Para Próxima Sprint**

- [ ] DataLogging (já documentado)
- [ ] Snapshots (histórico)
- [ ] Comparação de dados

---

## 🎯 Conclusão

**3 Telas = 7 Problemas Encontrados**

**Crítico (hoje):** 2 problemas (30 min)
- Alerta falso simulação
- Status ECU claro

**Importante (esta semana):** 3 problemas (5h)
- DTC separado
- Trend color
- Botões agrupados

**Polish (próxima sprint):** 2 problemas (2h)
- Coluna truncada
- Mock data

**Recomendação:** Faça os 2 críticos hoje, depois continua com DataLogging.

