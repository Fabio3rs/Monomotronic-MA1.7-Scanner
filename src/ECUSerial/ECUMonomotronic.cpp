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
#include "ECUMonomotronic.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <linux/serial.h>
#include <map>
#include <optional>
#include <stdio.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace {
constexpr auto kCommandTimeout = std::chrono::milliseconds(1800);
constexpr auto kRecoveryBackoff = std::chrono::milliseconds(2600);
constexpr int kMaxConsecutiveTimeouts = 3;

enum class ThreadState : int { Handshake = 0, Ready = 1, Recovering = 2 };
} // namespace

void ECUMonomotronic::updatePacketCounter(ECUMonomotronic &mm,
                                          const ECUmmpacket &p) {
    if (p.counter > mm.ECUPacketCounter)
        mm.ECUPacketCounter = p.counter;
}

std::string
GetDateAndTime(const std::chrono::high_resolution_clock::time_point &th) {
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            th.time_since_epoch());

    std::chrono::seconds s =
        std::chrono::duration_cast<std::chrono::seconds>(th.time_since_epoch());
    std::time_t tt = s.count();
    std::size_t fractional_ms = ms.count() % 1000;

    // Format: HH:MM:SS.mmm DD/MM/YY (compact format)
    std::tm *local_tm = std::localtime(&tt);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03zu %02d/%02d/%02d",
                  local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec,
                  fractional_ms, local_tm->tm_mday, local_tm->tm_mon + 1,
                  local_tm->tm_year % 100);

    return std::string(buffer);
}

// ===== NASA P10: Enhanced logging with CircleMTIO =====

void ECUMonomotronic::addbytelog(const PacketLogEntry &entry) {
    if (!enableLog)
        return;

    auto [ptr, idx] = bytesLogging.new_write();
    if (ptr == nullptr) {
        // Buffer full - circular overwrite will occur
        // Overrun is tracked by CircleMTIO internally
        return;
    }

    *ptr = entry;
    bytesLogging.set_ready(idx);
}

void ECUMonomotronic::extractLogsToStream(std::ostream &fs) {
    char error_buf[64];
    uint8_t last_packet_counter = 0xFF; // Invalid initial value

    while (true) {
        auto [ptr, idx] = bytesLogging.next();
        if (ptr == nullptr)
            break; // No more entries

        // Insert separator when packet changes
        if (ptr->packet_counter != last_packet_counter) {
            if (last_packet_counter != 0xFF) {
                fs << "---\n"; // Visual packet boundary
            }
            last_packet_counter = ptr->packet_counter;
        }

        // Format: [Time] [Pkt:Field] [Dir] [State] [Byte] [Errors]
        fs << GetDateAndTime(ptr->time) << " ";
        fs << "[PKT:" << std::setfill('0') << std::setw(3)
           << static_cast<int>(ptr->packet_counter) << "] ";
        fs << "[" << std::setw(10) << std::left
           << getPacketFieldName(ptr->packet_field) << "] ";
        fs << "[" << std::setw(5) << getDirectionName(ptr->direction) << "] ";
        fs << "[State:" << static_cast<int>(ptr->ecu_state) << "] ";
        fs << "0x" << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(ptr->byte_value);

        // Add ASCII if printable
        if (isalnum(ptr->byte_value) || isspace(ptr->byte_value)) {
            fs << " '" << static_cast<char>(ptr->byte_value) << "'";
        }

        // Add error flags if present
        if (ptr->error_flags != PacketLogError::NONE) {
            getErrorFlagsString(ptr->error_flags, error_buf, sizeof(error_buf));
            fs << " [ERR:" << error_buf << "]";
        }

        fs << "\n";

        bytesLogging.set_free(idx); // Mark as consumed
    }

    // Report overrun statistics
    uint64_t overruns = bytesLogging.get_overruns();
    if (overruns > 0) {
        fs << "\n*** WARNING: " << overruns
           << " log entries were lost due to buffer overrun ***\n";
    }
}

void ECUMonomotronic::printlogging() { extractLogsToStream(std::cout); }

void ECUMonomotronic::fprintlogging(std::ostream &fs) {
    extractLogsToStream(fs);
}

void ECUMonomotronic::extractLogsToString(std::string &str) {
    char line_buf[256]; // Fixed-size buffer for each log line (NASA P10
                        // compliant)
    char error_buf[64];

    while (true) {
        auto [ptr, idx] = bytesLogging.next();
        if (ptr == nullptr)
            break; // No more entries

        // Format timestamp
        std::string timestamp = GetDateAndTime(ptr->time);

        // Format packet info
        int written = snprintf(
            line_buf, sizeof(line_buf),
            "%s [PKT:%03d] [%-10s] [%-5s] [State:%d] 0x%02X", timestamp.c_str(),
            static_cast<int>(ptr->packet_counter),
            getPacketFieldName(ptr->packet_field),
            getDirectionName(ptr->direction), static_cast<int>(ptr->ecu_state),
            static_cast<int>(ptr->byte_value));

        if (written > 0 && written < static_cast<int>(sizeof(line_buf))) {
            str += line_buf;

            // Add ASCII if printable
            if (isalnum(ptr->byte_value) || isspace(ptr->byte_value)) {
                snprintf(line_buf, sizeof(line_buf), " '%c'",
                         static_cast<char>(ptr->byte_value));
                str += line_buf;
            }

            // Add error flags if present
            if (ptr->error_flags != PacketLogError::NONE) {
                getErrorFlagsString(ptr->error_flags, error_buf,
                                    sizeof(error_buf));
                snprintf(line_buf, sizeof(line_buf), " [ERR:%s]", error_buf);
                str += line_buf;
            }

            str += '\n';
        }

        bytesLogging.set_free(idx); // Mark as consumed
    }

    // Report overrun statistics
    uint64_t overruns = bytesLogging.get_overruns();
    if (overruns > 0) {
        snprintf(line_buf, sizeof(line_buf),
                 "\n*** WARNING: %lu log entries were lost due to buffer "
                 "overrun ***\n",
                 static_cast<unsigned long>(overruns));
        str += line_buf;
    }
}

