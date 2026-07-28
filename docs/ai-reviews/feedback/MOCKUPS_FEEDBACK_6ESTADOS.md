# Feedback Mockups: 6 Estados Visuais

## 🎨 Estado 1: IDLE (Inicial - Sem Conexão)

```
┌─────────────────────────────────────────────────────┐
│  9:41                                           🔋  │  ← TopBar normal
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│   ⚙  CONNECTION                                   │  ← Botão cinza 50%
│                                                     │  opacidade (inativo)
│                                                     │
│                    [  LIVE SCREEN  ]               │
│                                                     │
│                                                     │
├─────────────────────────────────────────────────────┤
│  [LIVE]  [GRAPH]  [DASH]  [DTC]  [LOGS]           │  ← BottomNav
└─────────────────────────────────────────────────────┘

Feedback:
  Visual:  Sem feedback
  Audio:   (Silêncio)
  Haptic:  (Nenhum)
```

---

## 🎨 Estado 2: CONNECTING (Tentativa de Conexão)

```
┌─────────────────────────────────────────────────────┐
│  9:41                                           🔋  │  ← Hora normal
├─────────────────────────────────────────────────────┤
│                                                     │
│   ◐ CONNECTING...            (4.2s)               │  ← Spinner girando
│                                                     │  + Timeout counter
│                                                     │
│       ╱                                             │
│      ╱  (spinner rotacionando a ~6 FPS)            │
│     ╱                                               │
│                                                     │
│   Clique para cancelar conectar                   │
│                                                     │
├─────────────────────────────────────────────────────┤
│  [LIVE]  [GRAPH]  [DASH]  [DTC]  [LOGS]           │
└─────────────────────────────────────────────────────┘

Feedback:
  Visual:
    • Spinner: 8 segmentos girando (1 revolução/s)
    • Cada segmento com fade (posteriores mais transparentes)
    • Contador: 4.2s restante (fade de vermelho em T>5s)
    • Cor: Azul 50% (neutro)

  Audio:
    • (Silêncio durante conexão)
    • Opcional: Tone baixo contínuo (200Hz) fadendo (não recomendado)

  Haptic:
    • Vibração 50ms a cada 1 segundo
    • Padrão: ON (50ms) + WAIT (950ms) + repeat
    • Indica "estou processando" ao usuário tátil
```

---

## 🎨 Estado 3: SUCCESS (Conectado + ECU Detectado)

```
┌─────────────────────────────────────────────────────┐
│  9:41                                           🔋  │
├─────────────────────────────────────────────────────┤
│                                                     │
│   ✓ ECU: BOSCH MA17 | FW: 2000148                │  ← Com GLOW
│   ◾◾◾◾◾  (5/5 Signal Bars - verde)               │
│   Latência: 15.3ms                                │
│                                                     │
│   P/N: 0261 203 388-AN09                         │  ← Slide-in animation
│   Hardware: I | Variante: AN09                    │
│                                                     │
│                    [  LIVE SCREEN  ]               │
│                                                     │
├─────────────────────────────────────────────────────┤
│  [LIVE]  [GRAPH]  [DASH]  [DTC]  [LOGS]           │
└─────────────────────────────────────────────────────┘

Feedback Timeline:

T+0.0s:
  ✓ Flash verde (0-0.3s fade-in)
  ✓ Background: semi-transparent verde 0.2 alpha
  ✓ Audio: BEEP 800Hz começa
  ✓ Haptic: Vibração dupla inicia

T+0.1s:
  ✓ Glow effect inicia ao redor de "ECU: BOSCH MA17"

T+0.2s:
  ✓ "ECU: BOSCH MA17" slide-in da esquerda (0-50px)
  ✓ Text fade-in (cinza 50% → verde)

T+0.3s:
  ✓ Flash desaparece (alpha → 0)
  ✓ Audio: BEEP termina
  ✓ Haptic: Vibração dupla termina
  ✓ Signal bars aparecem animadas (1 → 5 bars)

T+0.5s:
  ✓ Tudo estável em estado final
  ✓ Glow continua pulsando suavemente (0.5Hz)

Feedback Detalhado:
  Visual:
    • Flash verde (full-screen, 0.5 alpha, fade-out 0-0.3s)
    • Glow: 3 camadas concêntricas, pulsing 0.5Hz
    • ECU model: Slide-in da esquerda, fade-in cor
    • Signal bars: Barras animadas 1→5 com cor verde
    • Cores: Verde OK (#00FF00)

  Audio:
    • Beep 800Hz por 150ms (satisfatório, "ding" alto)
    • Opcional: 2-tone (800Hz 100ms + 1000Hz 100ms)

  Haptic:
    • Vibração dupla: 50ms ON + 50ms WAIT + 50ms ON
    • Padrão claro: "sucesso confirmado"
```

