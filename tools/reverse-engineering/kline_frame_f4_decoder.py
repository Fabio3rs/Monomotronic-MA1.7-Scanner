#!/usr/bin/env python3
"""
K-Line Frame F4 Decoder - Capabilities Tables
Decodes 10-byte F4 frame payload using correct table based on RAM_B3.

Usage:
    python3 kline_frame_f4_decoder.py

    # Or import and use programmatically:
    from kline_frame_f4_decoder import decode_f4_frame
    result = decode_f4_frame(ram_b3_value=0x01, payload=[0x51, 0x60, ...])

References:
    - kline_capabilities_tables_analysis.md
    - sensor_database.py
"""

from typing import List, Dict, Any, Tuple
from sensor_database import SENSOR_MAP, XRAM_MAP

# ============================================================================
# K-LINE CAPABILITIES TABLE DEFINITIONS
# ============================================================================

# Table 1: Advanced Mode (RAM_B3.bit1 = 1) at ROM 0x6E78
# Selected when: RAM_2E.0 set + RPM condition met
KLINE_TABLE_1 = [
    (0x63, 0x00),  # Pos 0: COOLANT_TEMP (RAM_63)
    (0x66, 0x00),  # Pos 1: THROTTLE_PERCENT_TRACK1 (RAM_66)
    (0x47, 0x00),  # Pos 2: ENGINE_RPM (RAM_47)
    (0x62, 0x00),  # Pos 3: AIR_INTAKE_TEMP (RAM_62)
    (0x0C, 0xF8),  # Pos 4: LAMBDA_INTEGRATOR (RAM_0C, diagnostic mode)
    (0x6C, 0xF8),  # Pos 5: MAP_ADAPTATION (RAM_6C, diagnostic mode)
    (0x6F, 0xF8),  # Pos 6: MLLECK_COEFFICIENT (RAM_6F, diagnostic mode)
    (0x3D, 0xF8),  # Pos 7: IDLE_RPM_REGULATOR (RAM_3D, diagnostic mode)
    (0x61, 0x00),  # Pos 8: BATTERY_VOLTAGE_RAW (RAM_61) ← NEW DISCOVERY
    (0x69, 0xF8),  # Pos 9: FTEAD_ADAPTION (RAM_69, diagnostic mode)
]

# Table 2: Standard Mode (RAM_B3.bit0 = 1) at ROM 0x6E8C
# Selected when: RAM_2E.1 set during handshake
KLINE_TABLE_2 = [
    (0x63, 0x00),  # Pos 0: COOLANT_TEMP (RAM_63)
    (0x66, 0x00),  # Pos 1: THROTTLE_PERCENT_TRACK1 (RAM_66)
    (0x70, 0x00),  # Pos 2: THROTTLE_ANGLE1 (RAM_70)
    (0x2F, 0x00),  # Pos 3: FUEL_MODE_FLAGS (RAM_2F) ← Flags, not sensor
    (0x72, 0xF8),  # Pos 4: TS_COEFFICIENT (RAM_72, diagnostic mode)
    (0x6C, 0xF8),  # Pos 5: MAP_ADAPTATION (RAM_6C, diagnostic mode)
    (0x6F, 0xF8),  # Pos 6: MLLECK_COEFFICIENT (RAM_6F, diagnostic mode)
    (0x40, 0xF8),  # Pos 7: ADAPTIVE_PARAM_0 (XRAM_0x40) ← NEW DISCOVERY
    (0x61, 0x00),  # Pos 8: BATTERY_VOLTAGE_RAW (RAM_61) ← NEW DISCOVERY
    (0x69, 0xF8),  # Pos 9: FTEAD_ADAPTION (RAM_69, diagnostic mode)
]

# ============================================================================
# DECODER FUNCTIONS
# ============================================================================

