# 📑 GUIA DE LEITURA: Qual Documento Ler?

**Encontre o documento certo em 30 segundos.**

---

## 🎯 Escolha Seu Cenário

### "Sou GERENTE e quero entender o problema"
```
Tempo disponível: 5 minutos
├─ Leia: QUICK_START_UX.md
├─ Próximo: SUMARIO_UX_CONNECTION.md
└─ Decide: Aprovar implementação? ✓ SIM
```

### "Sou DEV e vou IMPLEMENTAR"
```
Tempo disponível: 3-5 dias
├─ Dia 0 (30 min): QUICK_START + SUMARIO + REVIEW
├─ Dia 1-3 (3 dias): GUIA_IMPLEMENTACAO_CONNECTION.md
│                    (passo 1-6, copy-paste)
├─ Dia 3 (final): CHECKLIST_CODE_REVIEW.md (self-review)
└─ Resultado: PR pronto para merge
```

### "Sou CODE REVIEWER"
```
Tempo: Para cada PR
├─ Setup: Tenha CHECKLIST_CODE_REVIEW.md aberto
├─ Contexto: REVIEW_UX_CONNECTION.md (seção recomendação)
├─ Dúvida UX: MOCKUPS_UX_CONNECTION.md
└─ Merge: Se tudo ok (Definition of DONE)
```

### "Sou QA/TESTER"
```
Tempo: 1-2 horas
├─ Leia: MOCKUPS_UX_CONNECTION.md (20 min)
├─ Crie: 6 test cases (1 por estado)
├─ Teste: Modal, porta, simulação, ECU, erro
└─ Sign-off: ✓ Pronto
```

### "Estou PERDIDO e quero orientação"
```
Tempo: 5 minutos
├─ Leia: VISAO_GERAL_UX.md (este arquivo!)
├─ Consulte: INDICE_DOCUMENTACAO_UX.md
└─ Escolha seu cenário acima
```

---

## 📊 Matriz de Documentos

| Documento | Tempo | Público | Quando ler | Próximo |
|-----------|-------|---------|-----------|---------|
| **QUICK_START_UX.md** | 2 min | Todos | Primeiro | SUMARIO |
| **SUMARIO_UX_CONNECTION.md** | 5 min | Gerentes, Devs | Segundo | REVIEW ou GUIA |
| **REVIEW_UX_CONNECTION.md** | 30 min | Devs, Arquitetos | Antes de codificar | GUIA |
| **GUIA_IMPLEMENTACAO_CONNECTION.md** | 60 min | Devs (enquanto codifica) | Dia 1-3 | CHECKLIST |
| **MOCKUPS_UX_CONNECTION.md** | 20 min | QA, Designers | Para testes/design | Test cases |
| **CHECKLIST_CODE_REVIEW.md** | 15 min | Reviewers | Ao revisar PR | Merge |
| **INDICE_DOCUMENTACAO_UX.md** | 10 min | Navegação | Se perdido | Docs específicos |
| **VISAO_GERAL_UX.md** | 5 min | Quick ref | Agora | Seu documento |

---

## 🚀 Fluxos por Tempo Disponível

### "Tenho 5 minutos"
```
Leia → QUICK_START_UX.md
Resultado → Entende os 3 problemas
```

### "Tenho 10 minutos"
```
QUICK_START_UX.md (2 min)
+ SUMARIO_UX_CONNECTION.md (5 min)
+ VISAO_GERAL_UX.md (3 min)
Resultado → Visão completa + timeline
```

### "Tenho 1 hora (vou implementar)"
```
QUICK_START_UX.md (2 min)
+ SUMARIO_UX_CONNECTION.md (5 min)
+ REVIEW_UX_CONNECTION.md (seção Recomendação, 20 min)
+ GUIA_IMPLEMENTACAO_CONNECTION.md (start, 15 min)
+ CHECKLIST_CODE_REVIEW.md (overview, 10 min)
+ MOCKUPS_UX_CONNECTION.md (estados, 10 min)
Resultado → Pronto para começar código
```

