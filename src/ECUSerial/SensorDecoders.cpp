#include "SensorDecoders.h"
#include "KlineCollectionTables.h"
#include <algorithm>
#include <cmath>
#include <limits>

/**
 * @file SensorDecoders.cpp
 *
 * @warning VALIDAÇÃO EXPERIMENTAL INCOMPLETA
 *
 * Este módulo implementa decodificadores de sensores para leitura de dados do
 * ECU. Algumas calibrações foram assistidas por IA (ChatGPT) e nem todas foram
 * validadas experimentalmente em campo. O comportamento observado é que:
 *
 * ✅ VALIDADO EM CAMPO:
 *    • Tensão de bateria (RAM_7D e RAM_61): 4 pontos de medição, R² = 0.998
 *    • Marcha (RPM, TPS): Observação visual durante operação
 *    • Básicos de temperatura: Comportamento monotonico validado
 *
 * ⚠️  PARCIALMENTE VALIDADO:
 *    • Coeficientes Steinhart-Hart: Baseados em datasheet + aproximação por IA
 *    • Lambda sensor: Mapeamento linear assumido, não validado contra sensor
 * real • Avanço de ignição: Escala inferida, sem verificação com osciloscópio
 *
 * ❌ NÃO VALIDADO:
 *    • Sensores EVAP, injeção de combustível, e periféricos especializados
 *    • Precisão dos cálculos em condições extremas (-40°C, 150°C)
 *    • Efeitos de temperatura no comportamento dos sensores
 *
 * @note RECOMENDAÇÃO: Não confie cegamente neste módulo para:
 *       • Controle crítico de segurança (limp-home, proteção motor)
 *       • Leitura de diagnóstico em ambientes de produção sem validação
 *       • Decisões de manutenção baseadas apenas nestes valores
 *
 * @recommendation TODO: Validação experimental completa com equipamento
 * calibrado (osciloscópio, multímetro digital, câmara térmica) antes de usar em
 * ambiente de produção.
 */