void ECUMonomotronic::strprintlogging(std::string &str) {
    extractLogsToString(str);
}

void ECUMonomotronic::printPacketSummary(std::ostream &fs) {
    struct PacketData {
        uint8_t counter;
        uint8_t size{0};
        uint8_t frame_type{0};
        std::vector<uint8_t> data;
        bool has_end_marker{false};
        uint8_t direction{LogDirection::READ};
    };

    std::map<uint8_t, PacketData> packets;

    // Reconstruct packets from byte-level logs
    while (true) {
        auto [ptr, idx] = bytesLogging.next();
        if (ptr == nullptr)
            break;

        auto &pkt = packets[ptr->packet_counter];
        pkt.counter = ptr->packet_counter;

        switch (ptr->packet_field) {
        case PacketField::SIZE:
            pkt.size = ptr->byte_value;
            pkt.direction = ptr->direction;
            break;
        case PacketField::FRAME_TYPE:
            pkt.frame_type = ptr->byte_value;
            break;
        case PacketField::END_MARKER:
            pkt.has_end_marker = (ptr->byte_value == 0x03);
            break;
        default:
            if (ptr->packet_field >= PacketField::DATA_START &&
                ptr->packet_field < PacketField::END_MARKER) {
                pkt.data.push_back(ptr->byte_value);
            }
            break;
        }

        bytesLogging.set_free(idx); // Mark as consumed
    }

    // Print summary
    fs << "\n=== PACKET SUMMARY ===\n";
    for (const auto &[counter, pkt] : packets) {
        fs << "PKT:0x" << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(counter);
        fs << " " << (pkt.direction == LogDirection::READ ? "RX" : "TX");
        fs << " Size:" << std::dec << static_cast<int>(pkt.size);
        fs << " Frame:0x" << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<int>(pkt.frame_type);
        fs << " Data:[";
        for (size_t i = 0; i < pkt.data.size(); ++i) {
            if (i > 0)
                fs << " ";
            fs << "0x" << std::hex << std::setfill('0') << std::setw(2)
               << static_cast<int>(pkt.data[i]);
        }
        fs << "]";
        fs << " End:" << (pkt.has_end_marker ? "OK" : "MISSING");
        fs << "\n";
    }
}

void ECUMonomotronic::debug_regiter_err(const char *file, int line) {
    debug_file = file;
    debug_line = line;
}

// TODO: Tweak timings

std::optional<ECUResponseCollection>
ECUMonomotronic::ECURequestData(uint8_t frameid, uint8_t eECUFrameID,
                                const std::vector<uint8_t> &data) {
    if (!sendECURequest(frameid, data)) {
        return std::nullopt;
    }

    ECUResponseCollection ecup;

    uint8_t frametypeidtmp = 0;

    const auto deadline = std::chrono::steady_clock::now() + kCommandTimeout;

    auto wait_for_response = [&](std::optional<ECUmmpacket> &response) {
        while (std::chrono::steady_clock::now() < deadline && !ECUThreadExit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            response = getECUResponse();
            if (response.has_value()) {
                return true;
            }
        }

        health_metrics.consecutive_timeout_errors.fetch_add(
            1, std::memory_order_relaxed);
        telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_retry_events.fetch_add(1,
                                                    std::memory_order_relaxed);
        ECUThreadErr = 10; // Command timeout
        signalRecovery();
        return false;
    };

    do {
        std::optional<ECUmmpacket> ecuptmp;
        if (!wait_for_response(ecuptmp)) {
            return std::nullopt;
        }

        // Copy the frame type id
        frametypeidtmp = ecuptmp->frametypeid;

        if (frametypeidtmp != ECU_ACK_CODE && frameid == ECU_READ_ERRORS_CODE) {
            if (frametypeidtmp != eECUFrameID) {
                // Unexpected packet
                // TODO: Handle it, send NOT ACK?
                // ECUWriteResponse(ECU_NOT_ACK_CODE);

                if (!ecup.add(std::move(*ecuptmp))) {
                    std::cerr << "Warning: ECUResponseCollection full ("
                              << ECUResponseCollection::MAX_RESPONSE_PACKETS
                              << " packets), dropping response" << std::endl;
                }
                return ecup;
            }

            // Send "Acknowledge" packet to the ECU
            bool sendACK = false;
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (std::chrono::steady_clock::now() >= deadline) {
                    signalRecovery();
                    return std::nullopt;
                }
                sendACK = sendECURequest(ECU_ACK_CODE);
            } while (!sendACK);
        }

        // NASA P10: Fixed-size collection with bounds checking
        if (!ecup.add(std::move(*ecuptmp))) {
            std::cerr << "Warning: ECUResponseCollection full ("
                      << ECUResponseCollection::MAX_RESPONSE_PACKETS
                      << " packets), dropping response" << std::endl;
        }
    } while (frametypeidtmp != ECU_ACK_CODE &&
             frameid == ECU_READ_ERRORS_CODE &&
             std::chrono::steady_clock::now() < deadline);

    if (frameid == ECU_READ_ERRORS_CODE && frametypeidtmp != ECU_ACK_CODE) {
        signalRecovery();
        return std::nullopt;
    }

    // CppCoreGuidelines: No std::move on return (NRVO/implicit move)
    return ecup;
}

