#!/usr/bin/env python3
"""
sensor_database.py
Sensor metadata and conversion formulas for Fiat Tipo ECU (Bosch MA1.7)

Maps RAM addresses to sensor names, units, and conversion functions.
Source: README_AGENTE_RE.md K-Line sensor IDs and formulas
"""

from typing import Dict, Callable, Any, Optional
import math


# Bosch NTC M12 (datasheet 70101387) Steinhart–Hart; base: Steinhart & Hart (1968)
# Coeficientes ajustados com auxílio de IA (ChatGPT) a partir do datasheet público.
NTC_SH_A = 0.00130233960469996
NTC_SH_B = 0.00025894836043228303
NTC_SH_C = 0.0000001691900955030085
NTC_KELVIN_OFFSET = 273.15
RPU_AIR_OHM = 2661.0
RPU_WATER_OHM = 3004.0


def steinhart_hart_temp(raw: int, rpu_ohm: float) -> float:
    """Steinhart–Hart (Bosch NTC M12) with pull-up to Vref, NTC to GND."""
    clamped = max(1, min(254, raw))  # avoid div/0 and saturated extremes
    x = float(clamped)
    r_ntc = rpu_ohm * x / (255.0 - x)
    ln_r = math.log(r_ntc)
    inv_t = NTC_SH_A + NTC_SH_B * ln_r + NTC_SH_C * ln_r * ln_r * ln_r
    return 1.0 / inv_t - NTC_KELVIN_OFFSET


# Temperature conversion (Steinhart–Hart, datasheet Bosch NTC M12)
TPS_MAX_DEG = 90.0
TPS1_MAX_DEG = 24.0
TPS2_MIN_DEG = 18.0
TPS2_RANGE_DEG = TPS_MAX_DEG - TPS2_MIN_DEG
TPS2_START_COUNTS = (TPS2_MIN_DEG / TPS_MAX_DEG) * 255.0
TPS2_START_COUNTS_INT = int(round(TPS2_START_COUNTS))
TPS2_COUNTS_RANGE = 255.0 - TPS2_START_COUNTS_INT
TPS2_BELOW_SLOPE = TPS2_MIN_DEG / TPS2_START_COUNTS_INT if TPS2_START_COUNTS_INT > 0 else 0.0
TPS2_DEG_PER_COUNT = (TPS_MAX_DEG - TPS2_MIN_DEG) / TPS2_COUNTS_RANGE


def throttle_track2_angle(raw: int) -> float:
    """Throttle track 2 in degrees; only meaningful past ~18°."""
    if raw <= TPS2_START_COUNTS_INT:
        return raw * TPS2_BELOW_SLOPE
    return TPS2_MIN_DEG + (raw - TPS2_START_COUNTS_INT) * TPS2_DEG_PER_COUNT


def throttle_track1_percent(raw: int) -> float:
    """Track 1 expressed as % of full travel (tops out near 26.7%)."""
    return (raw / 255.0) * (TPS1_MAX_DEG / TPS_MAX_DEG) * 100.0


def throttle_combined_angle(raw: int) -> float:
    """Combined throttle angle in degrees (0–90° span)."""
    return (raw / 255.0) * TPS_MAX_DEG


def air_temp_polynomial(raw: int) -> float:
    return steinhart_hart_temp(raw, RPU_AIR_OHM)


def coolant_temp_polynomial(raw: int) -> float:
    return steinhart_hart_temp(raw, RPU_WATER_OHM)


def to_signed8(raw: int) -> int:
    """Convert 8-bit unsigned to signed (for values that can be negative)"""
    return raw - 256 if raw > 127 else raw


