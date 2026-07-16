# Reverse Engineering Tools

Este diretorio agrupa scripts auxiliares de analise e validacao usados fora do build principal.

Escopo: estes utilitarios foram preparados para a revisao de hardware da ECU Bosch Monomotronic MA1.7 usada no Fiat Tipo 1.6ie. Eles refletem descobertas, mapeamentos e hipoteses de engenharia reversa especificos desse alvo.

## Arquivos

- `sensor_database.py`
  - Base Python de metadados de sensores RAM/XRAM da ECU.
  - Define nomes, unidades e funcoes de conversao para leituras cruas.
  - Serve como dependencia do decoder F4 e como utilitario de consulta rapida.

- `kline_frame_f4_decoder.py`
  - Decoder de payload F4 (10 bytes) da sessao K-Line.
  - Seleciona a tabela correta com base em `RAM_B3` e converte cada posicao usando `sensor_database.py`.
  - Pode ser executado diretamente para rodar casos de teste incluidos no proprio script.

## Uso

Executar o decoder:

```bash
python3 tools/reverse-engineering/kline_frame_f4_decoder.py
```

Modo interativo:

```bash
python3 tools/reverse-engineering/kline_frame_f4_decoder.py --interactive
```

Executar o teste rapido do banco de sensores:

```bash
python3 tools/reverse-engineering/sensor_database.py
```

## Relacao com a documentacao

- Base tecnica: `docs/reference/protocol/kline_capabilities_tables_analysis.md`
- Contexto adicional: `docs/reference/protocol/ANALISE_INIT_PACKETS_STRING.md`

Estes scripts sao ferramentas de apoio para pesquisa e validacao da revisao Monomotronic MA1.7 do Fiat Tipo 1.6ie. Eles nao fazem parte do binario principal nem sao a fonte canonica das formulas usadas no codigo C++.