namespace {
constexpr int toSigned8(int rawVal) {
    return (rawVal > 127) ? rawVal - 256 : rawVal;
}
constexpr double kNoBound = std::numeric_limits<double>::quiet_NaN();

// Alias for cleaner sensor entry definitions
using Cat = ecu::SensorCategory;

// Conversion implementations
constexpr double kTpsMaxDeg = 90.0;
constexpr double kTps1MaxDeg = 24.0;
constexpr double kTps2MinDeg = 18.0;
constexpr double kTps2MaxDeg = kTpsMaxDeg;
constexpr double kTps2StartCountsD =
    (kTps2MinDeg / kTpsMaxDeg) * 255.0; // track 2 only meaningful past this
constexpr int kTps2StartCounts =
    static_cast<int>(kTps2StartCountsD + 0.5); // rounded threshold
constexpr double kTps2CountsRange = 255.0 - kTps2StartCounts;
constexpr double kTps2BelowSlope =
    (kTps2StartCounts > 0)
        ? (kTps2MinDeg / static_cast<double>(kTps2StartCounts))
        : 0.0; // linear ramp up to ~18° for debugging below useful range
constexpr double kTps2DegPerCount =
    (kTps2MaxDeg - kTps2MinDeg) / kTps2CountsRange;
constexpr double kTpsCombinedScale = kTpsMaxDeg / 255.0;
constexpr double kTps1PercentScale = (kTps1MaxDeg / kTpsMaxDeg * 100.0) / 255.0;
constexpr double kBatteryVoltageRam7DSlope =
    0.075; // ≈ (256/218)*0.0637 com RAM_64 ~0xDA; arredondado, erro ~0.08 V
constexpr double kBatteryVoltageSlope =
    71.0 / 1000.0; // fração simples (~0.071); err máx ~0.05 V nos pontos
                   // (ajuste e arredondamento assistidos por IA a partir dos 4
                   // pontos medidos)
constexpr double kBatteryVoltageOffset =
    16.0 / 25.0; // 0.64; regressão arredondada: 156->11.67V, 160->12.05V,
                 // 172->12.82V, 181->13.5V

// Bosch NTC M12 Steinhart–Hart (datasheet) coefficients
// Fonte: Bosch Motorsport "Temperature Sensor NTC M12" (Data Sheet 70101387)
// e Steinhart & Hart (1968) – Calibration curves for thermistors.
// Curva/coeficientes ajustados com auxílio de IA (ChatGPT) a partir do
// datasheet público.
constexpr double kShA = 0.00130233960469996;
constexpr double kShB = 0.00025894836043228303;
constexpr double kShC = 0.0000001691900955030085;
constexpr double kKelvinOffset = 273.15;
constexpr double kRpuAir = 2661.0;   // Ω (melhor encaixe ar)
constexpr double kRpuWater = 3004.0; // Ω (melhor encaixe água)

double steinhartHartTempC(int raw, double rpuOhm) {
    int clamped =
        std::clamp(raw, 1, 254); // evita divisão por zero e saturações
    double x = static_cast<double>(clamped);
    // Divisor: pull-up em Vref, NTC para GND
    double rntc = rpuOhm * x / (255.0 - x);
    double lnR = std::log(rntc);
    double invT = kShA + kShB * lnR + kShC * lnR * lnR * lnR; // 1/K
    double tempK = 1.0 / invT;
    return tempK - kKelvinOffset;
}

/**
 * @brief aparentemente 25 é um multiplicador comum usado pela Bosch
 */
double decodeNominalRPMImpl(int raw) { return raw * 25.0; }

double decodeEngineRPMImpl(int raw) { return raw * 25.0; }

double decodeThrottleAngleTrack2Impl(int raw) {
    if (raw <= kTps2StartCounts) {
        // Track 2 only becomes useful after ~18°; linearly ramp for debugging
        return raw * kTps2BelowSlope;
    }
    return kTps2MinDeg +
           (static_cast<double>(raw) - kTps2StartCounts) * kTps2DegPerCount;
}

double decodeAirTemperatureImpl(int raw) {
    return steinhartHartTempC(raw, kRpuAir);
}

double decodeWaterTemperatureImpl(int raw) {
    return steinhartHartTempC(raw, kRpuWater);
}

double decodeLambdaVoltageImpl(int raw) { return raw * 3.9; }

double decodeThrottlePercentTrack1Impl(int raw) {
    return raw * kTps1PercentScale;
}

double decodeSparkAdvanceImpl(int raw) { return 60.0 - raw * 0.35; }

double decodeThrottleCombinedAngleImpl(int raw) {
    return raw * kTpsCombinedScale;
}

double decodeThrottleAngleImpl(int raw) { return raw * kTpsCombinedScale; }

double decodeBatteryVoltageImpl(int raw) {
    return raw * kBatteryVoltageRam7DSlope;
}

double decodeBatteryVoltageRam61Impl(int raw) {
    return raw * kBatteryVoltageSlope + kBatteryVoltageOffset;
}

constexpr double decodeInjectionTimeImpl(int raw16) {
    /*
    Evidence from ROM, not cheat sheets:
    - T2CON is set to 0x85 in FiatTipoIDAFullDump_27c256.asm:2399.
      Per SAB80C517.txt:3465, that means T2PS=1 (prescaler /24),
      T2I1:T2I0=01 (timer mode), T2CM=1 (compare). So Timer2 tick = 24/Fosc.
      If Fosc = 12 MHz, Tw = 2 us per tick.
    - timer_update_cc3_from_ram14_ram15 (FiatTipoIDAFullDump_27c256.asm:1258)
      builds raw = (RAM_14<<8)|RAM_15, shifts left once with rlc/rlc/xch,
      then adds into CCL3/CCH3. So ticks_added = raw << 1.
      Therefore pulse_duration = (raw << 1) * Tw = raw * (48/Fosc).
      With 12 MHz: raw * 4 us.
    - Special branch: if RAM_14==0 and RAM_15 < 0x0D, the CJNE leaves C=1
      and the code exits to iex3_cc3_finalize_restore_ircon (no update).
    - raw comes from timer_tf0_handler (FiatTipoIDAFullDump_27c256.asm:6319)
      via R3:R2 with two mul_u16_by_u8_to_u24 ops and RAM_7F*2 added; it
      saturates at 0xFFFF before the shift/add
    (FiatTipoIDAFullDump_27c256.asm:6255-6321).
    */
    return (raw16 * 4.0) / 1000.0;
}

double decodeLambdaIntegratorImpl(int raw) { return (raw - 128.0) * 0.195; }

double decodeRevRegulatorAngleImpl(int raw) { return raw * kTps1PercentScale; }

double decodeFTEADSelfAdaptionImpl(int raw) { return raw / 256.0; }

double decodeGeneralMapAdaptionImpl(int raw) { return toSigned8(raw) * 0.39; }

double decodeMLLECKCoefficientImpl(int raw) { return toSigned8(raw) * 24.0; }

double decodeTSCoefficientImpl(int raw) { return toSigned8(raw) * 8.0; }

double decodeEvaporatorValveImpl(int raw) {
    return (raw > 50) ? 0.0 : (50.0 - raw) * 2.0;
}

double decodeRawImpl(int raw) { return raw; }

// clang-format off
constexpr std::array<SensorDecoderEntry, 22> kSensorDecoders = {{
    // rpm_nominal - 0x46/0x00 - ENGINE - not in collection tables
    {"rpm_nominal", "RPM Nominal", "Engine speed target", "ecu/scanner", 0x46,
     0x00, 1, decodeNominalRPMImpl, "rpm",
     "Rotação alvo calculada pela ECU (setpoint).", 0.0, 6400.0, kNoBound,
     5800.0, Cat::ENGINE, false},

    // rpm_engine - 0x47/0x00 - ENGINE - in table1
    {"rpm_engine", "RPM Motor", "Engine speed", "ecu", 0x47, 0x00, 1,
     decodeEngineRPMImpl, "rpm",
     "Rotação do motor derivada do sinal de referência do virabrequim.", 0.0,
     6400.0, kNoBound, 5800.0, Cat::ENGINE, true},

    // tps_track2_deg - 0x60/0x00 - ENGINE - not in collection tables
    {"tps_track2_deg", "TPS Trilha 2", "Throttle position sensor track 2",
     "ecu/scanner", 0x60, 0x00, 1, decodeThrottleAngleTrack2Impl, "graus",
     "Trilha 2 do TPS (~18–90°); utilizada para coerência/redundância.", 0.0,
     90.0, kNoBound, kNoBound, Cat::ENGINE, false},

    // iat - 0x62/0x00 - TEMPERATURE - in table1
    {"iat", "Temp. Ar (IAT)", "Intake air temperature",
     "Bosch NTC M12 datasheet", 0x62, 0x00, 1, decodeAirTemperatureImpl, "°C",
     "Temperatura do ar de admissão (NTC no corpo de borboleta).", -40.0, 130.0,
     kNoBound, kNoBound, Cat::TEMPERATURE, true},

    // ect - 0x63/0x00 - TEMPERATURE - in both tables
    {"ect", "Temp. Água (ECT)", "Coolant temperature",
     "Bosch NTC M12 datasheet", 0x63, 0x00, 1, decodeWaterTemperatureImpl, "°C",
     "Temperatura do líquido de arrefecimento (NTC em contato com o líquido).",
     -40.0, 140.0, kNoBound, kNoBound, Cat::TEMPERATURE, true},

    // lambda_voltage - 0x65/0x00 - FUEL_LAMBDA - not in collection tables
    {"lambda_voltage", "Sonda mV", "Lambda sensor voltage", "ecu", 0x65, 0x00,
     1, decodeLambdaVoltageImpl, "mV",
     "Tensão medida da sonda lambda; tende a oscilar em malha fechada.", 0.0,
     1000.0, kNoBound, kNoBound, Cat::FUEL_LAMBDA, false},

    // tps_track1_pct - 0x66/0x00 - ENGINE - in both tables
    {"tps_track1_pct", "TPS Trilha 1", "Throttle position sensor track 1",
     "ecu/scanner", 0x66, 0x00, 1, decodeThrottlePercentTrack1Impl, "%",
     "Trilha 1 do TPS em % do curso útil (~0–24°).", 0.0, 100.0, kNoBound,
     kNoBound, Cat::ENGINE, true},

    // ign_advance_deg - 0x69/0x00 - ENGINE - not in collection tables
    {"ign_advance_deg", "Avanço Ignição", "Ignition advance", "ecu", 0x69, 0x00,
     1, decodeSparkAdvanceImpl, "graus", "Ângulo de avanço calculado pela ECU.",
     -20.0, 60.0, kNoBound, kNoBound, Cat::ENGINE, false},

    // tps_combined_deg - 0x6F/0x00 - ENGINE - not in collection tables
    {"tps_combined_deg", "Borboleta Combinada", "Combined throttle angle",
     "ecu", 0x6F, 0x00, 1, decodeThrottleCombinedAngleImpl, "graus",
     "Ângulo combinado das trilhas 1+2 (0–90°).", 0.0, 90.0, kNoBound,
     kNoBound, Cat::ENGINE, false},

    // tps_absolute_deg - 0x70/0x00 - ENGINE - in table2
    {"tps_absolute_deg", "Ângulo Borboleta", "Absolute throttle angle", "ecu",
     0x70, 0x00, 1, decodeThrottleAngleImpl, "graus",
     "Posição absoluta da borboleta.", 0.0, 90.0, kNoBound, kNoBound,
     Cat::ENGINE, true},

    // batteryvoltage_comp - 0x7D/0x00 - ELECTRICAL - not in collection tables
    {"batteryvoltage_comp", "Bateria (RAM_7D)", "Battery voltage compensated",
     "ecu", 0x7D, 0x00, 1, decodeBatteryVoltageImpl, "V",
     "Tensão de bateria pós-processada (RAM_7D, usada para compensações).", 8.0,
     16.0, 11.5, 15.0, Cat::ELECTRICAL, false},

    // inj_time_ms - 0x14/0x00 - FUEL_LAMBDA - not in collection tables
    {"inj_time_ms", "Tempo Injeção", "Injection pulse width", "ecu", 0x14, 0x00,
     2, decodeInjectionTimeImpl, "ms",
     "Tempo de injeção (pulso base + compensações).", 0.0, 20.0, kNoBound,
     kNoBound, Cat::FUEL_LAMBDA, false},

    // lambda_integrator_pct - 0x0C/0xF8 - FUEL_LAMBDA - in table1
    {"lambda_integrator_pct", "Integrador Lambda", "Lambda integrator", "ecu",
     0x0C, 0xF8, 1, decodeLambdaIntegratorImpl, "%",
     "Integrador lambda (faixa observada próxima de +25%/-35%).", -40.0, 40.0,
     -35.0, 25.0, Cat::FUEL_LAMBDA, true},

    // idle_regulator_deg - 0x3D/0xF8 - ENGINE - in table1
    {"idle_regulator_deg", "Atuador Marcha Lenta", "Idle actuator position",
     "ecu", 0x3D, 0xF8, 1, decodeRevRegulatorAngleImpl, "graus",
     "Abertura do atuador de marcha lenta (alavanca da borboleta).", 0.0, 30.0,
     kNoBound, kNoBound, Cat::ENGINE, true},

    // evap_purge_adapt_factor - 0x69/0xF8 - DIAGNOSTICS - in both tables
    {"evap_purge_adapt_factor", "Adaptação Purga EVAP",
     "EVAP purge adaptation factor", "scanner/hipótese", 0x69, 0xF8, 1,
     decodeFTEADSelfAdaptionImpl, "ratio",
     "Fator adaptativo associado ao efeito da purga EVAP na mistura "
     "(adimensional; semântica depende da convenção do scanner/ECU).",
     0.0, 1.0, kNoBound, kNoBound, Cat::DIAGNOSTICS, true},

    // fuel_trim_long_term_pct - 0x6C/0xF8 - FUEL_LAMBDA - in both tables
    {"fuel_trim_long_term_pct", "Correção Longo Prazo (LTFT)",
     "Long-term fuel trim", "ecu", 0x6C, 0xF8, 1, decodeGeneralMapAdaptionImpl,
     "%",
     "Correção de longo prazo aprendida para compensar desvios persistentes de "
     "mistura (LTFT).",
     -100.0, 100.0, -50.0, 25.0, Cat::FUEL_LAMBDA, true},

    // idle_fuel_additive_us - 0x6F/0xF8 - FUEL_LAMBDA - in both tables
    {"idle_fuel_additive_us", "Correção Aditiva em Lenta",
     "Idle additive fuel correction", "scanner/hipótese", 0x6F, 0xF8, 1,
     decodeMLLECKCoefficientImpl, "µs",
     "Correção aditiva (µs) aplicada ao tempo de injeção em marcha lenta para "
     "compensar offsets/variações finas.",
     -4000.0, 4000.0, kNoBound, kNoBound, Cat::FUEL_LAMBDA, true},

    // transient_fuel_additive_us - 0x72/0xF8 - FUEL_LAMBDA - in table2
    {"transient_fuel_additive_us", "Correção Aditiva Transitória",
     "Transient additive fuel correction", "scanner/hipótese", 0x72, 0xF8, 1,
     decodeTSCoefficientImpl, "µs",
     "Correção aditiva (µs) auxiliar associada a mudanças dinâmicas de "
     "carga/borboleta (interpretação depende do scanner/ECU).",
     -2500.0, 2500.0, kNoBound, kNoBound, Cat::FUEL_LAMBDA, true},

    // evap_valve_pct - 0x8C/0xF8 - DIAGNOSTICS - not in collection tables
    {"evap_valve_pct", "Válvula EVAP", "EVAP purge command duty", "ecu", 0x8C,
     0xF8, 1, decodeEvaporatorValveImpl, "%",
     "Duty-cycle de comando da purga do canister/EVAP.", 0.0, 100.0, kNoBound,
     kNoBound, Cat::DIAGNOSTICS, false},

    // batteryvoltage_adc - 0x61/0x00 - ELECTRICAL - in both tables
    {"batteryvoltage_adc", "Bateria (RAM_61)", "Battery voltage ADC", "ecu",
     0x61, 0x00, 1, decodeBatteryVoltageRam61Impl, "V",
     "ADC de bateria calibrado (RAM_61, origem do RAM_7D).", 8.0, 16.0, 11.5,
     15.0, Cat::ELECTRICAL, true},

    // fuel_system_state_flags_u8 - 0x2F/0x00 - DIAGNOSTICS - in table2
    {"fuel_system_state_flags_u8", "Flags do Sistema de Combustível",
     "Fuel system state flags (raw)", "ecu", 0x2F, 0x00, 1, decodeRawImpl,
     "flags",
     "Flags de estado do sistema (RAW). Recomenda-se decodificar bits para "
     "status legíveis (cut-off, malha fechada etc.).",
     0.0, 255.0, kNoBound, kNoBound, Cat::DIAGNOSTICS, true},

    // adapt_block_start_u8 - 0x40/0xF8 - DIAGNOSTICS - in table2
    {"adapt_block_start_u8", "Bloco de Adaptação (início)",
     "Adaptation block start (raw)", "ecu/depuração", 0x40, 0xF8, 1,
     decodeRawImpl, "raw",
     "Primeiro byte do bloco adaptativo (RAW). Útil para depuração/engenharia.",
     0.0, 255.0, kNoBound, kNoBound, Cat::DIAGNOSTICS, true},
}};
// clang-format on
} // namespace

