#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "SensorCategory.h"

/**
 * @file SensorDecoders.h
 * @brief Interface para decodificação de sensores do ECU Bosch MA1.7
 *
 * @warning VALIDAÇÃO EXPERIMENTAL INCOMPLETA
 *
 * Este módulo define o contrato para leitura e decodificação de dados brutos
 * de sensores do ECU. Algumas calibrações foram assistidas por IA e nem todas
 * foram validadas experimentalmente. Consulte SensorDecoders.cpp para detalhes
 * completos sobre o status de validação de cada sensor.
 *
 * @see SensorDecoders.cpp para advertências e status de validação detalhados
 */

struct SensorDecoderEntry {
    std::string_view key;            // Identifier used in CLI/DLL maps
    std::string_view display_name;   // Short UI-friendly label
    std::string_view technical_name; // Manual/engineering term when available
    std::string_view source_note; // Provenance hint (manual/scanner/inferred)
    uint8_t id;                   // Sensor PID
    uint8_t subcommand;           // ECU subcommand (0x00 normal, 0xF8 diag)
    uint8_t length;               // Bytes to read (1 or 2)
    double (*decode)(int);        // Raw-to-engineering conversion
    std::string_view unit;        // Engineering unit (for UI)
    std::string_view description; // Human-friendly label/tooltip
    double display_min;           // Suggested lower bound for UI (NaN if N/A)
    double display_max;           // Suggested upper bound for UI (NaN if N/A)
    double alert_min;             // Safe/alert lower threshold (NaN if unknown)
    double alert_max;             // Safe/alert upper threshold (NaN if unknown)
    ecu::SensorCategory category; // Sensor category for UI grouping
    bool collection_capable;      // Can be read via K-Line collection mode
};

// Conversion functions reused by CLI, DLL, and GUI
double decodeNominalRPM(int raw);
double decodeEngineRPM(int raw);
double decodeThrottleAngleTrack2(int raw);
double decodeAirTemperature(int raw);
double decodeWaterTemperature(int raw);
double decodeLambdaVoltage(int raw);
double decodeThrottlePercentTrack1(int raw);
double decodeSparkAdvance(int raw);
double decodeThrottleCombinedAngle(int raw);
double decodeThrottleAngle(int raw);
double decodeBatteryVoltage(int raw);
double decodeInjectionTime(int raw16);
double decodeLambdaIntegrator(int raw);
double decodeRevRegulatorAngle(int raw);
double decodeFTEADSelfAdaption(int raw);
double decodeGeneralMapAdaption(int raw);
double decodeMLLECKCoefficient(int raw);
double decodeTSCoefficient(int raw);
double decodeEvaporatorValve(int raw);
double decodeRaw(int raw);

// Table of known sensors (PID + subcommand + converter)
std::span<const SensorDecoderEntry> GetSensorDecoders();
const SensorDecoderEntry *FindSensorDecoder(uint8_t subcommand, uint8_t id);
