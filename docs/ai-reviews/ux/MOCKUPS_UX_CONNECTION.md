# Mockups UX: Estados da Interface

**Objetivo:** Visualizar os 3 fluxos principais antes/depois

---

## Estado 1: Aplicação Iniciando

### ❌ ANTES (Problema)
```
┌─────────────────────────────────────────────────────────────┐
│ ECU: DISCONNECTED              [🌙] [⛶] [✕]                │
│ ⚠ MODO SIMULAÇÃO ⚠ MODO SIMULAÇÃO ⚠ MODO SIMULAÇÃO...       │ ← Confuso!
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  [Live] [Graph] [Dashboard] [DTC] [Logs]                    │
│                                                               │
│  Sensores... (mas dados de quem? Simulado? Real?)           │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Issues:
- ❌ Sem forma de escolher porta
- ❌ Não mostra qual ECU está conectado
- ❌ Não claro: simulação deliberada ou erro?
- ❌ Usuário confuso no startup
```

### ✅ DEPOIS (Solução)

```
┌─────────────────────────────────────────────────────────────┐
│ ✓ ECU: IAW6E.75 | 15.3ms  [⚙ Connection]   [🌙] [⛶] [✕]   │
│                     └─ Clicável → Modal                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  [Live] [Graph] [Dashboard] [DTC] [Logs]                    │
│                                                               │
│  Dados reais da ECU (OK!)                                   │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Melhorias:
- ✅ Mostra modelo ECU conectado
- ✅ Botão "⚙ Connection" para mudar porta/modo
- ✅ Latência visível
- ✅ Claro: dados reais, não simulado
```

---

## Estado 2: Modal de Conexão (Fluxo Real)

```
┌───────────────────────────────────────────────────────────────┐
│                      ECU Connection                        [X] │
├───────────────────────────────────────────────────────────────┤
│                                                                 │
│ Select Serial Port & Mode:                                     │
│                                                                 │
│ Available Ports:                                                │
│   ● COM3                          (USB Serial Device)           │
│   ○ COM4                          (USB Serial Device) [IN USE]  │
│   ○ /dev/ttyUSB0                  (Industrial Serial)           │
│                                                                 │
│ Connection Mode:                                                │
│   ◉ Real ECU          Use actual ECU data                      │
│   ○ Simulation        Hardcoded test data                      │
│                                                                 │
│ ECU Information:                                                │
│   ✓ Model: IAW 6E.75                                           │
│   ✓ Firmware: 1.23.45                                          │
│   ✓ Table: 1                                                   │
│                                                                 │
│                                                                 │
│               [Connect]  [Cancel]                              │
│                                                                 │
└───────────────────────────────────────────────────────────────┘

Características:
- Porta selecionada visualmente (●)
- Modo com radio buttons
- Feedback ECU info em tempo real (init packets)
- Botões grandes (touch-friendly)
```

---

## Estado 3: Modal de Conexão (Fluxo Simulação)

```
┌───────────────────────────────────────────────────────────────┐
│                      ECU Connection                        [X] │
├───────────────────────────────────────────────────────────────┤
│                                                                 │
│ Select Serial Port & Mode:                                     │
│                                                                 │
│ Available Ports:                                                │
│   ○ COM3                          (USB Serial Device)           │
│   ○ COM4                          (USB Serial Device) [IN USE]  │
│   ○ /dev/ttyUSB0                  (Industrial Serial)           │
│                                                                 │
│ Connection Mode:                                                │
│   ○ Real ECU          Use actual ECU data                      │
│   ◉ Simulation        Hardcoded test data                      │
│                                                                 │
│ ECU Information:                                                │
│   ℹ Connect to see ECU model and firmware                      │
│                                                                 │
│   (Simulação não faz "init" real, usa dados pré-definidos)     │
│                                                                 │
│               [Connect]  [Cancel]                              │
│                                                                 │
└───────────────────────────────────────────────────────────────┘

Características:
- Modo Simulation selecionado (◉)
- ECU Info mostra mensagem informativa
- Usuário sabe: "Vou usar teste, não real"
```

