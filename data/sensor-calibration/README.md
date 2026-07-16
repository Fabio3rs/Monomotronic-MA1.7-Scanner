# Sensor Calibration Data

Este diretorio reune tabelas CSV usadas em estudos de calibracao e comparacao de sensores de temperatura.

## Arquivos

- `AIR_refit_vs_datasheet_Rpu2661.csv`
  - Compara a curva de temperatura do sensor de ar entre o datasheet e um refit cubico assumindo `Rpu = 2661 ohm`.

- `WATER_refit_vs_datasheet_Rpu3004.csv`
  - Compara a curva de temperatura do sensor de agua entre o datasheet e um refit cubico assumindo `Rpu = 3004 ohm`.

- `manual_points_comparison.csv`
  - Tabela pequena de pontos medidos/manuals comparando ADC, resistencia NTC, tensao de saida e erro das formulas.

- `fitted_polynomials_vs_manual_and_bosch.csv`
  - Resumo curto dos coeficientes e correcoes comparados contra LUT Bosch e pontos manuais.

## Uso esperado

- validar ou revisar formulas de conversao de temperatura
- comparar erro entre curva atual, LUT Bosch e ajustes experimentais
- registrar evidencias de engenharia reversa/calibracao

## Observacoes

- Estes CSVs sao material de apoio e nao sao consumidos automaticamente pelo build.
- Se novas iteracoes de calibracao surgirem, prefira manter aqui com nomes explicitos e um README atualizado.