### "Tenho 3-5 dias (implementar + review)"
```
Dia 1 (leitura, 2h):
  QUICK_START → SUMARIO → REVIEW → GUIA (start)

Dia 2-4 (implementação, 4-6h):
  Siga GUIA passo 1-6
  Compile a cada passo
  Self-review com CHECKLIST

Dia 5 (PR):
  Submeter + Code review com CHECKLIST
  QA testa com MOCKUPS
  Merge ✓
```

---

## 🎓 Por Nível de Conhecimento

### Iniciante (primeira vez vendo este projeto)
```
Recomendado:
1. QUICK_START_UX.md (2 min)
2. SUMARIO_UX_CONNECTION.md (5 min)
3. VISAO_GERAL_UX.md (este arquivo, 5 min)
4. MOCKUPS_UX_CONNECTION.md (20 min, visuais)
└─ Totalizando: 30 min, entenderá os 3 problemas

Após implementação:
- REVIEW_UX_CONNECTION.md (se vai revisar código)
- GUIA_IMPLEMENTACAO_CONNECTION.md (se vai codificar)
```

### Intermediário (conhece codebase, vai implementar)
```
Recomendado:
1. SUMARIO_UX_CONNECTION.md (5 min, contexto)
2. REVIEW_UX_CONNECTION.md (seção "Recomendação", 20 min)
3. GUIA_IMPLEMENTACAO_CONNECTION.md (passo 1-6, 60 min)
4. CHECKLIST_CODE_REVIEW.md (durante PR, 15 min)
└─ Totalizando: 2h, pronto para codificar

Referência:
- MOCKUPS_UX_CONNECTION.md (se dúvida sobre UX)
```

### Experiente (vai revisar/arquitetar)
```
Recomendado:
1. REVIEW_UX_CONNECTION.md (inteiro, 30 min)
2. CHECKLIST_CODE_REVIEW.md (referência, 10 min)
3. GUIA_IMPLEMENTACAO_CONNECTION.md (verificar skeleton, 20 min)
└─ Totalizando: 1h, review-ready

Referência:
- MOCKUPS_UX_CONNECTION.md (aspectos UX)
- SUMARIO_UX_CONNECTION.md (timeline + riscos)
```

---

## 📍 Encontre Resposta Rápida

### "Qual é o problema 1?"
→ QUICK_START_UX.md, seção "Os 3 Problemas" (20 seg)

### "Qual é a solução?"
→ SUMARIO_UX_CONNECTION.md, seção "Solução (5 pts)" (5 min)

### "Como implementar?"
→ GUIA_IMPLEMENTACAO_CONNECTION.md, Passo 1 (30 seg)

### "Qual é o estado da simulação?"
→ MOCKUPS_UX_CONNECTION.md, Estado 3 (2 min)

### "NASA P10 compliance?"
→ CHECKLIST_CODE_REVIEW.md, seção NASA P10 (10 min)

### "Quanto tempo?"
→ SUMARIO_UX_CONNECTION.md, "Implementação 3 dias" (1 min)

### "Code review checklist?"
→ CHECKLIST_CODE_REVIEW.md, "Final Checklist" (30 seg)

### "Quais testes rodar?"
→ GUIA_IMPLEMENTACAO_CONNECTION.md, seção "Testes (GoogleTest)" (5 min)

### "Como integrar em main.cpp?"
→ GUIA_IMPLEMENTACAO_CONNECTION.md, "Passo 6: Integração" (10 min)

### "Estados visuais da UI?"
→ MOCKUPS_UX_CONNECTION.md, "6 Estados diferentes" (15 min)

---

## 🔗 Cross-references Úteis

### Se leu SUMARIO e quer detalhes
```
"ConnectionModal com seleção de porta"
├─ Detalhes: REVIEW_UX_CONNECTION.md, Problema 1
├─ Código: GUIA_IMPLEMENTACAO_CONNECTION.md, Passo 2-3
└─ Visual: MOCKUPS_UX_CONNECTION.md, Estado 2
```

### Se leu REVIEW e quer implementar
```
"Como exatamente codificar?"
├─ Step-by-step: GUIA_IMPLEMENTACAO_CONNECTION.md
├─ Skeleton: GUIA, seção "Passo 3: ConnectionModal.cpp"
└─ Testes: GUIA, seção "Testes (GoogleTest)"
```