void ECUMonomotronic::resetToNCKnownState() {
    ECUInited = false;
    KeepECUConnectionAlive = true;
    ECUThreadState = static_cast<int>(ThreadState::Handshake);
    ECUThreadErr = 0;
    ECUPacketCounter = 0;
    ECUThreadCanAcceptCommands = false;
    ECUConnectedNow = false;
    ECUThreadShouldProceed = true;
}

void ECUMonomotronic::signalRecovery() {
    ECUThreadCanAcceptCommands = false;
    ECUConnectedNow = false;
    ECUThreadShouldProceed = true;
    ECUThreadState.store(static_cast<int>(ThreadState::Recovering),
                         std::memory_order_relaxed);
}

std::optional<ECUmmpacket> ECUMonomotronic::getECUResponse() {
    if (ECUCommandResultAvailable) {
        ECUCommandResultAvailable = false;
        return std::move(ECUResponse);
    }

    return std::nullopt;
}

bool ECUMonomotronic::sendECURequest(uint8_t frameid,
                                     const std::vector<uint8_t> &data) {
    if (!ECUThreadCanAcceptCommands) {
        return false;
    }

    std::lock_guard<std::mutex> lck(ECUNewCommandMutex);

    ECUNewCommandTemp.frametypeid = frameid;

    // NASA P10: Copy vector into fixed-size array
    if (data.size() > ECUmmpacket::MAX_DATA_SIZE) {
        std::cerr << "Error: Request data too large: " << data.size()
                  << " bytes (max " << ECUmmpacket::MAX_DATA_SIZE << ")"
                  << std::endl;
        return false;
    }

    ECUNewCommandTemp.data_length = static_cast<uint8_t>(data.size());
    std::copy(data.begin(), data.end(), ECUNewCommandTemp.data.begin());

    ECUNewCommandAvailable = true;

    return true;
}

std::optional<ECUResponseCollection> ECUMonomotronic::ECUReadErrors() {
    return ECURequestData(ECU_READ_ERRORS_CODE, ECU_ERROR_DATA_CODE);
}

std::optional<ECUResponseCollection>
ECUMonomotronic::readECUMemory(uint8_t addressHigh, uint8_t addressLow,
                               uint8_t length) {
    if (length == 0 || length > 0x0C) {
        std::cerr << "Error: ECU memory read length invalid (" << std::dec
                  << static_cast<int>(length) << "), must be 1..12"
                  << std::endl;
        return std::nullopt;
    }

    return ECURequestData(ECU_DATA_MEMORY_READ, ECU_READ_DATA_CODE,
                          {length, addressHigh, addressLow});
}

std::optional<ECUResponseCollection>
ECUMonomotronic::requestSensorCollection() {
    return ECURequestData(ECU_REQ_SENSOR_COLLECTION,
                          ECU_RESP_SENSOR_COLLECTION);
}

std::optional<ECUmmpacket> ECUMonomotronic::ECUCleanErrors() {
    if (canAcceptCommands()) {
        std::optional<ECUmmpacket> ecuptmp;
        const auto deadline =
            std::chrono::steady_clock::now() + kCommandTimeout;

        bool sendACK = false;
        do {
            sendACK = sendECURequest(ECU_CLEAR_ERRORS_CODE);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (!sendACK && std::chrono::steady_clock::now() < deadline);

        if (!sendACK) {
            ECUThreadErr = 12;
            health_metrics.consecutive_timeout_errors.fetch_add(
                1, std::memory_order_relaxed);
            signalRecovery();
            return std::nullopt;
        }

        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ecuptmp = getECUResponse();
        } while (!ecuptmp.has_value() &&
                 std::chrono::steady_clock::now() < deadline && !ECUThreadExit);

        if (!ecuptmp.has_value()) {
            ECUThreadErr = 13;
            health_metrics.consecutive_timeout_errors.fetch_add(
                1, std::memory_order_relaxed);
            telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
            health_metrics.total_retry_events.fetch_add(
                1, std::memory_order_relaxed);
            signalRecovery();
            return std::nullopt;
        }
        std::cout << "ECU error response received" << std::endl;
        std::cout << static_cast<uint16_t>(ecuptmp.value().frametypeid)
                  << std::endl;
        std::cout << static_cast<uint16_t>(ecuptmp.value().counter)
                  << std::endl;
        std::cout << static_cast<uint16_t>(ecuptmp.value().size) << std::endl;

        for (const auto &byte : ecuptmp.value().get_data()) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        if (ecuptmp.value().frametypeid == 0x03) {
            assert(false);
            ECUWriteResponse(0x10);
        }

        // CppCoreGuidelines: No std::move on return (NRVO/implicit move)
        return ecuptmp;
    }

    return std::nullopt;
}

namespace {
struct ErrorDescription {
    uint16_t code;
    std::string_view description;
};

enum class ErrorOccurrence : uint8_t {
    Present = 0x1E,
    Intermittent = 0x03,
};

// Source: http://www.fiat-tipo.ru/fpost8823.html
constexpr auto kErrorDescriptions = std::to_array<ErrorDescription>({
    ErrorDescription{0x1A01,
                     "Atuador de marcha lenta - Sinal ruim? Motor com do "
                     "atuador de passo com defeito? Travado?"},
    ErrorDescription{0x0302,
                     "Sensor de rotação do virabrequim | Sem sinal/motor "
                     "desligado"},
    ErrorDescription{0x0402,
                     "Interruptor do atuador de marcha lenta | Curto ao GND ou "
                     "VCC"},
    ErrorDescription{0x0602, "Erro TPS"},
    ErrorDescription{0x0A02,
                     "Sensor de temperatura do liquido de arrefecimento"},
    ErrorDescription{0x0B02,
                     "Sensor de temperatura do ar circuito aberto ou curto "
                     "para GND ou VCC"},
    ErrorDescription{0x0D02, "Sonda lambda | problema sinal"},
    ErrorDescription{0x3102, "Erro de correção de mistura"},
    ErrorDescription{0x0303,
                     "Sensor de rotação do virabrequim | Erro de sincronismo "
                     "do sensor de posição do virabrequim"},
    ErrorDescription{0x3A46, "Imobilizador | Ativo"},
    ErrorDescription{0xFFFF,
                     "Erro de ECU | Defeito no computador ou selecionado "
                     "incorretamente OU problema de conexão"},
});

constexpr std::string_view kUnknownErrorDescription{"Unknown error code"};

constexpr bool isActive(uint8_t status) noexcept {
    return status == static_cast<uint8_t>(ErrorOccurrence::Present) ||
           status == static_cast<uint8_t>(ErrorOccurrence::Intermittent);
}
} // namespace