---

## 🎨 Estado 4: SIMULATION MODE (Modo Simulação)

```
┌─────────────────────────────────────────────────────┐
│  9:41                                           🔋  │
├─────────────────────────────────────────────────────┤
│                                                     │
│   ▯ SIMULATION MODE                               │  ← Pulsing LENTO (1Hz)
│   ◾◾◾◾◽  (4/5 Signal Bars - amarelo)            │  ← Desaturado
│   Latência: 18.5ms (simulado)                     │
│                                                     │
│   P/N: Dados fictícios                            │
│   Modo: Dados Simulados (não é real)             │
│                                                     │
│                    [  LIVE SCREEN  ]               │
│                    (desaturado 50%)                │  ← Tudo mais pálido
│                                                     │
├─────────────────────────────────────────────────────┤
│  [LIVE]  [GRAPH]  [DASH]  [DTC]  [LOGS]           │
└─────────────────────────────────────────────────────┘

Transição: REAL → SIMULAÇÃO

T+0.0s:
  ✓ "▯ SIMULATION MODE" label aparece
  ✓ Pulsing muda de (nenhum) → 1Hz
  ✓ Alpha oscila: 0.8 → 1.0 (menos agressivo)

T+0.1s:
  ✓ Toda tela desaturada (saturação → 50%)
  ✓ Signal bars cor muda: verde → amarelo
  ✓ Audio: Beep 500Hz 100ms

T+0.2s:
  ✓ Haptic: Vibração dupla (feedback da mudança)

T+0.3s:
  ✓ Estável em "modo simulação"

Feedback Detalhado:
  Visual:
    • Label "▯ SIMULATION" pulsing 1Hz (lento, calmo)
    • Alpha oscila: [0.8, 1.0] (menos urgente que 2Hz)
    • Tela desaturada: Colors.desaturate(colors, 0.5f)
    • Cor: Amarelo/Laranja (#FFB84D)
    • Background: Desaturado 50% para indicar "teste"

  Audio:
    • Beep 500Hz por 100ms (tom médio, neutro)
    • Indica "mudança de modo"

  Haptic:
    • Vibração dupla (confirmação da mudança)
```

---

## 🎨 Estado 5: ERROR (Falha de Conexão)