def decode_f4_frame(ram_b3_value: int, payload: List[int]) -> Dict[str, Any]:
    """
    Decode K-Line Frame F4 (10-byte sensor payload) based on RAM_B3 session flags.

    Args:
        ram_b3_value: Value of RAM_B3 (determines which table to use)
                     bit0=1 → Table 2 (Standard Mode)
                     bit1=1 → Table 1 (Advanced Mode)
        payload: List of 10 bytes from F4 frame

    Returns:
        Dictionary with:
            - 'table': Name of selected table
            - 'ram_b3': Formatted RAM_B3 value
            - 'ram_b3_binary': Binary representation
            - 'sensors': List of decoded sensor readings
            - 'error': Error message (if applicable)

    Example:
        >>> decode_f4_frame(0x01, [0x51, 0x60, 0x0D, 0x9A, 0x00, 0x00, 0x00, 0x80, 0x83, 0x00])
        {
            'table': 'Table 2 (0x6E8C) - Standard Mode',
            'ram_b3': '0x01',
            'ram_b3_binary': '0b00000001',
            'sensors': [...]
        }
    """
    # Validate payload length
    if len(payload) != 10:
        return {
            'error': f"Invalid payload length: {len(payload)} (expected 10 bytes)",
            'ram_b3': f"0x{ram_b3_value:02X}",
            'table': None,
            'sensors': []
        }

    # Select table based on RAM_B3 bits
    table: List[Tuple[int, int]]
    table_name: str

    if ram_b3_value & 0x01:  # Bit 0 set
        table = KLINE_TABLE_2
        table_name = "Table 2 (0x6E8C) - Standard Mode"
    elif ram_b3_value & 0x02:  # Bit 1 set
        table = KLINE_TABLE_1
        table_name = "Table 1 (0x6E78) - Advanced Mode"
    else:
        # No table selected (both bits clear)
        return {
            'error': 'No table selected (RAM_B3 bits 0 and 1 both clear)',
            'ram_b3': f"0x{ram_b3_value:02X}",
            'ram_b3_binary': f"0b{ram_b3_value:08b}",
            'table': 'None (zeros payload)',
            'sensors': [{'position': i, 'name': 'ZERO', 'value': 0, 'raw': 0, 'unit': '', 'pid': '0x00', 'subcmd': '0x00'} for i in range(10)]
        }

    # Decode each position using selected table
    sensors = []
    for i, (pid, subcmd) in enumerate(table):
        raw_value = payload[i]

        # Find sensor in database
        # Note: Some sensors have different meanings in normal vs diagnostic mode
        sensor_key_options = []
        if subcmd == 0x00:
            # Normal mode: Try _NORMAL variant first, then plain RAM
            sensor_key_options = [
                f"RAM_{pid:02X}_NORMAL",    # Try normal mode variant
                f"RAM_{pid:02X}",           # Try plain RAM
            ]
        else:
            # Diagnostic mode (0xF8): Try _DIAG variant first, then RAM, then XRAM
            sensor_key_options = [
                f"RAM_{pid:02X}_DIAG",      # Try diagnostic variant first
                f"RAM_{pid:02X}",           # Try plain RAM
                f"XRAM_0x{pid:02X}",        # Try XRAM
            ]

        # Search for sensor in database
        sensor_info = None
        for key in sensor_key_options:
            sensor_info = SENSOR_MAP.get(key) or XRAM_MAP.get(key)
            if sensor_info:
                break

        if sensor_info:
            # Apply conversion
            try:
                converted = sensor_info['conversion'](raw_value)
                sensors.append({
                    'position': i,
                    'pid': f"0x{pid:02X}",
                    'subcmd': f"0x{subcmd:02X}",
                    'name': sensor_info['name'],
                    'raw': raw_value,
                    'value': converted,
                    'unit': sensor_info['unit'],
                    'description': sensor_info.get('description', '')
                })
            except Exception as e:
                sensors.append({
                    'position': i,
                    'pid': f"0x{pid:02X}",
                    'subcmd': f"0x{subcmd:02X}",
                    'name': sensor_info['name'],
                    'raw': raw_value,
                    'value': raw_value,
                    'unit': '?',
                    'error': f"Conversion failed: {e}"
                })
        else:
            # Unknown sensor (not in database)
            sensors.append({
                'position': i,
                'pid': f"0x{pid:02X}",
                'subcmd': f"0x{subcmd:02X}",
                'name': 'UNMAPPED',
                'raw': raw_value,
                'value': raw_value,
                'unit': '?',
                'description': f'Unknown sensor at {sensor_key}'
            })

    return {
        'table': table_name,
        'ram_b3': f"0x{ram_b3_value:02X}",
        'ram_b3_binary': f"0b{ram_b3_value:08b}",
        'sensors': sensors
    }