// Public entry points (simple forwarders to keep signatures stable)
double decodeNominalRPM(int raw) { return decodeNominalRPMImpl(raw); }
double decodeEngineRPM(int raw) { return decodeEngineRPMImpl(raw); }
double decodeThrottleAngleTrack2(int raw) {
    return decodeThrottleAngleTrack2Impl(raw);
}
double decodeAirTemperature(int raw) { return decodeAirTemperatureImpl(raw); }
double decodeWaterTemperature(int raw) {
    return decodeWaterTemperatureImpl(raw);
}
double decodeLambdaVoltage(int raw) { return decodeLambdaVoltageImpl(raw); }
double decodeThrottlePercentTrack1(int raw) {
    return decodeThrottlePercentTrack1Impl(raw);
}
double decodeSparkAdvance(int raw) { return decodeSparkAdvanceImpl(raw); }
double decodeThrottleCombinedAngle(int raw) {
    return decodeThrottleCombinedAngleImpl(raw);
}
double decodeThrottleAngle(int raw) { return decodeThrottleAngleImpl(raw); }
double decodeBatteryVoltage(int raw) { return decodeBatteryVoltageImpl(raw); }
double decodeInjectionTime(int raw16) { return decodeInjectionTimeImpl(raw16); }
double decodeLambdaIntegrator(int raw) {
    return decodeLambdaIntegratorImpl(raw);
}
double decodeRevRegulatorAngle(int raw) {
    return decodeRevRegulatorAngleImpl(raw);
}
double decodeFTEADSelfAdaption(int raw) {
    return decodeFTEADSelfAdaptionImpl(raw);
}
double decodeGeneralMapAdaption(int raw) {
    return decodeGeneralMapAdaptionImpl(raw);
}
double decodeMLLECKCoefficient(int raw) {
    return decodeMLLECKCoefficientImpl(raw);
}
double decodeTSCoefficient(int raw) { return decodeTSCoefficientImpl(raw); }
double decodeEvaporatorValve(int raw) { return decodeEvaporatorValveImpl(raw); }
double decodeRaw(int raw) { return decodeRawImpl(raw); }

std::span<const SensorDecoderEntry> GetSensorDecoders() {
    return std::span<const SensorDecoderEntry>(kSensorDecoders);
}

const SensorDecoderEntry *FindSensorDecoder(uint8_t subcommand, uint8_t id) {
    const auto it = std::find_if(
        kSensorDecoders.begin(), kSensorDecoders.end(),
        [subcommand, id](const SensorDecoderEntry &entry) {
            return entry.subcommand == subcommand && entry.id == id;
        });
    if (it == kSensorDecoders.end()) {
        return nullptr;
    }
    return &(*it);
}