---

## Estado 4: Topbar em Simulação

```
┌─────────────────────────────────────────────────────────────┐
│ ▯ SIMULATION  ℹ Test data only - not real ECU  [⚙ Connection] │
│ └─ Claro e óbvio que é simulado, não erro!                  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  [Live] [Graph] [Dashboard] [DTC] [Logs]                    │
│                                                               │
│  Dados simulados (para teste)                               │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Diferenças vs. Antes:
- ❌ ANTES: "MODO SIMULAÇÃO" pulsing (confundia com erro)
- ✅ DEPOIS: "▯ SIMULATION" + "ℹ Test data only" (claro!)
```

---

## Estado 5: Erro de Conexão

```
┌───────────────────────────────────────────────────────────────┐
│                      ECU Connection                        [X] │
├───────────────────────────────────────────────────────────────┤
│                                                                 │
│ Select Serial Port & Mode:                                     │
│                                                                 │
│ Available Ports:                                                │
│   ● COM3                          (USB Serial Device)           │
│   ○ COM4                          (USB Serial Device) [IN USE]  │
│   ○ /dev/ttyUSB0                  (Industrial Serial)           │
│                                                                 │
│ Connection Mode:                                                │
│   ◉ Real ECU          Use actual ECU data                      │
│   ○ Simulation        Hardcoded test data                      │
│                                                                 │
│ ECU Information:                                                │
│   ⏳ Waiting for ECU response...                               │
│                                                                 │
│ ✗ Connection Error:                                            │
│   Timeout waiting for init response (5s)                       │
│   • Check KLINE connection                                     │
│   • Try different port                                         │
│                                                                 │
│               [Connect]  [Cancel]                              │
│                                                                 │
└───────────────────────────────────────────────────────────────┘

Características:
- Mensagem de erro em vermelho
- Info ECU mostra "Waiting..." durante conexão
- Erro é informativo (não só "FAILED")
```

---

## Estado 6: TopBar em Erro

```
┌─────────────────────────────────────────────────────────────┐
│ ✗ DISCONNECTED [Reconnect?]       [⚙ Connection]             │
│                                                               │
│ Vermelho = problema! Usuário sabe que precisa conectar       │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│ (Tela desabilitada ou mostra último estado válido?)          │
│                                                               │
└─────────────────────────────────────────────────────────────┘

Vs. ANTES:
- ❌ Apenas "DISCONNECTED" (sem contexto)
- ✅ "✗ DISCONNECTED" em vermelho + dica "Reconnect?"
```

---

## Transições de Estado (State Machine)

```
                          ┌─────────────────────┐
                          │    APPLICATION      │
                          │    STARTUP          │
                          └──────────┬──────────┘
                                     │
                    User click "⚙ Connection"
                                     │
                                     ▼
                          ┌─────────────────────┐
                    ┌────▶│ MODAL: PORT SELECT  │
                    │     └─────────┬───────────┘
                    │               │
                    │     User selects port + mode
                    │               │
                    │               ▼
   (Cancel)         │     ┌─────────────────────┐
   [Cancel]◀────────┤────▶│ STATE: CONNECTING   │
                    │     │ (Init handshake)    │
                    │     └─────────┬───────────┘
                    │               │
                    │         Init success/fail
                    │               │
                    │      ┌────────┴────────┐
                    │      ▼                 ▼
                    │  ✓ CONNECTED      ✗ ERROR
                    │  TopBar updates   Show error message
                    │  Show ECU info    Allow retry
                    │
              Reconnect
              (Click ⚙ again)

Estados possíveis em TopBar:
  1. ✓ ECU: <MODEL> | <LATENCY>ms  (real, OK)
  2. ▯ SIMULATION                  (teste, OK)
  3. ✗ DISCONNECTED                (erro)
  4. ⏳ CONNECTING...              (aguardando)
```

---

## Matriz de Ações Usuário