### Se vai fazer code review
```
"Que checklist usar?"
├─ Checklist: CHECKLIST_CODE_REVIEW.md
├─ Arquitetura: REVIEW_UX_CONNECTION.md
├─ States: MOCKUPS_UX_CONNECTION.md
└─ Timeline: SUMARIO_UX_CONNECTION.md
```

### Se quer validar UX
```
"Que estados testar?"
├─ 6 Estados: MOCKUPS_UX_CONNECTION.md (todo o doc)
├─ Fluxos: MOCKUPS, seção "Fluxo de Inicialização"
└─ Checklist: MOCKUPS, seção "Resultado Final"
```

---

## 📋 Ordem Recomendada (por Papel)

### 👔 GERENTE / STAKEHOLDER
```
1. VISAO_GERAL_UX.md ← VOCÊ ESTÁ AQUI
2. QUICK_START_UX.md
3. SUMARIO_UX_CONNECTION.md
└─ Tempo: 12 min, decide aprovação
```

### 👨‍💻 DEVELOPER (Implementador)
```
1. QUICK_START_UX.md
2. SUMARIO_UX_CONNECTION.md
3. REVIEW_UX_CONNECTION.md (foco em "Recomendação")
4. GUIA_IMPLEMENTACAO_CONNECTION.md (Dia 1-3)
5. CHECKLIST_CODE_REVIEW.md (self-review)
└─ Tempo: 2-3h inicial, 4-6h código, 1h review
```

### 🔍 CODE REVIEWER
```
1. REVIEW_UX_CONNECTION.md
2. CHECKLIST_CODE_REVIEW.md (abrir ao lado do diff)
3. MOCKUPS_UX_CONNECTION.md (se dúvida)
└─ Tempo: 1h setup, 30 min por PR
```

### 🧪 QA / TESTER
```
1. QUICK_START_UX.md
2. MOCKUPS_UX_CONNECTION.md
3. Criar 6 test cases (1 por estado)
4. Testar contra "Resultado Final" (SUMARIO ou MOCKUPS)
└─ Tempo: 1-2h
```

### 🎨 DESIGNER / UX
```
1. MOCKUPS_UX_CONNECTION.md
2. VISAO_GERAL_UX.md
3. SUMARIO_UX_CONNECTION.md (para contexto)
└─ Tempo: 30 min
```

---

## 🚨 Emergency FAQ

| Pergunta | Resposta Rápida | Documento |
|----------|-----------------|-----------|
| **Qual é o pior problema?** | Sem modal de porta (crítico) | SUMARIO, "3 Problemas" |
| **Quanto código é?** | ~300 linhas (ConnectionModal 250 + mods) | GUIA, "Passo 2-3" |
| **Compila?** | Sim, skeleton testado no GUIA | GUIA, "ConnectionModal.cpp" |
| **Impacto performance?** | Nenhum (modal é O(n), n≤20) | REVIEW, "Performance" |
| **Testado?** | GoogleTest skeleton fornecido | GUIA, "Testes" |
| **Quanto tempo?** | 3 dias (1 dev) se segue GUIA | SUMARIO, "Timeline" |
| **Quebra algo?** | Não, é backward compatible | REVIEW, "Solução" |
| **Qual padrão?** | NASA P10 + C++20/23 | CHECKLIST, "Regras" |

---

## ✅ Sua Próxima Ação

Escolha uma:

### Opção A: "Quero ver overview rápido"
```
→ Leia QUICK_START_UX.md (2 min)
→ Próximo: SUMARIO_UX_CONNECTION.md (5 min)
```

### Opção B: "Vou implementar"
```
→ Leia SUMARIO_UX_CONNECTION.md (5 min)
→ Leia REVIEW_UX_CONNECTION.md (30 min)
→ Abra GUIA_IMPLEMENTACAO_CONNECTION.md
→ Siga Passo 1-6
```

### Opção C: "Vou revisar PR"
```
→ Leia REVIEW_UX_CONNECTION.md (30 min)
→ Abra CHECKLIST_CODE_REVIEW.md (referência)
→ Ao lado do diff, use a checklist
```

### Opção D: "Vou testar"
```
→ Leia MOCKUPS_UX_CONNECTION.md (20 min)
→ Crie 6 test cases
→ Execute contra app
```

---

**Pronto? Comece em QUICK_START_UX.md → 2 minutos → GO!**