SENSOR_MAP: Dict[str, Dict[str, Any]] = {
    # Engine speed sensors
    'RAM_46': {
        'name': 'RPM_NOMINAL',
        'unit': 'RPM',
        'conversion': lambda x: x * 25.0,
        'description': 'RPM nominal setpoint (from ECU control logic)'
    },
    'RAM_47': {
        'name': 'ENGINE_RPM',
        'unit': 'RPM',
        'conversion': lambda x: x * 25.0,
        'description': 'Actual engine speed from crankshaft position sensor'
    },

    # Throttle position sensors
    'RAM_60': {
        'name': 'THROTTLE_ANGLE_TRACK2',
        'unit': '°',
        'conversion': throttle_track2_angle,
        'description': 'Throttle position sensor track 2 (K-Line ID 0x60, backup/validation)'
    },
    'RAM_66': {
        'name': 'THROTTLE_PERCENT_TRACK1',
        'unit': '%',
        'conversion': throttle_track1_percent,
        'description': 'Throttle position sensor track 1 percentage (K-Line ID 0x66)'
    },
    'RAM_6F': {
        'name': 'THROTTLE_COMBINED_ANGLE',
        'unit': '°',
        'conversion': throttle_combined_angle,
        'description': 'Combined throttle angle track1+track2 (K-Line ID 0x6F normal mode)'
    },
    'RAM_70': {
        'name': 'THROTTLE_ANGLE1',
        'unit': '°',
        'conversion': throttle_combined_angle,
        'description': 'Absolute throttle position (K-Line ID 0x70, primary)'
    },

    # Temperature sensors (exact polynomials from C++ code)
    'RAM_62': {
        'name': 'AIR_INTAKE_TEMP',
        'unit': '°C',
        'conversion': air_temp_polynomial,
        'description': 'Air intake temperature sensor (K-Line ID 0x62, NTC thermistor)'
    },
    'RAM_63': {
        'name': 'COOLANT_TEMP',
        'unit': '°C',
        'conversion': coolant_temp_polynomial,
        'description': 'Engine coolant temperature sensor (K-Line ID 0x63, NTC thermistor)'
    },

    # Lambda/oxygen sensor
    'RAM_65': {
        'name': 'LAMBDA_SENSOR',
        'unit': 'mV',
        'conversion': lambda x: x * 3.87,
        'description': 'Lambda sensor voltage (oxygen sensor for air/fuel ratio)'
    },

    # Ignition timing (normal mode 0x00)
    'RAM_69_NORMAL': {
        'name': 'IGNITION_ADVANCE',
        'unit': '°BTDC',
        'conversion': lambda x: 60.0 - x * 0.35,
        'description': 'Ignition timing advance (K-Line ID 0x69 subcommand 0x00)'
    },
    # FTEAD Adaption (diagnostic mode 0xF8)
    'RAM_69_DIAG': {
        'name': 'FTEAD_ADAPTION',
        'unit': '%',
        'conversion': lambda x: (x - 128) * 0.195,
        'description': 'FTEAD adaptation parameter (K-Line ID 0x69 subcommand 0xF8)'
    },

    # Injection pulse width (16-bit value, typically)
    'RAM_14': {
        'name': 'INJECTION_TIME',
        'unit': 'ms',
        'conversion': lambda x: (x * 4.0) / 1000.0,  # Assuming 16-bit raw value
        'description': 'Fuel injection pulse width'
    },

    # Battery voltage
    'RAM_7D': {
        'name': 'BATTERY_VOLTAGE',
        'unit': 'V',
        'conversion': lambda x: x * 0.0637,  # Exact from C++ code (K-Line ID 0x7D)
        'description': 'Battery voltage from ADC'
    },
    'RAM_61': {
        'name': 'BATTERY_VOLTAGE_RAW',
        'unit': 'ADC',
        'conversion': lambda x: x,
        'description': 'Battery voltage raw ADC staging buffer (staged before RAM_7D conversion, K-Line ID 0x61)'
    },

    # Advanced diagnostic parameters (subcommand 0xF8)
    'RAM_0C': {
        'name': 'LAMBDA_INTEGRATOR',
        'unit': '%',
        'conversion': lambda x: (x - 128) * 0.195,  # Signed offset
        'description': 'Lambda closed-loop integrator correction'
    },
    'RAM_3D': {
        'name': 'IDLE_RPM_REGULATOR',
        'unit': '°',
        'conversion': lambda x: x * 0.108,
        'description': 'Idle speed regulator position/angle'
    },
    'RAM_6C': {
        'name': 'MAP_ADAPTATION',
        'unit': '%',
        'conversion': lambda x: to_signed8(x) * 0.39,
        'description': 'General fuel map adaptation percentage (K-Line ID 0x6C subcommand 0xF8)'
    },
    'RAM_6F_DIAG': {
        'name': 'MLLECK_COEFFICIENT',
        'unit': 'µs',
        'conversion': lambda x: to_signed8(x) * 24.0,
        'description': 'MLLECK coefficient (K-Line ID 0x6F diag mode)'
    },
    'RAM_72': {
        'name': 'TS_COEFFICIENT',
        'unit': 'µs',
        'conversion': lambda x: to_signed8(x) * 8.0,
        'description': 'TS coefficient (K-Line ID 0x72 subcommand 0xF8)'
    },
    'RAM_8C': {
        'name': 'EVAP_VALVE',
        'unit': '%',
        'conversion': lambda x: (50 - x) * 2.0 if x <= 50 else 0.0,
        'description': 'EVAP canister purge valve duty cycle (K-Line ID 0x8C subcommand 0xF8)'
    },

    # Additional RAM addresses from error detection code
    'RAM_27': {
        'name': 'ERROR_FLAGS_1',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'Error enable flags byte 1 (ANDed with XRAM[0x84])'
    },
    'RAM_28': {
        'name': 'ERROR_FLAGS_2',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'Error enable flags byte 2 (ANDed with XRAM[0x85])'
    },
    'RAM_29': {
        'name': 'ERROR_FLAGS_3',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'Error enable flags byte 3 (ANDed with XRAM[0x86])'
    },
    'RAM_2E': {
        'name': 'KLINE_HANDSHAKE_FLAGS',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'K-Line handshake flags (bit0=advanced mode allowed, bit1=standard mode allowed, bit7=autodiag failure)'
    },
    'RAM_2F': {
        'name': 'FUEL_MODE_FLAGS',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'Fuel control mode flags (bit 0, 2 = fuel modes)'
    },
    'RAM_2B': {
        'name': 'DIAGNOSTIC_FLAGS',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'Diagnostic/status flags (bit 0, 3 used in error logic)'
    },
    'RAM_B3': {
        'name': 'KLINE_SESSION_FLAGS',
        'unit': 'flags',
        'conversion': lambda x: f"0x{x:02X}",
        'description': 'K-Line session flags (bit0/1 select capabilities table, bit7=K-Line active)'
    },
}

