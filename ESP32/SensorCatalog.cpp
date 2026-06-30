#include "SensorCatalog.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr float kTps1PercentScale = ((24.0f / 90.0f) * 100.0f) / 255.0f;
constexpr float kBatteryVoltageSlope = 71.0f / 1000.0f;
constexpr float kBatteryVoltageOffset = 16.0f / 25.0f;
constexpr float kKelvinOffset = 273.15f;
constexpr float kRpuAir = 2661.0f;
constexpr float kRpuWater = 3004.0f;
constexpr float kShA = 0.00130233960469996f;
constexpr float kShB = 0.00025894836043228303f;
constexpr float kShC = 0.0000001691900955030085f;

constexpr int ToSigned8(int rawVal) {
    return (rawVal > 127) ? rawVal - 256 : rawVal;
}

float SteinhartHartTempC(int raw, float pullup_ohm) {
    int clamped = std::max(1, std::min(raw, 254));
    float x = static_cast<float>(clamped);
    float rntc = pullup_ohm * x / (255.0f - x);
    float lnR = std::log(rntc);
    float invT = kShA + kShB * lnR + kShC * lnR * lnR * lnR;
    float tempK = 1.0f / invT;
    return tempK - kKelvinOffset;
}

float DecodeEngineRPM(int raw) { return raw * 25.0f; }
float DecodeAirTemperature(int raw) { return SteinhartHartTempC(raw, kRpuAir); }
float DecodeWaterTemperature(int raw) {
    return SteinhartHartTempC(raw, kRpuWater);
}
float DecodeThrottlePercentTrack1(int raw) { return raw * kTps1PercentScale; }
float DecodeThrottleAngle(int raw) { return raw * (90.0f / 255.0f); }
float DecodeLambdaIntegrator(int raw) { return (raw - 128.0f) * 0.195f; }
float DecodeGeneralMapAdaption(int raw) { return ToSigned8(raw) * 0.39f; }
float DecodeMLLECKCoefficient(int raw) { return ToSigned8(raw) * 24.0f; }
float DecodeIdleRegulator(int raw) { return raw * kTps1PercentScale; }
float DecodeBatteryVoltage(int raw) {
    return raw * kBatteryVoltageSlope + kBatteryVoltageOffset;
}
float DecodeFTEADSelfAdaption(int raw) { return raw / 256.0f; }
float DecodeTSCoefficient(int raw) { return ToSigned8(raw) * 8.0f; }
float DecodeRaw(int raw) { return static_cast<float>(raw); }

constexpr std::array<SensorCatalogEntry, 13> kSensorCatalog{{
    {"ect", "Temp. Agua", "C", 0x63, 0x00, 1, 1, DecodeWaterTemperature},
    {"tps_track1_pct", "TPS Trilha 1", "%", 0x66, 0x00, 1, 1,
     DecodeThrottlePercentTrack1},
    {"rpm_engine", "RPM Motor", "rpm", 0x47, 0x00, 1, 1, DecodeEngineRPM},
    {"iat", "Temp. Ar", "C", 0x62, 0x00, 1, 1, DecodeAirTemperature},
    {"lambda_integrator_pct", "Integrador Lambda", "%", 0x0C, 0xF8, 1, 1,
     DecodeLambdaIntegrator},
    {"fuel_trim_long_term_pct", "LTFT", "%", 0x6C, 0xF8, 1, 1,
     DecodeGeneralMapAdaption},
    {"idle_fuel_additive_us", "Correcao Lenta", "us", 0x6F, 0xF8, 1, 1,
     DecodeMLLECKCoefficient},
    {"idle_regulator_deg", "Atuador Lenta", "graus", 0x3D, 0xF8, 1, 1,
     DecodeIdleRegulator},
    {"batteryvoltage_adc", "Bateria", "V", 0x61, 0x00, 1, 1,
     DecodeBatteryVoltage},
    {"evap_purge_adapt_factor", "Adaptacao EVAP", "ratio", 0x69, 0xF8, 1, 1,
     DecodeFTEADSelfAdaption},
    {"tps_absolute_deg", "Angulo Borboleta", "graus", 0x70, 0x00, 1, 2,
     DecodeThrottleAngle},
    {"fuel_system_state_flags_u8", "Flags Combustivel", "flags", 0x2F, 0x00, 1,
     2, DecodeRaw},
    {"transient_fuel_additive_us", "Correcao Transitoria", "us", 0x72, 0xF8, 1,
     2, DecodeTSCoefficient},
}};
} // namespace

const std::array<KlineEntry, kCollectionSlots> kCollectionTable1{{
    {0x63, 0x00},
    {0x66, 0x00},
    {0x47, 0x00},
    {0x62, 0x00},
    {0x0C, 0xF8},
    {0x6C, 0xF8},
    {0x6F, 0xF8},
    {0x3D, 0xF8},
    {0x61, 0x00},
    {0x69, 0xF8},
}};

const std::array<KlineEntry, kCollectionSlots> kCollectionTable2{{
    {0x63, 0x00},
    {0x66, 0x00},
    {0x70, 0x00},
    {0x2F, 0x00},
    {0x72, 0xF8},
    {0x6C, 0xF8},
    {0x6F, 0xF8},
    {0x40, 0xF8},
    {0x61, 0x00},
    {0x69, 0xF8},
}};

const SensorCatalogEntry *GetSensorCatalog() { return kSensorCatalog.data(); }

size_t GetSensorCatalogCount() { return kSensorCatalog.size(); }

const SensorCatalogEntry *FindSensorCatalogEntry(uint8_t subcommand,
                                                 uint8_t id) {
    for (const auto &entry : kSensorCatalog) {
        if (entry.id == id && entry.subcommand == subcommand) {
            return &entry;
        }
    }
    return nullptr;
}

const std::array<KlineEntry, kCollectionSlots> *
GetCollectionTable(uint8_t table_id) {
    if (table_id == 1) {
        return &kCollectionTable1;
    }
    if (table_id == 2) {
        return &kCollectionTable2;
    }
    return nullptr;
}
