#pragma once
#ifndef ECUMONOMOTRONIC_H
#define ECUMONOMOTRONIC_H

/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

MIT License

Copyright (c) 2019 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "CircleMTIO.hpp"
#include "ECUHealthMetrics.h"
#include "PacketLogEntry.h"
#include "SerialPort.h"
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

typedef std::optional<uint8_t> ECUByte;

struct ECUmmpacket {
    static constexpr size_t MAX_DATA_SIZE =
        252; // Protocol limit (255 - 3 header bytes)

    uint8_t size;
    uint8_t counter;
    uint8_t frametypeid;

    std::array<uint8_t, MAX_DATA_SIZE> data; // Fixed-size array (NASA P10)
    uint8_t data_length;                     // Actual used length

    uint8_t end;

    ECUmmpacket &operator=(ECUmmpacket &&) = default;
    ECUmmpacket &operator=(const ECUmmpacket &) = default;
    ECUmmpacket(ECUmmpacket &&) = default;
    ECUmmpacket(const ECUmmpacket &) = default;

    ECUmmpacket()
        : size(3), counter(0), frametypeid(0), data{}, data_length(0),
          end(0x03) {}

    // Helper to get valid data span (C++20)
    std::span<const uint8_t> get_data() const {
        return std::span<const uint8_t>(data.data(), data_length);
    }

    std::span<uint8_t> get_data_mutable() {
        return std::span<uint8_t>(data.data(), data_length);
    }
};

// NASA P10: Fixed-size response collection (replaces std::deque returns)
struct ECUResponseCollection {
    static constexpr size_t MAX_RESPONSE_PACKETS = 10;

    std::array<ECUmmpacket, MAX_RESPONSE_PACKETS> packets;
    size_t count{0};

    // Helper to get valid packets span
    std::span<const ECUmmpacket> get_packets() const {
        return std::span<const ECUmmpacket>(packets.data(), count);
    }

    std::span<ECUmmpacket> get_packets_mutable() {
        return std::span<ECUmmpacket>(packets.data(), count);
    }

    // Add packet with bounds checking
    bool add(ECUmmpacket &&pkt) {
        if (count < MAX_RESPONSE_PACKETS) {
            packets[count++] = std::move(pkt);
            return true;
        }
        return false;
    }

    void clear() { count = 0; }
};

class ECUMonomotronic {
    SerialPort sp;
    std::thread ECUThreadObj;
    std::atomic<bool> ECUThreadRunning;
    std::atomic<bool> ECUThreadExit{false};
    std::atomic<int> ECUThreadState{0};
    std::atomic<int> ECUThreadErr{0};
    std::atomic<bool> ECUInited{false};
    std::atomic<bool> KeepECUConnectionAlive;
    uint8_t ECUPacketCounter{};

    bool enableLog{true};

    std::atomic<bool> ECUConnectedNow;
    std::atomic<bool> ECUThreadShouldProceed;

    //						send commands variables
    std::atomic<bool> ECUThreadCanAcceptCommands;
    std::mutex ECUNewCommandMutex;
    ECUmmpacket ECUNewCommandTemp;
    std::atomic<bool> ECUNewCommandAvailable;

    std::atomic<bool> ECUCommandResultAvailable;
    ECUmmpacket ECUResponse;

    // NASA P10: Fixed-size array instead of deque (max 10 init packets
    // observed)
    static constexpr size_t MAX_INIT_PACKETS = 10;
    std::array<ECUmmpacket, MAX_INIT_PACKETS> initPackets;
    size_t initPackets_count{0};

    // ===== Packet field tracking for logging =====
    uint8_t current_write_packet_field{PacketField::UNKNOWN};
    uint8_t current_read_packet_field{PacketField::UNKNOWN};

    // ===== Telemetry counters (atomics, no post-init allocation) =====
    std::atomic<uint64_t> telemetry_ack_mismatch{0};
    std::atomic<uint64_t> telemetry_end_marker_error{0};
    std::atomic<uint64_t> telemetry_retry_events{0};
    std::chrono::steady_clock::time_point steady_start_time;
    std::chrono::system_clock::time_point system_start_time;
    std::chrono::steady_clock::time_point recovery_guard_until;

    // ===== NASA P10 Health Metrics (Fixed-size, atomic counters) =====
    ECU::HealthMetrics health_metrics;

    // ===== Logging helper methods =====
    void addbytelog(const PacketLogEntry &entry);

    void resetWritePacketFieldTracking() {
        current_write_packet_field = PacketField::SIZE;
    }

    void resetReadPacketFieldTracking() {
        current_read_packet_field = PacketField::SIZE;
    }

    void advanceWritePacketField(bool is_end_marker = false) {
        if (is_end_marker) {
            current_write_packet_field = PacketField::END_MARKER;
        } else if (current_write_packet_field < PacketField::END_MARKER) {
            current_write_packet_field++;
        }
    }

    void advanceReadPacketField(bool is_end_marker = false) {
        if (is_end_marker) {
            current_read_packet_field = PacketField::END_MARKER;
        } else if (current_read_packet_field < PacketField::END_MARKER) {
            current_read_packet_field++;
        }
    }

    void validatePacketField(uint8_t expected, bool is_read) {
        uint8_t current =
            is_read ? current_read_packet_field : current_write_packet_field;

        if (current != expected && enableLog) {
            // Log error flag
            PacketLogEntry error_entry(
                std::chrono::high_resolution_clock::now(), ECUPacketCounter,
                current, 0x00,
                is_read ? LogDirection::READ : LogDirection::WRITE,
                static_cast<uint8_t>(ECUThreadState.load()),
                PacketLogError::UNEXPECTED_VALUE); // Field mismatch
            addbytelog(error_entry);
        }
    }