# XRAM addresses (extended RAM)
XRAM_MAP: Dict[str, Dict[str, Any]] = {
    # DTC flag bytes (read for K-Line transmission)
    'XRAM_0x4E': {
        'name': 'DTC_FLAGS_BYTE1',
        'unit': 'flags',
        'conversion': lambda x: f"0b{x:08b}",
        'description': 'DTC error flags byte 1 (R3=0-8)'
    },
    'XRAM_0x4F': {
        'name': 'DTC_FLAGS_BYTE2',
        'unit': 'flags',
        'conversion': lambda x: f"0b{x:08b}",
        'description': 'DTC error flags byte 2 (R3=9-16)'
    },
    'XRAM_0x50': {
        'name': 'DTC_FLAGS_BYTE3',
        'unit': 'flags',
        'conversion': lambda x: f"0b{x:08b}",
        'description': 'DTC error flags byte 3 (R3=17-23)'
    },

    # DTC counter bytes (written by error detection loop)
    'XRAM_0x51': {'name': 'DTC_COUNTER_R3_01', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=1'},
    'XRAM_0x52': {'name': 'DTC_COUNTER_R3_02', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=2'},
    'XRAM_0x53': {'name': 'DTC_COUNTER_R3_03', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=3'},
    'XRAM_0x54': {'name': 'DTC_COUNTER_R3_04_05', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=4,5'},
    'XRAM_0x55': {'name': 'DTC_COUNTER_R3_06_07', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=6,7'},
    'XRAM_0x56': {'name': 'DTC_COUNTER_R3_08_10', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=8,9,10'},
    'XRAM_0x57': {'name': 'DTC_COUNTER_R3_11_13_19', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=11,12,13,19'},
    'XRAM_0x58': {'name': 'DTC_COUNTER_R3_14_15', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=14,15'},
    'XRAM_0x59': {'name': 'DTC_COUNTER_R3_16', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=16'},
    'XRAM_0x5A': {'name': 'DTC_COUNTER_R3_17', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=17'},
    'XRAM_0x5B': {'name': 'DTC_COUNTER_R3_18', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=18'},
    'XRAM_0x5C': {'name': 'DTC_COUNTER_R3_20_21', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=20,21'},
    'XRAM_0x5D': {'name': 'DTC_COUNTER_R3_22', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Error counter for R3=22'},

    # Error register mirrors/state
    'XRAM_0x84': {'name': 'ERROR_STATE_R7', 'unit': 'flags', 'conversion': lambda x: f"0x{x:02X}", 'description': 'Previous error state for R7 (ANDed with RAM_27)'},
    'XRAM_0x85': {'name': 'ERROR_STATE_R6', 'unit': 'flags', 'conversion': lambda x: f"0x{x:02X}", 'description': 'Previous error state for R6 (ANDed with RAM_28)'},
    'XRAM_0x86': {'name': 'ERROR_STATE_R5', 'unit': 'flags', 'conversion': lambda x: f"0x{x:02X}", 'description': 'Previous error state for R5 (ANDed with RAM_29)'},

    # Threshold check gates/counters
    'XRAM_0x82': {'name': 'THRESHOLD_GATE_1', 'unit': 'bool', 'conversion': lambda x: "enabled" if x == 0 else "disabled", 'description': 'Gate for TPS/RPM/TEMP threshold check'},
    'XRAM_0x83': {'name': 'THRESHOLD_GATE_2', 'unit': 'bool', 'conversion': lambda x: "enabled" if x == 0 else "disabled", 'description': 'Gate for RAM_29.0 threshold check'},
    'XRAM_0x88': {'name': 'COOLANT_TEMP_TIMER', 'unit': 'count', 'conversion': lambda x: x, 'description': 'Countdown timer for coolant temp check'},

    # Adaptive parameters and reset state (discovered 2026-01-09)
    'XRAM_0x40': {'name': 'ADAPTIVE_PARAM_0', 'unit': 'raw', 'conversion': lambda x: x, 'description': 'First byte of 64-byte adaptive parameter block (0x40-0x7F), decays in timer ISR (K-Line ID 0x40 subcmd 0xF8)'},
    'XRAM_0xFF': {'name': 'RESET_STATE_FLAG', 'unit': 'state', 'conversion': lambda x: {0x80: 'Normal', 0xFF: 'Alternative', 0x01: 'Complete'}.get(x, f'Unknown(0x{x:02X})'), 'description': 'Reset state flag (0x80=normal copy, 0xFF=alt copy, 0x01=complete)'},
}


def get_sensor_info(address: str) -> Optional[Dict[str, Any]]:
    """
    Get sensor metadata for a RAM or XRAM address.

    Args:
        address: RAM address like "RAM_70" or "XRAM_0x4E"

    Returns:
        Dictionary with sensor info or None if not found
    """
    # Try RAM map first
    if address in SENSOR_MAP:
        return SENSOR_MAP[address]

    # Try XRAM map
    if address in XRAM_MAP:
        return XRAM_MAP[address]

    # Try with XRAM_ prefix
    if not address.startswith('XRAM_') and address.startswith('0x'):
        xram_key = f'XRAM_{address}'
        if xram_key in XRAM_MAP:
            return XRAM_MAP[xram_key]

    return None


def convert_threshold(address: str, raw_value: int) -> str:
    """
    Convert raw threshold value to human-readable format with units.

    Args:
        address: RAM/XRAM address like "RAM_70"
        raw_value: Raw ADC/sensor value

    Returns:
        Formatted string like "47 raw (15.04°)" or "80 raw (2000 RPM)"
    """
    sensor_info = get_sensor_info(address)

    if sensor_info is None:
        return f"{raw_value} raw (unknown sensor)"

    conversion_func = sensor_info.get('conversion')
    unit = sensor_info.get('unit', '')

    if conversion_func is None:
        return f"{raw_value} raw"

    try:
        converted = conversion_func(raw_value)
        if isinstance(converted, float):
            return f"{raw_value} raw ({converted:.2f} {unit})"
        else:
            return f"{raw_value} raw ({converted} {unit})"
    except Exception as e:
        return f"{raw_value} raw (conversion error: {e})"


def get_all_sensor_addresses() -> Dict[str, str]:
    """
    Get all known sensor RAM addresses mapped to sensor names.

    Returns:
        Dict of {address: sensor_name}
    """
    result = {}
    for addr, info in SENSOR_MAP.items():
        result[addr] = info['name']
    for addr, info in XRAM_MAP.items():
        result[addr] = info['name']
    return result


if __name__ == '__main__':
    # Test/demo
    print("Sensor Database Test")
    print("=" * 60)

    # Test RAM sensors
    test_cases = [
        ('RAM_70', 47),   # TPS
        ('RAM_47', 80),   # RPM
        ('RAM_63', 26),   # Coolant temp
        ('RAM_65', 200),  # Lambda
        ('RAM_69', 50),   # Ignition advance
        ('XRAM_0x4E', 0b11000000),  # DTC flags
    ]

    for addr, raw_val in test_cases:
        info = get_sensor_info(addr)
        if info:
            converted = convert_threshold(addr, raw_val)
            print(f"{addr:15s} ({info['name']:20s}): {converted}")
        else:
            print(f"{addr:15s}: NOT FOUND")

    print("\n" + "=" * 60)
    print(f"Total sensors mapped: {len(SENSOR_MAP)} RAM + {len(XRAM_MAP)} XRAM")
