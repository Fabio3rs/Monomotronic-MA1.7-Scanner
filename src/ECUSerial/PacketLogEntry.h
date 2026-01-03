#pragma once
#ifndef PACKET_LOG_ENTRY_H
#define PACKET_LOG_ENTRY_H

/*
 * PacketLogEntry - Enhanced logging structure for ECU communication
 *
 * NASA Power of 10 Compliant:
 * - Fixed-size structure (16 bytes)
 * - No dynamic allocation
 * - Packet-aware logging for protocol debugging
 *
 * Provides detailed packet tracking including:
 * - Packet counter correlation
 * - Field-level granularity (SIZE, COUNTER, DATA, END)
 * - ECU state at time of logging
 * - Error flag tracking
 */

#include <chrono>
#include <cstdint>
#include <cstdio>  // for snprintf
#include <cstring> // for strlen, strncat

/// Enhanced logging entry with packet tracking and error flags
struct PacketLogEntry {
    // ===== Timing (8 bytes) =====
    /// High-resolution timestamp
    std::chrono::high_resolution_clock::time_point time;

    // ===== Protocol Tracking (2 bytes) =====
    /// ECU packet counter (from protocol, 0-255)
    /// Links bytes to specific packets
    uint8_t packet_counter;

    /// Which field of the packet:
    /// 0 = Size byte
    /// 1 = Counter byte
    /// 2 = Frame Type ID
    /// 3-254 = Data[n-3] (e.g., 3 = Data[0], 4 = Data[1])
    /// 255 = End marker (0x03)
    uint8_t packet_field;

    // ===== Data & Direction (2 bytes) =====
    uint8_t byte_value; /// The actual byte transmitted/received
    uint8_t direction;  /// 0 = READ, 1 = WRITE

    // ===== State & Error Tracking (2 bytes) =====
    /// Current ECU state when byte was logged
    /// Maps to ECUState enum values (0-6)
    uint8_t ecu_state;

    /// Error flags (bitfield):
    /// Bit 0: Checksum mismatch
    /// Bit 1: Timeout occurred
    /// Bit 2: Unexpected value
    /// Bit 3: State machine error
    /// Bit 4-7: Reserved
    uint8_t error_flags;

    // ===== Padding (2 bytes) =====
    uint16_t padding; /// Align to 16 bytes

    // Total: 16 bytes

    // ===== Constructors =====

    /// Default constructor
    PacketLogEntry()
        : time(), packet_counter(0), packet_field(0), byte_value(0),
          direction(0), ecu_state(0), error_flags(0), padding(0) {}

    /// Full constructor
    PacketLogEntry(std::chrono::high_resolution_clock::time_point t,
                   uint8_t pkt_ctr, uint8_t pkt_field, uint8_t byte_val,
                   uint8_t dir, uint8_t state, uint8_t err_flags = 0)
        : time(t), packet_counter(pkt_ctr), packet_field(pkt_field),
          byte_value(byte_val), direction(dir), ecu_state(state),
          error_flags(err_flags), padding(0) {}

    /// Legacy constructor (for backward compatibility during transition)
    PacketLogEntry(int act, int byte_val)
        : time(std::chrono::high_resolution_clock::now()), packet_counter(0),
          packet_field(0), byte_value(static_cast<uint8_t>(byte_val)),
          direction(static_cast<uint8_t>(act)), ecu_state(0), error_flags(0),
          padding(0) {}
};

// ===== Error Flag Constants =====

namespace PacketLogError {
constexpr uint8_t NONE = 0x00;
constexpr uint8_t CHECKSUM_MISMATCH = 0x01;
constexpr uint8_t TIMEOUT = 0x02;
constexpr uint8_t UNEXPECTED_VALUE = 0x04;
constexpr uint8_t STATE_ERROR = 0x08;
} // namespace PacketLogError

// ===== Packet Field Constants =====

namespace PacketField {
constexpr uint8_t SIZE = 0;
constexpr uint8_t COUNTER = 1;
constexpr uint8_t FRAME_TYPE = 2;
constexpr uint8_t DATA_START = 3; // Data[0] = 3, Data[1] = 4, etc.
constexpr uint8_t END_MARKER = 255;
constexpr uint8_t UNKNOWN = 254; // Used when field is not tracked
} // namespace PacketField

// ===== Direction Constants =====

namespace LogDirection {
constexpr uint8_t READ = 0;
constexpr uint8_t WRITE = 1;
} // namespace LogDirection

// ===== Helper Functions =====

/// Get human-readable field name
inline const char *getPacketFieldName(uint8_t field) {
    switch (field) {
    case PacketField::SIZE:
        return "SIZE";
    case PacketField::COUNTER:
        return "COUNTER";
    case PacketField::FRAME_TYPE:
        return "FRAME_TYPE";
    case PacketField::END_MARKER:
        return "END";
    case PacketField::UNKNOWN:
        return "UNKNOWN";
    default:
        if (field >= PacketField::DATA_START && field < PacketField::UNKNOWN) {
            static char buf[16];
            snprintf(buf, sizeof(buf), "DATA[%d]",
                     field - PacketField::DATA_START);
            return buf;
        }
        return "INVALID";
    }
}

/// Get human-readable direction
inline const char *getDirectionName(uint8_t direction) {
    return (direction == LogDirection::READ) ? "READ" : "WRITE";
}

/// Get human-readable error flags
inline void getErrorFlagsString(uint8_t flags, char *buf, size_t bufsize) {
    if (flags == PacketLogError::NONE) {
        snprintf(buf, bufsize, "-");
        return;
    }

    buf[0] = '\0';
    bool first = true;

    if (flags & PacketLogError::CHECKSUM_MISMATCH) {
        if (!first)
            strncat(buf, "|", bufsize - strlen(buf) - 1);
        strncat(buf, "CHECKSUM", bufsize - strlen(buf) - 1);
        first = false;
    }
    if (flags & PacketLogError::TIMEOUT) {
        if (!first)
            strncat(buf, "|", bufsize - strlen(buf) - 1);
        strncat(buf, "TIMEOUT", bufsize - strlen(buf) - 1);
        first = false;
    }
    if (flags & PacketLogError::UNEXPECTED_VALUE) {
        if (!first)
            strncat(buf, "|", bufsize - strlen(buf) - 1);
        strncat(buf, "UNEXPECTED", bufsize - strlen(buf) - 1);
        first = false;
    }
    if (flags & PacketLogError::STATE_ERROR) {
        if (!first)
            strncat(buf, "|", bufsize - strlen(buf) - 1);
        strncat(buf, "STATE_ERR", bufsize - strlen(buf) - 1);
        first = false;
    }
}

#endif // PACKET_LOG_ENTRY_H