std::string_view ECUMonomotronic::errorPacketToString(const ECUmmpacket &p,
                                                      bool &present) {
    present = false;

    if (p.frametypeid != ECU_ERROR_DATA_CODE) {
        return {};
    }

    const auto data = p.get_data();
    if (data.size() < 3) {
        return {};
    }

    // CppCoreGuidelines: build code explicitly from bytes
    const uint16_t errcode =
        static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);

    present = isActive(data[2]);

    const auto it =
        std::find_if(kErrorDescriptions.begin(), kErrorDescriptions.end(),
                     [errcode](const ErrorDescription &entry) {
                         return entry.code == errcode;
                     });

    if (it != kErrorDescriptions.end()) {
        return it->description;
    }

    return kUnknownErrorDescription;
}

template <int n>
void sendInit(SerialPort &sp, int timeoutms, std::array<bool, n> v) {
    // CppCoreGuidelines: Use size_t for array indexing
    for (size_t i = 0; i < v.size(); i++) {
        sp.scBreak(v[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutms));
    }
}

namespace {
std::array<bool, 8> buildFiveBaudDataBits(uint8_t address) {
    std::array<bool, 8> bits{};
    for (size_t i = 0; i < bits.size(); ++i) {
        // break asserted = logical 0, break cleared = logical 1
        bits[i] = (address & (1u << i)) == 0;
    }
    return bits;
}
} // namespace

void ECUMonomotronic::ECUOpenThread() {
    if (!ECUThreadRunning) {
        ECUThreadExit = false;
        ECUInited = false;
        KeepECUConnectionAlive = true;
        ECUThreadState = 0;
        ECUThreadErr = 0;
        ECUPacketCounter = 0;

        ECUThreadObj = std::thread(ECUThreadFun, std::ref(*this));
        ECUThreadRunning = true;

        int policy = SCHED_FIFO;
        sched_param sch_params;
        sch_params.sched_priority = sched_get_priority_max(policy);
        if (pthread_setschedparam(ECUThreadObj.native_handle(), policy,
                                  &sch_params) != 0) {
            perror("pthread_setschedparam");
        } else {
            std::cout << "Thread priority set to max SCHED_FIFO\n";
        }
    } else {
        if (ECUThreadObj.joinable())
            ECUThreadObj.join();
    }
}

void ECUMonomotronic::sendInitSequence() {
    if (link_config_.init_mode != ECUInitMode::FiveBaudBreak) {
        std::cerr << "Unsupported init mode for MA1.7" << std::endl;
        return;
    }

    const auto init_bits = buildFiveBaudDataBits(link_config_.init_address);

    sp.scBreak(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sendInit<8>(sp, 200, init_bits);
    sp.scBreak(false);
}

void ECUMonomotronic::processECURequest(
    ECUMonomotronic &mm, std::chrono::steady_clock::time_point &lastpackettime,
    bool &nError) {
    // Custom request handler
    if (mm.ECUNewCommandAvailable) {
        std::lock_guard<std::mutex> lck(mm.ECUNewCommandMutex);

        // NASA P10: Use span overload - zero allocation in hot path
        if (!mm.ECUWritePacket(mm.ECUNewCommandTemp.frametypeid,
                               mm.ECUNewCommandTemp.get_data())) {
            mm.debug_regiter_err(__FILE__, __LINE__);
            mm.ECUThreadErr = 7;
            nError = true;
        } else {
            std::optional<ECUmmpacket> p = mm.ECUReadPacket();

            if (p.has_value()) {
                updatePacketCounter(mm, p.value());

                mm.ECUResponse = std::move(p.value());

                lastpackettime = std::chrono::steady_clock::now();
                mm.ECUCommandResultAvailable = true;

                // NASA P10 Health: Reset timeout errors, update
                // last communication
                mm.health_metrics.consecutive_timeout_errors.store(
                    0, std::memory_order_relaxed);
                mm.health_metrics.last_successful_communication =
                    lastpackettime;
            } else {
                mm.debug_regiter_err(__FILE__, __LINE__);
                mm.ECUThreadErr = 9;
                nError = true;
                mm.ECUCommandResultAvailable = false;
            }

            mm.ECUNewCommandAvailable = false;
        }
    }

    auto now = std::chrono::steady_clock::now();

    auto diffms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastpackettime);

    if (diffms.count() > 1000) {
        mm.debug_regiter_err(__FILE__, __LINE__);
        mm.ECUThreadErr = 5;
        nError = true;

        // NASA P10 Health: Track consecutive timeout errors
        mm.health_metrics.consecutive_timeout_errors.fetch_add(
            1, std::memory_order_relaxed);
    }

    if (diffms.count() > 500) // Keep the connection alive
    {
        // TODO: Improve it
        // May have bugs

        if (!mm.ECUWritePacket(ECU_ACK_CODE)) {
            mm.debug_regiter_err(__FILE__, __LINE__);
            mm.ECUThreadErr = 56;
            nError = true;
        } else {
            std::optional<ECUmmpacket> p = mm.ECUReadPacket();

            if (p.has_value()) {
                mm.ECUPacketCounter = p.value().counter;
                int code = p.value().frametypeid;

                if (code == ECU_ACK_CODE) {
                    //
                } else {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr = 6;
                    nError = true;
                }
            }

            lastpackettime = std::chrono::steady_clock::now();

            // NASA P10 Health: Reset timeout errors, update last
            // communication
            mm.health_metrics.consecutive_timeout_errors.store(
                0, std::memory_order_relaxed);
            mm.health_metrics.last_successful_communication = lastpackettime;
        }
    }
}