| Ação | Antes | Depois |
|------|-------|--------|
| **App inicia** | TopBar: "DISCONNECTED"<br/>Pulsing "MODO SIMULAÇÃO" | TopBar: "⚙ Connection"<br/>Modal auto-abre (1ª vez) |
| **Mudar porta** | ❌ Impossível (hardcoded) | ✅ Click "⚙ Connection" → porta selecionável |
| **Alternar simulação** | ❌ Impossível em runtime | ✅ Click "⚙ Connection" → radio button |
| **Ver qual ECU** | ❌ Não mostra | ✅ TopBar mostra "IAW 6E.75" |
| **Reconectar** | Restart app | Click "⚙ Connection" → "Reconnect?" |
| **Entender erro** | Só vermelho/verde | ✅ Modal mostra "Timeout init response" |

---

## Fluxo de Inicialização Completo

### Cenário: Primeira execução

```
1. App inicia
   ↓
2. TopBar: "⚙ Connection" (ativo)
   Modal aparece (ou dica: "Click ⚙ to connect")
   ↓
3. Usuário:
   - Seleciona porta (COM3)
   - Escolhe "Real ECU"
   - Click "Connect"
   ↓
4. App em estado CONNECTING (5s timeout)
   Modal: "⏳ Waiting for ECU response..."
   ↓
5a. ✓ SUCCESS
   - Init packets chegam (0xF6)
   - Modal mostra: "✓ Model: IAW 6E.75"
   - TopBar atualiza: "✓ ECU: IAW6E.75 | 15.3ms"
   - Modal fecha, app normal

5b. ✗ TIMEOUT
   - Modal mostra erro em vermelho
   - Usuário pode:
     a) Tentar outra porta
     b) Verificar cabo
     c) Usar Simulação (teste)

6. App pronto
   Dados fluem em Live, Graph, Dashboard, etc.
```

---

## Acessibilidade & Feedback

### Cores (Deficiência Visual)

```
✓ OK      = Verde (#00FF00)  + símbolo ✓
✗ CRÍTICO = Vermelho (#FF0000) + símbolo ✗
⚠ AVISO   = Amarelo (#FFFF00) + símbolo ⚠
▯ INFO    = Azul (#0080FF)   + símbolo ▯
```

### Tooltips

```
[⚙ Connection]  Hover → "Select port and connection mode"
[▯ SIMULATION]  Hover → "Running with simulated data"
[✓ CONNECTED]   Hover → "ECU ready, latency 15.3ms"
```

### Fontes (Legibilidade)

```
TopBar:        Monospace 12pt (monitorar latência, números)
Modal Title:   Sans 14pt bold
Port List:     Monospace 11pt (nomes de porta)
Buttons:       Sans 12pt (ações principais)
Error Message: Sans 11pt, vermelho (atenção)
```

---

## Responsividade (Touch/Mouse)

```
                    Desktop         Touch Device
Connect Button      Click           Tap (50x50px min)
Port Selection      Click item      Tap (50x50px min)
Mode Toggle         Radio button    Tap (50x50px min)
Modal Size          500x500         Full-screen or 600x600

P10 Consideration:
- Buttons ≥ 40pt altura
- Espaçamento ≥ 10pt entre itens
- Sem hover-only info (mobile-first)
```

---

## Exemplo: Integração com GraphScreen

```
TopBar: ✓ ECU: IAW6E.75 | 15.3ms  [⚙ Connection]
                                      └─ Modal aqui
│
├─ GraphScreen renderiza
│  └─ Dados do ECU (real)
│
└─ Usuário pode:
   - View sensores (Live, Graph, Dashboard)
   - Trocar modo: click ⚙ → Simulação
   - Reconectar: click ⚙ → port diferente

Transição suave (sem restart, sem perda de UI state)
```

---

## Performance Considerações

```
Modal Render (cada frame):
  - Port list: O(n) onde n ≤ 20 (bounded)
  - ImGui::Render: <1ms típico
  - Sem alocação dinâmica em render path (NASA P10)

Backend:
  - ECU info: atualizado 1x no init (em WorkerThread)
  - Callback síncrono (rápido)
  - TopBar refresh: 60fps (normal)
```

---

**Mockups preparados para apresentação/implementação.**