    void ECUOpenThread();
    void sendInitSequence();
    static void updatePacketCounter(ECUMonomotronic &mm, const ECUmmpacket &p);
    static void
    processECURequest(ECUMonomotronic &mm,
                      std::chrono::steady_clock::time_point &lastpackettime,
                      bool &nError);
    static void ECUThreadFun(ECUMonomotronic &mm);

    bool ECUWrite(uint8_t b);
    ECUByte ECURead(int timeout = 1000);
    ECUByte ECUReadResponse();
    bool ECUWriteResponse(uint8_t b);
    void
    ECUReadSequential(std::span<uint8_t> buffer); // NASA P10: fill in-place
    bool ECUWriteSequential(std::span<const uint8_t> data);
    std::optional<ECUmmpacket> ECUReadPacket();
    bool
    ECUWritePacket(uint8_t frameid,
                   const std::vector<uint8_t> &data = std::vector<uint8_t>());
    bool
    ECUWritePacket(uint8_t frameid,
                   std::span<const uint8_t> data); // NASA P10: span overload

  private:
    void
    extractLogsToStream(std::ostream &fs); // Helper for CircleMTIO extraction
    void extractLogsToString(
        std::string &str); // Helper for string extraction (no stringstream)

    const char *debug_file;
    int debug_line;

    void debug_regiter_err(const char *file, int line);

    std::optional<ECUResponseCollection>
    ECURequestData(uint8_t frameid, uint8_t eECUFrameID,
                   const std::vector<uint8_t> &data = std::vector<uint8_t>());

    void resetToNCKnownState();
    void signalRecovery();

  public:
    // Snapshot struct exposed to callers
    struct TelemetrySnapshot {
        uint64_t ack_mismatch;
        uint64_t end_marker_error;
        uint64_t retry_events;
    };

    TelemetrySnapshot getTelemetrySnapshot() const {
        return TelemetrySnapshot{
            telemetry_ack_mismatch.load(std::memory_order_relaxed),
            telemetry_end_marker_error.load(std::memory_order_relaxed),
            telemetry_retry_events.load(std::memory_order_relaxed)};
    }

    /**
     * @brief Get snapshot of all health metrics
     * @return ECU::HealthMetricsSnapshot Non-atomic copy for safe reading
     *
     * Thread-safe: Can be called from any thread while worker updates metrics.
     * Metrics include packet counters, latency, errors, buffer overruns.
     */
    ECU::HealthMetricsSnapshot getHealthSnapshot() const {
        return health_metrics.snapshot();
    }

    /**
     * @brief Check if ECU communication is healthy
     * @return bool True if no errors, recent communication, low error rate
     *
     * Criteria: No consecutive errors, communication <5s old, error rate <1%
     */
    bool isHealthy() const { return health_metrics.is_healthy(); }

    // Init received packages/ECU identification
    // NASA P10: Returns span to valid portion of fixed array
    std::span<const ECUmmpacket> getinitPackets() const {
        return std::span<const ECUmmpacket>(initPackets.data(),
                                            initPackets_count);
    }

    // Custom commands
    // See ECU_FRAMES_ID
    std::optional<ECUmmpacket> getECUResponse();
    bool
    sendECURequest(uint8_t frameid,
                   const std::vector<uint8_t> &data = std::vector<uint8_t>());

    // Wrapper to commands
    std::optional<ECUResponseCollection> ECUReadErrors();
    std::optional<ECUResponseCollection>
    readECUMemory(uint8_t addressHigh, uint8_t addressLow, uint8_t length);
    std::optional<ECUResponseCollection> requestSensorCollection();
    std::optional<ECUmmpacket> ECUCleanErrors();

    // Source: http://www.nailed-barnacle.co.uk/coupe/startrek/startrek.html
    enum ECU_FRAMES_ID {
        ECU_DATA_MEMORY_READ = 0x01,
        ECU_REQ_ACTUATOR = 0x04,
        ECU_CLEAR_ERRORS_CODE = 0x05,
        ECU_REQ_DIAGNOSIS_END = 0x06,
        ECU_READ_ERRORS_CODE = 0x07,
        ECU_ACK_CODE = 0x09,
        ECU_NOT_ACK_CODE = 0x0A,
        ECU_REQ_SENSOR_COLLECTION = 0x12,
        ECU_RESP_SENSOR_COLLECTION = 0xF4,
        ECU_INIT_STRING = 0xF6,
        ECU_REQUEST_ADC_CODE = 0xFB,
        ECU_ERROR_DATA_CODE = 0xFC,
        ECU_READ_DATA_CODE = 0xFE
    };

    // Error codes to description
    static std::string_view errorPacketToString(const ECUmmpacket &p,
                                                bool &present);

    bool canAcceptCommands() const { return ECUThreadCanAcceptCommands; }
    bool isThreadRunning() const { return ECUThreadRunning; }
    bool portIsOpen() const noexcept { return sp.isConnected(); };

    bool isECUConnectedNow() const { return ECUConnectedNow; }
    void shouldTryAutoConnect(bool b) { ECUThreadShouldProceed = b; };

    int getECUThreadError() const noexcept { return ECUThreadErr; }

    // Interface controls
    void init();
    void forcestop();
    void stop();

    // Debugging
    void debugTofile();
    void purgeSerial();

    // Logging access
    void printlogging();
    void fprintlogging(std::ostream &fs);
    void strprintlogging(std::string &str);
    void printPacketSummary(std::ostream &fs);

    // Public access to logging buffer for advanced features
    CircleMTIO<20480, PacketLogEntry> bytesLogging;

    ECUMonomotronic(const char *port, bool enableLogging = true) noexcept;
    ~ECUMonomotronic();
};

#endif