void ECUMonomotronic::ECUThreadFun(ECUMonomotronic &mm) {
    mm.health_metrics.thread_start_time = std::chrono::steady_clock::now();

    auto lastpackettime = mm.health_metrics.thread_start_time;

    mm.ECUThreadCanAcceptCommands = false;
    mm.ECUConnectedNow = false;
    mm.recovery_guard_until = std::chrono::steady_clock::time_point{};

    // Initialize field tracking before any I/O
    mm.resetReadPacketFieldTracking();
    mm.resetWritePacketFieldTracking();

    while (mm.sp.available() > 0) {
        mm.ECURead();
    }
    mm.sp.flushBoth();

    while (!mm.ECUThreadExit) {
        const auto now = std::chrono::steady_clock::now();

        if (now < mm.recovery_guard_until) {
            mm.ECUThreadCanAcceptCommands = false;
            mm.ECUConnectedNow = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (!mm.ECUThreadShouldProceed && !mm.ECUConnectedNow) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const ThreadState state = static_cast<ThreadState>(
            mm.ECUThreadState.load(std::memory_order_relaxed));
        bool nError = false;
        bool ECUSendSyncCode = false;

        // CppCoreGuidelines: Mark unused protocol bytes (may be validated in
        // future)
        [[maybe_unused]] int key1 = 0, key3 = 0, key4 = 0;
        int key2 = 0; // Initialize to prevent uninitialized use

        switch (state) {
        case ThreadState::Handshake: {
            mm.ECUThreadCanAcceptCommands = false;
            mm.ECUConnectedNow = false;

            int init_phase = 0;
            mm.sendInitSequence();

            for (int i = 0; i < 10000 && init_phase != 5; i++) {
                if (mm.sp.available() > 0) {
                    ECUByte code = mm.ECURead();

                    if (code.has_value()) {
                        std::cout << "Init byte: " << std::hex
                                  << static_cast<int>(code.value())
                                  << std::endl;
                        std::cout << "Code: " << std::hex
                                  << static_cast<int>(code.value())
                                  << std::endl;
                        switch (init_phase) {
                        case 0:
                            if (code == 0x55 && !ECUSendSyncCode) {
                                ECUSendSyncCode = true;
                                ++init_phase;
                            }
                            break;

                        case 1:
                            key1 = code.value(); // 0x46
                            ++init_phase;
                            break;

                        case 2:
                            key2 = code.value(); // 0x85
                            ++init_phase;
                            break;

                        case 3:
                            key3 = code.value(); // 0x89
                            ++init_phase;
                            break;

                        case 4:
                            key4 = code.value(); // 0xBC
                            ++init_phase;
                            break;

                        default:
                            break;
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (ECUSendSyncCode) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                // NOVO: Flush adequado antes de enviar resposta crítica
                mm.sp.flushBoth(); // Limpa ambos buffers (TX e RX)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                while (mm.sp.available() > 0) {
                    mm.ECURead(50);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                // CppCoreGuidelines: Explicit cast for bitwise NOT result
                if (!mm.ECUWrite(static_cast<uint8_t>(~key2))) {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr = 2;
                    nError = true;
                }
            } else {
                mm.debug_regiter_err(__FILE__, __LINE__);
                mm.ECUThreadErr = 1;
                nError = true;
            }

            if (!nError) {
                int code = 0;

                // NASA P10: Reset count instead of clear()
                mm.initPackets_count = 0;
                do {
                    std::optional<ECUmmpacket> p = mm.ECUReadPacket();

                    if (p.has_value()) {
                        updatePacketCounter(mm, p.value());
                        code = p.value().frametypeid;

                        // NASA P10: Fixed-size array with bounds checking
                        if (mm.initPackets_count < mm.MAX_INIT_PACKETS) {
                            mm.initPackets[mm.initPackets_count++] =
                                std::move(p.value());
                        } else {
                            std::cerr << "Warning: MAX_INIT_PACKETS ("
                                      << mm.MAX_INIT_PACKETS
                                      << ") exceeded, dropping packet"
                                      << std::endl;
                        }

                        if (code == ECU_ACK_CODE) {
                            std::this_thread::sleep_for(
                                std::chrono::milliseconds(50));
                            break;
                        }

                        if (!mm.ECUWritePacket(ECU_ACK_CODE)) {
                            mm.debug_regiter_err(__FILE__, __LINE__);
                            mm.ECUThreadErr = 4;
                            nError = true;
                        }
                    } else {
                        mm.debug_regiter_err(__FILE__, __LINE__);
                        mm.ECUThreadErr = 3;
                        nError = true;
                        break;
                    }
                } while (true);
            }

            lastpackettime = std::chrono::steady_clock::now();
            if (!nError) {
                mm.ECUConnectedNow = true;
                mm.ECUThreadCanAcceptCommands = true;
                mm.health_metrics.consecutive_timeout_errors.store(
                    0, std::memory_order_relaxed);
                mm.health_metrics.last_successful_communication =
                    lastpackettime;
                mm.ECUThreadState.store(static_cast<int>(ThreadState::Ready),
                                        std::memory_order_relaxed);
            } else {
                mm.signalRecovery();
            }
        } break;

        case ThreadState::Ready: {
            // NASA P10 Health: Measure loop latency
            auto loop_start = std::chrono::steady_clock::now();

            std::this_thread::yield();
            if (mm.KeepECUConnectionAlive) {
                processECURequest(mm, lastpackettime, nError);
            }

            const auto consecutive_timeouts =
                mm.health_metrics.consecutive_timeout_errors.load(
                    std::memory_order_relaxed);
            if (nError || consecutive_timeouts >=
                              static_cast<uint32_t>(kMaxConsecutiveTimeouts)) {
                nError = true;
                mm.signalRecovery();
            }

            // NASA P10 Health: Update loop latency metrics
            auto loop_end = std::chrono::steady_clock::now();
            uint64_t latency_us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    loop_end - loop_start)
                    .count());

            // Update max latency (compare-and-swap)
            uint64_t old_max = mm.health_metrics.max_loop_latency_us.load(
                std::memory_order_relaxed);
            while (latency_us > old_max &&
                   !mm.health_metrics.max_loop_latency_us.compare_exchange_weak(
                       old_max, latency_us, std::memory_order_relaxed)) {
            }

            // Update exponential moving average (alpha = 1/16)
            uint64_t old_avg = mm.health_metrics.avg_loop_latency_us.load(
                std::memory_order_relaxed);
            uint64_t new_avg = (old_avg * 15 + latency_us) / 16;
            mm.health_metrics.avg_loop_latency_us.store(
                new_avg, std::memory_order_relaxed);

            // Poll buffer overruns (CircleMTIO tracks internally)
            mm.health_metrics.buffer_overruns.store(
                mm.bytesLogging.get_overruns(), std::memory_order_relaxed);
        } break;

        case ThreadState::Recovering: {
            // NASA P10 Health: Track recovery cycle entry
            mm.health_metrics.total_recovery_cycles.fetch_add(
                1, std::memory_order_relaxed);
            mm.resetToNCKnownState();
            mm.recovery_guard_until =
                std::chrono::steady_clock::now() + kRecoveryBackoff;
            lastpackettime = std::chrono::steady_clock::now();
            mm.sp.flushBoth();
            mm.ECUThreadState.store(static_cast<int>(ThreadState::Handshake),
                                    std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } break;

        default:
            mm.signalRecovery();
            nError = true;
            break;
        }

        if (nError) {
            mm.telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
            mm.health_metrics.total_retry_events.fetch_add(
                1, std::memory_order_relaxed);
        }

        std::this_thread::yield();
    }

    mm.ECUThreadExit = false;
    mm.ECUThreadRunning = false;
    mm.ECUInited = false;
    mm.KeepECUConnectionAlive = true;
    mm.ECUThreadState = 0;
    mm.ECUThreadErr = 0;
    mm.ECUPacketCounter = 0;
}

bool ECUMonomotronic::ECUWrite(uint8_t b) {
    b = b & 0xFF;

    // Log before writing (with current packet tracking)
    if (enableLog) {
        PacketLogEntry entry(
            std::chrono::high_resolution_clock::now(), ECUPacketCounter,
            current_write_packet_field, b, LogDirection::WRITE,
            static_cast<uint8_t>(ECUThreadState.load()), PacketLogError::NONE);
        addbytelog(entry);
    }

    if (!sp.writeByte(b)) {
        return false;
    }

    // REMOVED: drainOutput() blocks for ~18ms per byte on USB (2ms TX + 16ms
    // latency) For 5-byte packet: 90ms vs 10ms on emulator (9× slower!) Echo
    // verification below already ensures byte was transmitted correctly
    // sp.drainOutput();

    std::optional<uint8_t> result;

    // USB serial adapters need more time for echo due to latency (16ms default)
    // Increased from 50ms to 100ms to accommodate USB delays
    constexpr int echo_timeout_ms = 100; // Was 50ms
    int attempts = 0;

    do {
        result = sp.fastByteReadUSerialPort(echo_timeout_ms);
        attempts++;
    } while (!result.has_value() && attempts < 10); // Reduced from 20 to 10

    if (!result.has_value()) {
        telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_retry_events.fetch_add(1,
                                                    std::memory_order_relaxed);
        return false;
    }

    if (result.has_value()) {
        if (result.value() == b) {
            return true;
        }
        telemetry_ack_mismatch.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_ack_mismatches.fetch_add(
            1, std::memory_order_relaxed);
        telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_retry_events.fetch_add(1,
                                                    std::memory_order_relaxed);
        std::cout << std::hex << "result.value() == b "
                  << static_cast<int>(result.value()) << " "
                  << static_cast<int>(b);
    }

    return false;
}

ECUByte ECUMonomotronic::ECURead([[maybe_unused]] int timeout) {
    std::optional<uint8_t> result = sp.fastByteReadUSerialPort(timeout);

    // Log after reading (with current packet tracking)
    if (enableLog && result.has_value()) {
        PacketLogEntry entry(
            std::chrono::high_resolution_clock::now(), ECUPacketCounter,
            current_read_packet_field, result.value(), LogDirection::READ,
            static_cast<uint8_t>(ECUThreadState.load()), PacketLogError::NONE);
        addbytelog(entry);
    }

    return result;
}

ECUByte ECUMonomotronic::ECUReadResponse() {
    ECUByte result = ECURead();

    if (result.has_value()) {
        bool success = ECUWrite(static_cast<uint8_t>(~result.value()));
        if (!success) {
            telemetry_ack_mismatch.fetch_add(1, std::memory_order_relaxed);
            health_metrics.total_ack_mismatches.fetch_add(
                1, std::memory_order_relaxed);
        }
    }

    return result;
}

bool ECUMonomotronic::ECUWriteResponse(uint8_t b) {
    if (!ECUWrite(b)) {
        return false;
    }

    ECUByte result = ECURead();

    if (!result) {
        return false;
    }

    if (result.value() != static_cast<uint8_t>(~b)) {
        telemetry_ack_mismatch.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_ack_mismatches.fetch_add(
            1, std::memory_order_relaxed);
        telemetry_retry_events.fetch_add(1, std::memory_order_relaxed);
        health_metrics.total_retry_events.fetch_add(1,
                                                    std::memory_order_relaxed);
        return false;
    }

    return true;
}

// NASA P10: In-place fill, no allocation
void ECUMonomotronic::ECUReadSequential(std::span<uint8_t> buffer) {
    for (size_t i = 0; i < buffer.size(); i++) {
        ECUByte r = ECUReadResponse();

        if (r.has_value()) {
            buffer[i] = r.value();
        } else {
            // Partial read - should not happen in normal operation
            std::cerr << "Warning: ECUReadSequential incomplete at byte " << i
                      << " of " << buffer.size() << std::endl;
            break;
        }

        // Advance packet field for next data byte
        advanceReadPacketField();
    }
}

// NASA P10: Accept span instead of vector
bool ECUMonomotronic::ECUWriteSequential(std::span<const uint8_t> data) {
    for (size_t i = 0; i < data.size(); i++) {
        if (!ECUWriteResponse(data[i]))
            return false;

        // Advance packet field for next data byte
        advanceWritePacketField();
    }

    return true;
}

std::optional<ECUmmpacket> ECUMonomotronic::ECUReadPacket() {
    ECUmmpacket result;

    sp.drainOutput(); // Aguarda qualquer TX pendente

    // NASA P10: Track packet field for logging
    resetReadPacketFieldTracking(); // Start with SIZE

    // Validate field is at SIZE
    validatePacketField(PacketField::SIZE, true);

    {
        ECUByte size = ECUReadResponse();

        if (size.has_value()) {
            result.size = size.value();
        } else {
            return std::nullopt;
        }
    }

    current_read_packet_field = PacketField::COUNTER;
    {
        ECUByte counter = ECUReadResponse();

        if (counter.has_value()) {
            result.counter = counter.value();
        } else {
            std::cerr << "Error: Incomplete data received: no counter"
                      << std::endl;
            return std::nullopt;
        }
    }

    current_read_packet_field = PacketField::FRAME_TYPE;
    {
        ECUByte frameid = ECUReadResponse();

        if (frameid.has_value()) {
            result.frametypeid = frameid.value();
        } else {
            std::cerr << "Error: Incomplete data received: no frame ID"
                      << std::endl;
            return std::nullopt;
        }
    }

    if (result.size > 3) {
        current_read_packet_field = PacketField::DATA_START;

        // NASA P10: Fill in-place, no allocation
        result.data_length = result.size - 3;

        // Bounds check (protocol guarantees max 252 bytes)
        if (result.data_length > ECUmmpacket::MAX_DATA_SIZE) {
            std::cerr << "Error: Packet data too large: " << result.data_length
                      << " bytes (max " << ECUmmpacket::MAX_DATA_SIZE << ")"
                      << std::endl;
            return std::nullopt;
        }

        // Read directly into fixed-size array
        ECUReadSequential(
            std::span<uint8_t>(result.data.data(), result.data_length));
    } else {
        result.data_length = 0; // No data payload
    }

    current_read_packet_field = PacketField::END_MARKER;

    // Validate field is at END_MARKER
    validatePacketField(PacketField::END_MARKER, true);

    {
        ECUByte end = ECURead();

        if (end.has_value()) {
            if (end.value() != 0x03) {
                // Log error for invalid terminator
                if (enableLog) {
                    PacketLogEntry error(
                        std::chrono::high_resolution_clock::now(),
                        ECUPacketCounter, PacketField::END_MARKER, end.value(),
                        LogDirection::READ,
                        static_cast<uint8_t>(ECUThreadState.load()),
                        PacketLogError::CHECKSUM_MISMATCH); // Invalid
                                                            // terminator
                    addbytelog(error);
                }

                std::cout
                    << "Error: Incomplete data received: unexpected end byte"
                    << std::endl;
                std::cout << "Size: " << std::hex
                          << static_cast<int>(result.size) << std::endl;
                std::cout << "Counter: " << std::hex
                          << static_cast<int>(result.counter) << std::endl;
                std::cout << "Frame ID: " << std::hex
                          << static_cast<int>(result.frametypeid) << std::endl;
                std::cout << "End: " << std::hex
                          << static_cast<int>(end.value()) << std::endl;

                debugTofile();
                telemetry_end_marker_error.fetch_add(1,
                                                     std::memory_order_relaxed);

                // NASA P10 Health: Track consecutive checksum errors
                health_metrics.consecutive_checksum_errors.fetch_add(
                    1, std::memory_order_relaxed);

                return std::nullopt;
            }
        } else {
            // Log error for missing terminator
            if (enableLog) {
                PacketLogEntry error(
                    std::chrono::high_resolution_clock::now(), ECUPacketCounter,
                    PacketField::END_MARKER, 0x00, LogDirection::READ,
                    static_cast<uint8_t>(ECUThreadState.load()),
                    PacketLogError::TIMEOUT); // Missing byte
                addbytelog(error);
            }

            std::cerr << "Error: Incomplete data received: no end byte"
                      << std::endl;
            telemetry_end_marker_error.fetch_add(1, std::memory_order_relaxed);

            // NASA P10 Health: Track consecutive checksum errors
            health_metrics.consecutive_checksum_errors.fetch_add(
                1, std::memory_order_relaxed);

            return std::nullopt;
        }
    }

    // NASA P10 Health: Track successful packet reception
    health_metrics.total_packets_received.fetch_add(1,
                                                    std::memory_order_relaxed);
    health_metrics.consecutive_checksum_errors.store(0,
                                                     std::memory_order_relaxed);

    // Reset field tracking for next packet
    resetReadPacketFieldTracking();

    return result;
}

bool ECUMonomotronic::ECUWritePacket(uint8_t frameid,
                                     const std::vector<uint8_t> &data) {
    // CppCoreGuidelines: Use explicit cast for size conversion
    auto packetSize = static_cast<uint8_t>(3 + data.size());

    // NASA P10: Track packet field for logging
    resetWritePacketFieldTracking(); // Start with SIZE

    if (!ECUWriteResponse(packetSize)) {
        return false;
    }

    current_write_packet_field = PacketField::COUNTER;
    if (!ECUWriteResponse(++ECUPacketCounter)) {
        return false;
    }

    current_write_packet_field = PacketField::FRAME_TYPE;
    if (!ECUWriteResponse(frameid)) {
        return false;
    }

    if (data.size() > 0) {
        current_write_packet_field = PacketField::DATA_START;
        ECUWriteSequential(data); // Advances field internally
    }

    current_write_packet_field = PacketField::END_MARKER;
    if (!ECUWrite(0x03)) {
        return false;
    }

    // NASA P10 Health: Track successful packet transmission
    health_metrics.total_packets_sent.fetch_add(1, std::memory_order_relaxed);

    // Reset field tracking for next packet
    resetWritePacketFieldTracking();

    return true;
}

// NASA P10: Span overload for zero-copy packet transmission
bool ECUMonomotronic::ECUWritePacket(uint8_t frameid,
                                     std::span<const uint8_t> data) {
    // CppCoreGuidelines: Use explicit cast for size conversion
    auto packetSize = static_cast<uint8_t>(3 + data.size());

    // NASA P10: Track packet field for logging
    resetWritePacketFieldTracking(); // Start with SIZE

    if (!ECUWriteResponse(packetSize)) {
        return false;
    }

    current_write_packet_field = PacketField::COUNTER;
    if (!ECUWriteResponse(++ECUPacketCounter)) {
        return false;
    }

    current_write_packet_field = PacketField::FRAME_TYPE;
    if (!ECUWriteResponse(frameid)) {
        return false;
    }

    if (data.size() > 0) {
        current_write_packet_field = PacketField::DATA_START;
        ECUWriteSequential(data); // Advances field internally
    }

    current_write_packet_field = PacketField::END_MARKER;
    if (!ECUWrite(0x03)) {
        return false;
    }

    // NASA P10 Health: Track successful packet transmission
    health_metrics.total_packets_sent.fetch_add(1, std::memory_order_relaxed);

    // Reset field tracking for next packet
    resetWritePacketFieldTracking();

    return true;
}

void ECUMonomotronic::init() { ECUOpenThread(); }

void ECUMonomotronic::forcestop() {
    sp.close();

    std::cout << "Err " << ECUThreadErr << std::endl;
    printlogging();
}

void ECUMonomotronic::stop() {
    // Graceful shutdown: try to end diagnosis session if link is healthy
    if (ECUThreadRunning && ECUThreadCanAcceptCommands && ECUConnectedNow) {
        bool queued = sendECURequest(ECU_REQ_DIAGNOSIS_END);

        if (queued) {
            // TODO: Investigate firmware end-of-diagnosis response mapping
            // Firmware branch (state 6) returns pairs like 0x11/0x11,
            // 0x44/0x44, 0x55/0x55, 0x66/0x66, 0x77/0x77 based on XDATA[0xC8]
            // and flags. Need to correlate these values with ECU status
            // meanings.
            auto deadline = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(500);
            while (std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (auto resp = getECUResponse()) {
                    // Any response is enough to consider the ECU listening
                    break;
                }
            }
        }
    }

    KeepECUConnectionAlive = false;
    ECUThreadExit = true;
}

void ECUMonomotronic::debugTofile() {
    // if (!ECUThreadRunning)
    {
        std::fstream fs("ECUSerial.log", std::ios::out | std::ios::trunc);

        fs << "\n========== BYTE-LEVEL LOG ==========\n";
        fprintlogging(fs);

        fs << "\n========== PACKET SUMMARY ==========\n";
        printPacketSummary(fs);

        fs.flush();
    }
}

void ECUMonomotronic::purgeSerial() {}

ECUMonomotronic::ECUMonomotronic(const ECULinkConfig &config) noexcept
    : link_config_(config), sp(config.port, ToUint(config.session_baud)) {
    enableLog = config.enable_logging;
    ECUThreadExit = false;
    ECUThreadRunning = false;
    ECUInited = false;
    KeepECUConnectionAlive = true;
    ECUThreadState = 0;
    ECUThreadErr = 0;
    ECUPacketCounter = 0;
    ECUNewCommandAvailable = false;
    ECUCommandResultAvailable = false;
    ECUThreadCanAcceptCommands = false;

    ECUThreadShouldProceed = false;

    // NASA P10: CircleMTIO is fixed-size, no reserve() needed
    // bytesLogging is pre-allocated to 20,480 entries at compile-time

    debug_file = nullptr;
    debug_line = 0;

    steady_start_time = std::chrono::steady_clock::now();
    system_start_time = std::chrono::system_clock::now();
}

ECUMonomotronic::ECUMonomotronic(const char *port, bool enableLogging) noexcept
    : ECUMonomotronic(MakeKnownProfileConfig(ECUKnownProfile::FiatTipo16Ie,
                                             port != nullptr ? port
                                                             : "/dev/ttyUSB0",
                                             enableLogging)) {}

ECUMonomotronic::~ECUMonomotronic() {
    if (ECUThreadRunning) {
        ECUThreadExit = true;
    }

    if (ECUThreadObj.joinable())
        ECUThreadObj.join();
}
