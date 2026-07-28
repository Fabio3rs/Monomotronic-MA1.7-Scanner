# Documentacao

Este diretorio reune material de apoio que ainda pode ser util para o projeto.

Importante: boa parte destes documentos foi gerada total ou parcialmente com ajuda de IA durante sessoes de analise, review e planejamento. Eles devem ser tratados como material auxiliar, nao como fonte canonica do comportamento atual do sistema.

Regra pratica:
- O codigo-fonte atual e os testes continuam sendo a fonte primaria de verdade.
- Documentos em `reference/` tendem a conter conhecimento tecnico mais estavel.
- Documentos em `ai-reviews/` sao analises e propostas que ainda podem ser uteis, mas precisam ser validadas contra o estado atual do repositorio antes de orientar implementacoes.

## Estrutura

### `reference/`

Material tecnico que permanece relevante mesmo com a evolucao do projeto.

- `reference/protocol/`
  - `ANALISE_INIT_PACKETS_STRING.md`: observacoes sobre o banner/string de identificacao da ECU.
  - `kline_capabilities_tables_analysis.md`: analise detalhada das tabelas de capacidades K-Line no firmware.
- `reference/esp32/`
  - `ESP32_SESSION_HANDOFF.md`: handoff tecnico de uma rodada relevante da versao ESP32. Historico util, mas com chance maior de envelhecimento.

### `ai-reviews/`

Analises, reviews e guias gerados com ajuda de IA que ainda podem servir como base de trabalho.

- `ai-reviews/ui/`
  - `REVIEW_UI_ARCHITECTURE.md`
  - `ANALISE_SCREENSHOTS_DETALHADA.md`
  - `RECOMENDACOES_AJUSTES_UI.md`
  - `CODIGO_FIXES_PRONTOS.md`
  - `REFACTORING_PRACTICAL_GUIDE.md`
  - `CHECKLIST_CODE_REVIEW.md`
- `ai-reviews/ux/`
  - `REVIEW_UX_CONNECTION.md`
  - `GUIA_IMPLEMENTACAO_CONNECTION.md`
  - `MOCKUPS_UX_CONNECTION.md`
- `ai-reviews/feedback/`
  - `GUIA_FEEDBACK_PRATICO.md`
  - `MOCKUPS_FEEDBACK_6ESTADOS.md`
  - `REVIEW_FEEDBACK_COMPLETO.md`
- `ai-reviews/datalogging/`
  - `ANALISE_DATALOGGING.md`
  - `GUIA_DATALOGGING_IMPLEMENTACAO.md`
  - `GAPS_DATALOGGING_PLANO.md`
  - `NASA_P10_DATALOGGING.md`
- `ai-reviews/sensors/`
  - `REVIEW_SENSOR_DECODERS.md`
  - `ANALISE_APROFUNDADA_SENSOR_DECODERS.md`
  - `TESTING_SENSOR_DECODERS.md`
  - `IMPLEMENTACAO_MELHORIAS_SENSOR_DECODERS.md`
  - `SENSOR_DECODERS_SUMMARY.txt`

### `archive/`

Material gerado por IA com valor principalmente historico ou de navegacao:

- indices
- quick starts
- resumos executivos
- pacotes de entrega de uma sessao especifica
- duplicatas de documentos mais completos

Esse conteudo foi preservado para consulta, mas em geral nao deve orientar decisoes tecnicas sem uma validacao forte no codigo atual.

## Como usar este material

- Use `reference/` para contexto de protocolo, reverse engineering e historico tecnico.
- Use `ai-reviews/` como backlog comentado, apoio de investigacao e ponto de partida para refactorings ou testes.
- Use `archive/` apenas quando precisar recuperar contexto de uma sessao antiga ou localizar como uma analise foi empacotada.
- Antes de implementar qualquer recomendacao, confirme no codigo se ela ainda faz sentido e se ja nao foi parcialmente resolvida.

## Fora de `docs/`

Os arquivos `AGENTS.md` e `CLAUDE.md` permanecem na raiz porque sao arquivos especiais usados por ferramentas e agentes de desenvolvimento. Eles nao entram nesta organizacao por serem instrucionais, nao documentacao funcional do projeto.

Outros ativos relacionados a esta documentacao foram organizados fora de `docs/`:

- `tools/reverse-engineering/`: scripts Python auxiliares para decoder K-Line e consulta de sensores, focados na revisao de hardware Monomotronic MA1.7 do Fiat Tipo 1.6ie
- `data/sensor-calibration/`: CSVs de calibracao e comparacao de curvas de sensores
