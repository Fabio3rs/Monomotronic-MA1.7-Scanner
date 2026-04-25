#pragma once

#include <cstdint>
#include <string_view>

namespace ecu {

/**
 * @brief Category classification for ECU sensors
 *
 * Used for UI grouping and organization of sensor displays.
 * Categories are read-only after initialization (thread-safe).
 */
enum class SensorCategory : uint8_t {
    ENGINE = 0,   // RPM, TPS, throttle, ignition
    TEMPERATURE,  // ECT, IAT
    ELECTRICAL,   // Battery voltage
    FUEL_LAMBDA,  // Lambda, trims, injection
    DIAGNOSTICS,  // Flags, EVAP, adaptation blocks
    UNKNOWN
};

/**
 * @brief Get human-readable display name for a category
 *
 * @param cat The sensor category
 * @return Display name in Portuguese (UI language)
 */
constexpr std::string_view GetCategoryDisplayName(SensorCategory cat) {
    switch (cat) {
    case SensorCategory::ENGINE:
        return "Motor";
    case SensorCategory::TEMPERATURE:
        return "Temperatura";
    case SensorCategory::ELECTRICAL:
        return "Elétrico";
    case SensorCategory::FUEL_LAMBDA:
        return "Lambda/Combustível";
    case SensorCategory::DIAGNOSTICS:
        return "Diagnóstico";
    case SensorCategory::UNKNOWN:
    default:
        return "Desconhecido";
    }
}

/**
 * @brief Get English category name (for logs/export)
 *
 * @param cat The sensor category
 * @return English category name
 */
constexpr std::string_view GetCategoryEnglishName(SensorCategory cat) {
    switch (cat) {
    case SensorCategory::ENGINE:
        return "Engine";
    case SensorCategory::TEMPERATURE:
        return "Temperature";
    case SensorCategory::ELECTRICAL:
        return "Electrical";
    case SensorCategory::FUEL_LAMBDA:
        return "Fuel/Lambda";
    case SensorCategory::DIAGNOSTICS:
        return "Diagnostics";
    case SensorCategory::UNKNOWN:
    default:
        return "Unknown";
    }
}

}  // namespace ecu