def format_decoded_frame(decoded: Dict[str, Any], verbose: bool = False) -> str:
    """
    Format decoded frame as human-readable text.

    Args:
        decoded: Result from decode_f4_frame()
        verbose: Include descriptions and additional details

    Returns:
        Formatted string
    """
    lines = []

    # Header
    lines.append("=" * 80)
    lines.append("K-LINE FRAME F4 DECODER")
    lines.append("=" * 80)

    # Session info
    lines.append(f"Session State: RAM_B3 = {decoded['ram_b3']} ({decoded['ram_b3_binary']})")
    lines.append(f"Selected Table: {decoded['table']}")

    # Error handling
    if 'error' in decoded:
        lines.append(f"\n⚠️  ERROR: {decoded['error']}")

    lines.append("-" * 80)

    # Sensor readings
    for sensor in decoded['sensors']:
        # Format value
        if isinstance(sensor['value'], float):
            value_str = f"{sensor['value']:>10.2f}"
        elif isinstance(sensor['value'], int):
            value_str = f"{sensor['value']:>10d}"
        else:
            value_str = f"{str(sensor['value']):>10s}"

        # Basic line
        line = (
            f"Pos {sensor['position']}: "
            f"{sensor['name']:25s} = "
            f"{value_str} {sensor['unit']:8s} "
            f"(raw: 0x{sensor['raw']:02X})"
        )
        lines.append(line)

        # Verbose details
        if verbose and 'description' in sensor and sensor['description']:
            lines.append(f"         └─ {sensor['description']}")

        if 'error' in sensor:
            lines.append(f"         └─ ⚠️  {sensor['error']}")

    lines.append("=" * 80)

    return "\n".join(lines)


# ============================================================================
# CLI INTERFACE
# ============================================================================

def main():
    """Command-line interface for testing decoder."""
    import sys

    print("K-Line Frame F4 Decoder - Capabilities Tables")
    print("=" * 80)

    # Real captured frame from user analysis
    print("\n[TEST 1] Real Captured Frame (2026-01-09)")
    print("-" * 80)
    ram_b3 = 0x01
    payload = [0x51, 0x60, 0x0D, 0x9A, 0x00, 0x00, 0x00, 0x80, 0x83, 0x00]

    print(f"Input: RAM_B3 = 0x{ram_b3:02X}")
    print(f"Payload: {' '.join(f'{b:02X}' for b in payload)}")
    print()

    decoded = decode_f4_frame(ram_b3, payload)
    print(format_decoded_frame(decoded, verbose=False))

    # Test case 2: Table 1 (bit1 set)
    print("\n\n[TEST 2] Simulated Frame - Table 1 (Advanced Mode)")
    print("-" * 80)
    ram_b3 = 0x02  # Bit 1 set
    payload = [0x50, 0x60, 0x50, 0x1A, 0x80, 0x00, 0x00, 0x05, 0x85, 0x00]

    print(f"Input: RAM_B3 = 0x{ram_b3:02X}")
    print(f"Payload: {' '.join(f'{b:02X}' for b in payload)}")
    print()

    decoded = decode_f4_frame(ram_b3, payload)
    print(format_decoded_frame(decoded, verbose=False))

    # Test case 3: No table selected
    print("\n\n[TEST 3] No Table Selected (B3 = 0x00)")
    print("-" * 80)
    ram_b3 = 0x00  # Both bits clear
    payload = [0x00] * 10

    print(f"Input: RAM_B3 = 0x{ram_b3:02X}")
    print(f"Payload: {' '.join(f'{b:02X}' for b in payload)}")
    print()

    decoded = decode_f4_frame(ram_b3, payload)
    print(format_decoded_frame(decoded, verbose=False))

    # Interactive mode
    if len(sys.argv) > 1 and sys.argv[1] == '--interactive':
        print("\n\n[INTERACTIVE MODE]")
        print("=" * 80)

        try:
            ram_b3_input = input("Enter RAM_B3 value (hex, e.g., 0x01 or 01): ")
            ram_b3 = int(ram_b3_input.replace('0x', ''), 16)

            payload_input = input("Enter 10-byte payload (hex, space-separated): ")
            payload = [int(x, 16) for x in payload_input.split()]

            if len(payload) != 10:
                print(f"ERROR: Expected 10 bytes, got {len(payload)}")
                return

            decoded = decode_f4_frame(ram_b3, payload)
            print("\n" + format_decoded_frame(decoded, verbose=True))

        except KeyboardInterrupt:
            print("\n\nInterrupted by user.")
        except Exception as e:
            print(f"\nERROR: {e}")


if __name__ == '__main__':
    main()
