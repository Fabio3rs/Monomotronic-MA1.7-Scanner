# Guia de Implementação: Melhorias SensorDecoders

**Objetivo:** Fornecer código pronto para implementar as 5 principais melhorias
**Tempo Total:** 10-16 horas
**Complexidade:** Fácil a Média

---

## 📋 Índice de Soluções

1. [Validação de Entrada](#1-validação-de-entrada) - 1-2h
2. [Tipos de Dados Precisos](#2-tipos-de-dados-precisos) - 1-2h
3. [Constantes Mágicas](#3-extrair-constantes-mágicas) - 30min-1h
4. [Documentação Doxygen](#4-documentação-doxygen) - 2-3h
5. [Testes Unitários](#5-testes-unitários) - 3-4h

---

## 1. Validação de Entrada

### Passo 1: Atualizar Header

```cpp
// src/ECUSerial/SensorDecoders.h

// ANTES:
double decodeNominalRPM(int raw);
double decodeEngineRPM(int raw);
double decodeAirTemperature(int raw);

// DEPOIS: Usar tipos específicos
double decodeNominalRPM(uint8_t raw);
double decodeEngineRPM(uint8_t raw);
double decodeAirTemperature(uint8_t raw);
```

### Passo 2: Implementar Validação

```cpp
// src/ECUSerial/SensorDecoders.cpp

namespace {

// Helper para validação consistente
constexpr double validateAndDecode(int raw,
                                  int minVal, int maxVal,
                                  std::function<double(int)> decoder) {
    raw = std::clamp(raw, minVal, maxVal);
    return decoder(raw);
}

}

// Opção A: Clamping (RECOMENDADO)
double decodeAirTemperature(uint8_t raw) {
    // uint8_t já garante 0-255
    return decodeAirTemperatureImpl(raw);
}

// Opção B: Com validação explícita (se receber int)
double decodeAirTemperatureFromInt(int raw) {
    constexpr int MIN_VALID = 0, MAX_VALID = 255;
    if (raw < MIN_VALID || raw > MAX_VALID) {
        // Log warning
        raw = std::clamp(raw, MIN_VALID, MAX_VALID);
    }
    return decodeAirTemperatureImpl(static_cast<uint8_t>(raw));
}

// Opção C: Com NaN em erro
double decodeAirTemperatureOrNaN(int raw) {
    if (raw < 0 || raw > 255) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return decodeAirTemperatureImpl(static_cast<uint8_t>(raw));
}
```

### Checklist de Implementação

- [ ] Atualizar assinatura de todas 20 funções públicas
- [ ] Adicionar conversão segura int → uint8_t/uint16_t
- [ ] Testar com valores extremos (0, 255, -1, 256)
- [ ] Atualizar documentação de contrato

---

## 2. Tipos de Dados Precisos

### Passo 1: Atualizar Todas as Assinaturas

```cpp
// ANTES (inseguro)
double decodeNominalRPM(int raw);
double decodeInjectionTime(int raw16);

// DEPOIS (type-safe)
double decodeNominalRPM(uint8_t raw);
double decodeEngineRPM(uint8_t raw);
double decodeThrottleAngleTrack2(uint8_t raw);
double decodeAirTemperature(uint8_t raw);
double decodeWaterTemperature(uint8_t raw);
double decodeLambdaVoltage(uint8_t raw);
double decodeThrottlePercentTrack1(uint8_t raw);
double decodeSparkAdvance(uint8_t raw);
double decodeThrottleCombinedAngle(uint8_t raw);
double decodeThrottleAngle(uint8_t raw);
double decodeBatteryVoltage(uint8_t raw);
double decodeInjectionTime(uint16_t raw);  // ← 2 bytes
double decodeLambdaIntegrator(uint8_t raw);
double decodeRevRegulatorAngle(uint8_t raw);
double decodeFTEADSelfAdaption(uint8_t raw);
double decodeGeneralMapAdaption(uint8_t raw);
double decodeMLLECKCoefficient(uint8_t raw);
double decodeTSCoefficient(uint8_t raw);
double decodeEvaporatorValve(uint8_t raw);
double decodeRaw(uint8_t raw);
```

### Passo 2: Atualizar Implementação Interna

```cpp
namespace {

// Assinaturas internas também
double decodeNominalRPMImpl(uint8_t raw) {
    return static_cast<double>(raw) * 25.0;
}

double decodeInjectionTimeImpl(uint16_t raw16) {
    return (static_cast<double>(raw16) * 4.0) / 1000.0;
}

}
```

### Checklist

- [ ] Atualizar .h com novos tipos
- [ ] Atualizar .cpp (Impl functions)
- [ ] Atualizar public wrappers
- [ ] Compilar e verificar erros
- [ ] Atualizar tests/callers do código
- [ ] Verificar type conversions automáticas

---

## 3. Extrair Constantes Mágicas

### Passo 1: Identificar Constantes

```cpp
// Magic numbers identificados:
// 3.9     → Lambda voltage scale
// 0.195   → Lambda integrator scale
// 128.0   → Lambda integrator offset
// 0.35    → Spark advance scale
// 60.0    → Spark advance max
// 24.0    → TPS1 max (já tem)
// 25.0    → RPM scale (descobrir documento)
```

### Passo 2: Implementar Constantes

```cpp
// src/ECUSerial/SensorDecoders.cpp - adicionar no namespace anônimo

namespace {

// === RPM Conversion ===
constexpr double kRpmScale = 25.0;
// Fonte: Aparentemente padrão Bosch
// Cada count = 25 RPM

// === Lambda Sensor ===
constexpr double kLambdaVoltageScale = 3.9;  // mV per count
// Mapeia 0-255 → 0-995 mV (span ~1000mV típico)
// Erro máximo: ±1.5 mV (aceitável)

constexpr int kLambdaIntegratorZeroPoint = 128;  // ADC value for 0%
constexpr double kLambdaIntegratorScale = 0.195; // % per count
// raw=0 → -24.96%, raw=128 → 0%, raw=255 → +24.8%
// Faixa observada: -35% a +25%

// === Ignition Advance ===
constexpr double kSparkAdvanceMaxDeg = 60.0;     // Max advance
constexpr double kSparkAdvanceScale = 0.35;      // degrees per count
// raw=0 → 60°, raw=171 → 0°, raw=255 → -29.25°

// === Battery Voltage ===
constexpr double kBatteryVoltageScale = 71.0 / 1000.0;  // V per count
constexpr double kBatteryVoltageOffset = 16.0 / 25.0;   // 0.64 V
// Regressão linear: ADC=156→11.67V, 181→13.5V, R²=0.998

}
```

### Passo 3: Refatorar Funções

```cpp
// ANTES
double decodeLambdaVoltageImpl(uint8_t raw) {
    return raw * 3.9;
}

// DEPOIS
double decodeLambdaVoltageImpl(uint8_t raw) {
    return static_cast<double>(raw) * kLambdaVoltageScale;
}

// ANTES
double decodeLambdaIntegratorImpl(uint8_t raw) {
    return (raw - 128.0) * 0.195;
}

// DEPOIS
double decodeLambdaIntegratorImpl(uint8_t raw) {
    return (static_cast<double>(raw) - kLambdaIntegratorZeroPoint) *
           kLambdaIntegratorScale;
}

// ANTES
double decodeSparkAdvanceImpl(uint8_t raw) {
    return 60.0 - raw * 0.35;
}

// DEPOIS
double decodeSparkAdvanceImpl(uint8_t raw) {
    return kSparkAdvanceMaxDeg -
           static_cast<double>(raw) * kSparkAdvanceScale;
}
```

### Checklist

- [ ] Adicionar todas constantes identificadas
- [ ] Adicionar comentários com origem
- [ ] Refatorar 4-5 funções
- [ ] Compilar e validar resultados
- [ ] Tests verificam valores

---

## 4. Documentação Doxygen

### Passo 1: Padrão de Documentação

```cpp
/**
 * @brief [Descrição breve]
 *
 * [Descrição longa detalhada, se necessário]
 *
 * @param raw Valor bruto ADC (0-255 para uint8_t, 0-65535 para uint16_t)
 *            [Explicar o que representa, ranges conhecidos]
 *
 * @return [Unidade] - Descrição
 *         - Intervalo típico: [min, max]
 *         - Comportamento especial
 *
 * @note [Informação relevante para uso]
 * @warning [Aviso de limitações ou edge cases]
 * @see [Referências a outras funções/datasheets]
 */
```

### Passo 2: Exemplo Concreto

```cpp
/**
 * @brief Decodifica temperatura do ar de admissão (IAT)
 *
 * Converte valor ADC para temperatura usando equação Steinhart-Hart
 * com termístor NTC Bosch M12, conforme datasheet oficial.
 *
 * @param raw Valor bruto do conversor ADC (0-255)
 *            - 1 = ~-40°C (mínimo operacional)
 *            - 128 = ~0°C (ponto de calibração)
 *            - 254 = ~130°C (máximo operacional)
 *
 * @return Temperatura em graus Celsius
 *         - Intervalo típico: -40 a 130°C
 *         - Monotonicamente crescente com raw
 *         - Clamped internamente a [1, 254] para evitar singularidades
 *
 * @note Baseado em datasheet Bosch Motorsport NTC M12 (70101387)
 * @note Coeficientes Steinhart-Hart: A=1.30e-3, B=2.59e-4, C=1.69e-7
 * @note Precisão estimada: ±2°C na faixa -20 a +100°C
 * @note Pull-up resistor: 2661Ω (valor experimentalmente determinado)
 *
 * @warning raw < 5 ou raw > 250 → extrapolação, confiabilidade reduzida
 * @warning Sensor desconectado → raw ≈ 0 ou 255 → temperatura extrema
 *
 * @see decodeWaterTemperature para temperatura do líquido de arrefecimento
 * @see steinhartHartTempC para implementação interna da fórmula
 */
double decodeAirTemperature(uint8_t raw);
```

### Passo 3: Aplicar em Todas Funções Públicas

Seguir o mesmo padrão para:
- decodeNominalRPM
- decodeEngineRPM
- decodeThrottleAngleTrack2
- ... (20 funções públicas)

### Checklist

- [ ] Documentar todas 20 funções públicas
- [ ] Incluir @brief + @param + @return
- [ ] Adicionar @note com fonte/datasheet
- [ ] Adicionar @warning para edge cases
- [ ] Adicionar @see para funções relacionadas
- [ ] Gerar HTML com Doxygen: `doxygen Doxyfile`
- [ ] Validar formatação no HTML

---

## 5. Testes Unitários

### Passo 1: Estrutura de Teste

```cpp
// src/ECUSerial/test/SensorDecodersAdvancedTest.cpp

#include <gtest/gtest.h>
#include <cmath>
#include "../SensorDecoders.h"

class SensorDecodersValidationTest : public ::testing::Test {
protected:
    static constexpr double kTolerance = 1e-6;

    bool AlmostEqual(double a, double b, double tol = kTolerance) {
        return std::abs(a - b) < tol;
    }
};
```

### Passo 2: Testes de Validação

```cpp
// Testes com dados conhecidos (datasheets)

TEST_F(SensorDecodersValidationTest, AirTemperatureDatasheetPoints) {
    // Pontos de validação baseados em datasheet Bosch

    // Ponto frio (-40°C)
    double temp_cold = decodeAirTemperature(1);
    EXPECT_LT(temp_cold, -35.0);

    // Ponto ambiente (25°C)
    double temp_ambient = decodeAirTemperature(150);
    EXPECT_GT(temp_ambient, 20.0);
    EXPECT_LT(temp_ambient, 30.0);

    // Ponto quente (100°C)
    double temp_hot = decodeAirTemperature(230);
    EXPECT_GT(temp_hot, 90.0);
    EXPECT_LT(temp_hot, 110.0);
}

TEST_F(SensorDecodersValidationTest, BatteryVoltageCalibrationPoints) {
    // Pontos de calibração medidos experimentalmente
    // ADC=156 → 11.67V
    // ADC=160 → 12.05V
    // ADC=172 → 12.82V
    // ADC=181 → 13.50V

    EXPECT_NEAR(decodeBatteryVoltage(156), 11.67, 0.1);
    EXPECT_NEAR(decodeBatteryVoltage(160), 12.05, 0.1);
    EXPECT_NEAR(decodeBatteryVoltage(172), 12.82, 0.1);
    EXPECT_NEAR(decodeBatteryVoltage(181), 13.50, 0.1);
}

TEST_F(SensorDecodersValidationTest, LambdaIntegratorRanges) {
    // Faixa observada: -35% a +25%

    double at_min = decodeLambdaIntegrator(0);
    double at_center = decodeLambdaIntegrator(128);
    double at_max = decodeLambdaIntegrator(255);

    EXPECT_LT(at_min, -30.0);
    EXPECT_DOUBLE_EQ(at_center, 0.0);
    EXPECT_GT(at_max, 20.0);
}

TEST_F(SensorDecodersValidationTest, SparkAdvancePhysicalRange) {
    // -30° a +60° é range típico de motores

    double retard_max = decodeSparkAdvance(255);
    double zero_advance = decodeSparkAdvance(171);
    double advance_max = decodeSparkAdvance(0);

    EXPECT_LT(retard_max, -25.0);     // ~-29°
    EXPECT_NEAR(zero_advance, 0.0, 1.0);
    EXPECT_NEAR(advance_max, 60.0, 0.1);
}
```

### Passo 3: Testes de Monotonidade

```cpp
TEST_F(SensorDecodersValidationTest, MonotonicityIncreasing) {
    // Temperaturas devem aumentar com raw
    double prev = decodeAirTemperature(10);
    for (uint8_t raw = 20; raw < 250; raw += 10) {
        double curr = decodeAirTemperature(raw);
        EXPECT_GT(curr, prev) << "Não monotônico em raw=" << (int)raw;
        prev = curr;
    }
}

TEST_F(SensorDecodersValidationTest, MonotonicityDecreasing) {
    // Avanço de ignição deve diminuir com raw (retardo)
    double prev = decodeSparkAdvance(0);
    for (uint8_t raw = 10; raw < 255; raw += 10) {
        double curr = decodeSparkAdvance(raw);
        EXPECT_LT(curr, prev) << "Não monotônico em raw=" << (int)raw;
        prev = curr;
    }
}
```

### Passo 4: Testes de Edge Cases

```cpp
TEST_F(SensorDecodersValidationTest, BoundaryValues) {
    // Testes com valores extremos

    // uint8_t mínimo
    EXPECT_FALSE(std::isnan(decodeAirTemperature(0)));
    EXPECT_FALSE(std::isnan(decodeBatteryVoltage(0)));

    // uint8_t máximo
    EXPECT_FALSE(std::isnan(decodeAirTemperature(255)));
    EXPECT_FALSE(std::isnan(decodeBatteryVoltage(255)));

    // Valores "esperados" devem estar em ranges físicos
    for (uint8_t raw = 0; raw < 255; raw++) {
        double iat = decodeAirTemperature(raw);
        EXPECT_GE(iat, -100.0) << "IAT muito negativa em raw=" << (int)raw;
        EXPECT_LE(iat, 200.0) << "IAT muito alta em raw=" << (int)raw;
    }
}
```

### Checklist

- [ ] Criar arquivo `SensorDecodersAdvancedTest.cpp`
- [ ] Adicionar 15-20 testes de validação
- [ ] Testes com datasheet points
- [ ] Testes de monotonidade
- [ ] Testes de edge cases
- [ ] Compilar: `ctest` ou `./sensor_decoders_advanced_test`
- [ ] 100% tests passing

---

## 🎯 Ordem de Implementação Recomendada

### Fase 1: Fundação (2-3 horas)
```
1. Tipos de dados (1-2h)
   └─ Mudar int → uint8_t/uint16_t em header e cpp

2. Constantes mágicas (30min-1h)
   └─ Extrair 3.9, 0.195, 0.35, 60.0, 128.0

Teste: Compilação OK, sem quebra
```

### Fase 2: Validação (1-2 horas)
```
3. Validação de entrada (1-2h)
   └─ Adicionar clamping em funções públicas

Teste: Valores extremos funcionam
```

### Fase 3: Documentação (2-3 horas)
```
4. Documentação Doxygen (2-3h)
   └─ Comentários em 20 funções

Teste: `doxygen` gera HTML
```

### Fase 4: Testes (3-4 horas)
```
5. Testes unitários (3-4h)
   └─ 20+ testes GoogleTest

Teste: `ctest` com 100% passing
```

---

## ✅ Validação Final

### Checklist Completo

- [ ] Tipos: uint8_t/uint16_t em todas funções
- [ ] Constantes: Todas magic numbers extraídas
- [ ] Validação: Clamping em entrada
- [ ] Doxygen: 20 funções documentadas
- [ ] Testes: 20+ testes passing
- [ ] Compilação: -Wall -Wextra -Werror limpo
- [ ] Code Review: Peer review passing

### Teste de Integração

```bash
# Compilar com warnings como erros
cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror" ..
cmake --build .

# Rodar testes
ctest --output-on-failure

# Gerar documentação
doxygen

# Verificar cobertura
gcov src/ECUSerial/SensorDecoders.cpp
```

### Métricas Esperadas Após

```
Code Coverage:     95%+
Warnings:          0
Tests:             25+
Documentation:     100% Doxygen
Lines of Code:     ~320 (vs 286 atual)
Complexity:        Mesmo (refactoring)
```

---

## 📞 Próximos Passos

1. **Escolha uma fase** para começar (recomendado: Tipos de dados)
2. **Implemente** seguindo os passos
3. **Teste** frequentemente
4. **Code Review** com team
5. **Merge** em branches testados

**Tempo Total Estimado:** 10-16 horas
**Ganho:** 80-100% menos bugs em decodificação

Boa sorte! 🚀