```
┌─────────────────────────────────────────────────────┐
│  9:41                                           🔋  │
├─────────────────────────────────────────────────────┤
│                                                     │
│   ✗ DISCONNECTED                                  │  ← Pulsing VERMELHO (1Hz)
│   ◾◽◽◽◽  (1/5 Signal Bars - vermelho)            │
│                                                     │
│   Tentando reconectar... (1/3)                    │
│   Próxima tentativa em 5s...                      │
│                                                     │
│   [Tentar agora]  [Modo simulação]               │  ← Botões de ação
│                                                     │
│                    [  LIVE SCREEN ]               │
│                    (com alpha reduzido)            │  ← Semitransparente
│                                                     │
├─────────────────────────────────────────────────────┤
│  [LIVE]  [GRAPH]  [DASH]  [DTC]  [LOGS]           │
└─────────────────────────────────────────────────────┘

Feedback Timeline:

T+0.0s:
  ✓ Flash vermelho (0-0.3s fade-in)
  ✓ Background: semi-transparent vermelho 0.2 alpha
  ✓ Audio: BEEP 300Hz começa
  ✓ Haptic: Vibração LONGA (200ms) inicia

T+0.2s:
  ✓ "✗ DISCONNECTED" label aparece
  ✓ Pulsing vermelho 1Hz inicia (não é urgente como 2Hz)

T+0.3s:
  ✓ Flash desaparece
  ✓ Audio: BEEP termina
  ✓ Haptic: Vibração termina

T+1.0s → T+5.0s:
  ✓ Contador regressivo: "Próxima em 5s... 4s... 3s..."
  ✓ Pulsing continua (1Hz, lembre ao usuário)

T+5.0s:
  ✓ Tentativa automática de reconexão
  ✓ Label muda: "(1/3)" → "(2/3)"
  ✓ Se sucesso: volta ao Estado 3 (SUCCESS)
  ✓ Se falha novamente: reinicia contador

T+15.0s (após 3 falhas):
  ✓ "Falha permanente - [Tentar agora]"
  ✓ Botão destacado

Feedback Detalhado:
  Visual:
    • Flash vermelho (full-screen, 0.5 alpha)
    • Badge: "✗ DISCONNECTED" pulsing 1Hz
    • Alpha oscila: [0.6, 1.0]
    • Cor: Vermelho (#FF0000)
    • Signal bars: Desaparecem ou mostram 1 bar
    • Contador: "Próxima em 5s" com timer

  Audio:
    • Beep 300Hz por 300ms (som "ruim", triste)
    • Opcional: Tone triste (300Hz → 200Hz descendo)

  Haptic:
    • Vibração longa 200ms (urgente, mas sustentado)
    • Padrão diferente do sucesso (identificável)

  Comportamento:
    • Reconexão automática (3 tentativas)
    • Botões: "Tentar agora", "Modo simulação"
    • Não bloqueia UI (usuário pode usar simulação)
```

---

## 🎨 Estado 6: MODE SWITCH (Transição Real ↔ Simulação)

### Transição A: REAL → SIMULAÇÃO

```
┌─────────────────────────────────────────────────────┐
│  ✓ ECU: BOSCH MA17 (REAL, conectado)              │  ← Antes
└─────────────────────────────────────────────────────┘

         [Usuário clica "Usar Simulação"]

T+0.0s:  Desaturação inicia (cores → cinza 50%)
T+0.1s:  "▯ SIMULATION" label aparece com slide-in
T+0.2s:  Pulsing muda de (nenhum) → 1Hz
T+0.3s:  Audio beep 500Hz
T+0.4s:  Haptic vibração dupla

┌─────────────────────────────────────────────────────┐
│  ▯ SIMULATION (MODO TESTE, desaturado)             │  ← Depois
└─────────────────────────────────────────────────────┘

Feedback:
  Visual:  Transição suave: cores → cinza 50% (0.2s)
           Label slide-in da esquerda
           Pulsing muda intensidade 2Hz → 1Hz

  Audio:   Beep 500Hz 100ms (tom neutro, médio)

  Haptic:  Vibração dupla (50ms + pausa + 50ms)
```

### Transição B: SIMULAÇÃO → REAL

```
┌─────────────────────────────────────────────────────┐
│  ▯ SIMULATION (desaturado)                         │  ← Antes
└─────────────────────────────────────────────────────┘

    [Usuário reconecta a ECU real]

T+0.0s:  Flash verde (confirmação)
T+0.1s:  Saturação aumenta (cinza → cores normais)
T+0.2s:  "✓ ECU: BOSCH MA17" aparece com glow
T+0.3s:  Pulsing desliga (estado normal)
T+0.4s:  Audio beep 800Hz
T+0.5s:  Haptic vibração dupla

┌─────────────────────────────────────────────────────┐
│  ✓ ECU: BOSCH MA17 | FW: 2000148 (REAL, com glow) │  ← Depois
└─────────────────────────────────────────────────────┘

Feedback:
  Visual:  Flash verde (0.3s)
           Saturação: cinza 50% → cores normais (0.2s)
           Glow effect ao redor de ECU model
           Pulsing desliga (estado normal)

  Audio:   Beep 800Hz 150ms (satisfação)

  Haptic:  Vibração longa 150ms (confirmação)
```

