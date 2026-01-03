#pragma once

#include <array>
#include <cstdint>
#include <utility>

namespace ecu {

/**
 * @brief K-Line collection table entry (sensor ID + subcommand)
 *
 * These tables define which sensors are read in batch mode via the
 * ECU's collection request (RAM_B3 register selection).
 *
 * @note Tables are constexpr (NASA P10 compliant - zero runtime allocation)
 */
using KlineEntry = std::pair<uint8_t, uint8_t>;

/// Number of sensor slots per collection table
constexpr size_t kCollectionSlots = 10;

/**
 * @brief Collection Table 1 - Selected when RAM_B3 bit 1 is set
 *
 * Sensors:
 *  0: 0x63/0x00 - Coolant temperature (ECT)
 *  1: 0x66/0x00 - TPS track 1 (%)
 *  2: 0x47/0x00 - Engine RPM
 *  3: 0x62/0x00 - Air temperature (IAT)
 *  4: 0x0C/0xF8 - Lambda integrator
 *  5: 0x6C/0xF8 - Long-term fuel trim (LTFT)
 *  6: 0x6F/0xF8 - Idle fuel additive (MLLECK)
 *  7: 0x3D/0xF8 - Idle actuator position
 *  8: 0x61/0x00 - Battery voltage (ADC)
 *  9: 0x69/0xF8 - EVAP purge adaptation
 */
constexpr std::array<KlineEntry, kCollectionSlots> kCollectionTable1{{
    {0x63, 0x00},  // ECT
    {0x66, 0x00},  // TPS track 1
    {0x47, 0x00},  // RPM
    {0x62, 0x00},  // IAT
    {0x0C, 0xF8},  // Lambda integrator
    {0x6C, 0xF8},  // LTFT
    {0x6F, 0xF8},  // MLLECK coef
    {0x3D, 0xF8},  // Idle actuator
    {0x61, 0x00},  // Battery ADC
    {0x69, 0xF8}   // EVAP purge adapt
}};

/**
 * @brief Collection Table 2 - Selected when RAM_B3 bit 0 is set
 *
 * Sensors:
 *  0: 0x63/0x00 - Coolant temperature (ECT)
 *  1: 0x66/0x00 - TPS track 1 (%)
 *  2: 0x70/0x00 - Absolute throttle angle
 *  3: 0x2F/0x00 - Fuel system state flags
 *  4: 0x72/0xF8 - Transient fuel additive
 *  5: 0x6C/0xF8 - Long-term fuel trim (LTFT)
 *  6: 0x6F/0xF8 - Idle fuel additive (MLLECK)
 *  7: 0x40/0xF8 - Adaptation block start
 *  8: 0x61/0x00 - Battery voltage (ADC)
 *  9: 0x69/0xF8 - EVAP purge adaptation
 */
constexpr std::array<KlineEntry, kCollectionSlots> kCollectionTable2{{
    {0x63, 0x00},  // ECT
    {0x66, 0x00},  // TPS track 1
    {0x70, 0x00},  // Throttle angle
    {0x2F, 0x00},  // Fuel flags
    {0x72, 0xF8},  // TS coef
    {0x6C, 0xF8},  // LTFT
    {0x6F, 0xF8},  // MLLECK coef
    {0x40, 0xF8},  // Adapt block
    {0x61, 0x00},  // Battery ADC
    {0x69, 0xF8}   // EVAP purge adapt
}};

/**
 * @brief Check if sensor is in Collection Table 1
 */
constexpr bool IsInCollectionTable1(uint8_t id, uint8_t subcmd) {
    for (const auto& [entry_id, entry_subcmd] : kCollectionTable1) {
        if (entry_id == id && entry_subcmd == subcmd) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if sensor is in Collection Table 2
 */
constexpr bool IsInCollectionTable2(uint8_t id, uint8_t subcmd) {
    for (const auto& [entry_id, entry_subcmd] : kCollectionTable2) {
        if (entry_id == id && entry_subcmd == subcmd) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if sensor is in any collection table
 *
 * @param id Sensor PID
 * @param subcmd Sensor subcommand (0x00 or 0xF8)
 * @return true if sensor can be read via collection mode
 */
constexpr bool IsInAnyCollectionTable(uint8_t id, uint8_t subcmd) {
    return IsInCollectionTable1(id, subcmd) ||
           IsInCollectionTable2(id, subcmd);
}

/**
 * @brief Get collection table by ID
 *
 * @param table_id Table identifier (1 or 2)
 * @return Pointer to table array, or nullptr if invalid ID
 */
constexpr const std::array<KlineEntry, kCollectionSlots>*
GetCollectionTable(int table_id) {
    if (table_id == 1) {
        return &kCollectionTable1;
    }
    if (table_id == 2) {
        return &kCollectionTable2;
    }
    return nullptr;
}

}  // namespace ecu