---

## 📊 Tabela de Feedback por Estado

| Estado | Visual | Audio | Haptic | Observação |
|--------|--------|-------|--------|------------|
| IDLE | (nenhum) | (silêncio) | (nenhum) | Estado parado |
| CONNECTING | Spinner 1Hz | (silêncio) | Pulso 1s | "Estou fazendo algo" |
| SUCCESS | Flash verde + Glow | Beep 800Hz | Vibração dupla | "Sucesso!" |
| SIMULATION | Pulsing 1Hz | Beep 500Hz | Vibração dupla | "Modo teste ativo" |
| ERROR | Flash vermelho + Pulse 1Hz | Beep 300Hz | Vibração longa | "Falha, tentando..." |
| MODE_SWITCH | Desaturação/Saturação | Beep 500/800Hz | Vibração dupla | "Mudança de modo" |

---

## 🎬 Sequências de Animação (Frame by Frame)

### SUCCESS Sequence (0-0.5s)

```
Frame 0 (T=0.0s):  Flash verde aparece (alpha=0.5)
                   Beep 800Hz começa
                   Haptic inicia

Frame 3 (T=0.1s):  Flash ainda visível (alpha=0.5)
                   Glow effect começando ao redor de ECU
                   Beep em progresso

Frame 6 (T=0.2s):  Flash começando a desaparecer (alpha=0.3)
                   ECU text slide-in (x: 0→50px)
                   Glow pulsa suavemente

Frame 9 (T=0.3s):  Flash desaparece (alpha=0)
                   Beep termina
                   Haptic termina
                   ECU text em posição final

Frame 15 (T=0.5s): Tudo estável
                   Glow continua pulsando 0.5Hz
                   Signal bars visíveis (5/5)
```

---

## 🎨 Cores Utilizadas

```cpp
// Estados e cores correspondentes
enum class ConnectionState {
    IDLE,        // Cinza 50% (#808080)
    CONNECTING,  // Azul neutro (#4080FF)
    SUCCESS,     // Verde (#00FF00)
    SIMULATION,  // Amarelo/Laranja (#FFB84D)
    ERROR,       // Vermelho (#FF0000)
};

// Animações de transição
Fade: 0.2-0.3 segundos
Slide: 0.2-0.3 segundos (50px horizontal)
Pulse: 0.5-2.0 Hz (frequência)
  - 2Hz: Urgente (ERROR antes reconecção)
  - 1Hz: Normal (SIMULATION, ERROR reconectando)
  - 0.5Hz: Calmo (GLOW no SUCCESS)
```

---

## ✅ Checklist Visual

- [ ] Spinner de 8 segmentos (rotação suave)
- [ ] Glow effect (3 camadas concêntricas)
- [ ] Flash fade-in/out (0.3s)
- [ ] Pulsing com frequências diferentes (0.5, 1, 2 Hz)
- [ ] Cores claramente distintas (verde/amarelo/vermelho)
- [ ] Texto com slide-in animation
- [ ] Signal bars animadas (1→5)
- [ ] Desaturação/Saturação smooth (0.2s)
- [ ] Tudo a 60 FPS sem lag

---

## 📱 Responsividade (Touch)

```
Estado CONNECTING:
  ✓ Clique em qualquer lugar = cancel

Estado ERROR:
  ✓ Botão [Tentar agora] = reconectar imediato
  ✓ Botão [Modo simulação] = switch para simulação

Estado SUCCESS:
  ✓ Clique em "⚙ Connection" = abrir modal (mudar porta/modo)
  ✓ Clique em ECU info = mostrar detalhes completos
```

---

## 🎯 Resultado Final

Com esses 6 estados e feedback multi-sensorial, o usuário **nunca mais** vai:
- ❌ Não saber se está conectando
- ❌ Pensar que simulação é erro
- ❌ Tocar múltiplas vezes o botão
- ❌ Estar confuso sobre qual modo está ativo

✅ **UX clara, profissional e intuitiva**

