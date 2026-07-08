#pragma once
#ifndef ESP32MONOMOTRONIC_h
#define ESP32MONOMOTRONIC_h

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

#include "Arduino.h"
#include "SensorCatalog.h"
#include "optional.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string.h>

typedef optional<uint8_t> ECUByte;

struct ESP32ScannerConfig {
    uint32_t session_baud{4800};
    uint8_t init_address{0x10};
    uint8_t tx_init_pin{0};
    uint8_t aux_init_pin{2};
    uint32_t read_timeout_ms{250};
    uint32_t write_timeout_ms{250};
    uint32_t command_timeout_ms{1800};
    uint32_t keep_alive_ms{500};
};

struct ECUmmpacket {
    static constexpr size_t MAX_DATA_SIZE = 252;

    uint8_t size{3};
    uint8_t counter{0};
    uint8_t frametypeid{0};
    std::array<uint8_t, MAX_DATA_SIZE> data{};
    uint8_t data_length{0};
    uint8_t end{0x03};
};

struct ECUResponseCollection {
    static constexpr size_t MAX_RESPONSE_PACKETS = 10;

    std::array<ECUmmpacket, MAX_RESPONSE_PACKETS> packets{};
    size_t count{0};

    void clear() { count = 0; }

    bool add(const ECUmmpacket &pkt) {
        if (count >= packets.size()) {
            return false;
        }
        packets[count++] = pkt;
        return true;
    }
};

struct ECUHealthSnapshot {
    uint32_t packets_sent{0};
    uint32_t packets_received{0};
    uint32_t timeouts{0};
    uint32_t retries{0};
    uint32_t reconnects{0};
    uint32_t ack_errors{0};
    uint32_t packet_errors{0};
    uint32_t last_packet_ms{0};
    uint8_t last_collection_table{0};
};

struct ECUStatusSnapshot {
    const char *protocol_state{"disconnected"};
    const char *current_operation{"none"};
    bool connected{false};
    bool can_accept_commands{false};
    bool busy{false};
    bool init_ready{false};
    bool echo_byte_present{false};
    bool thread_started{false};
    int task_state{0};
    int freertos_state{0};
    int error_code{0};
    int debug_line{0};
};

class ESP32Monomotronic {
  public:
    enum class OperationKind : uint8_t {
        None = 0,
        ReadErrors,
        ReadSensor,
        ReadMemory,
        ReadCollection,
        ClearErrors,
        DetermineCollectionTable
    };

  private:
    struct PendingCommand {
        uint8_t frameid{0};
        std::array<uint8_t, 16> data{};
        uint8_t data_length{0};
    };

    bool inited_{false};
    std::atomic<int> taskState_{0};
    std::atomic<int> ECUThreadErr_{0};
    std::atomic<bool> ECUConnected_{false};
    std::atomic<bool> baudEchoOK_{false};
    std::atomic<bool> initPacketsOk_{false};
    std::atomic<int> debug_line_{0};
    std::atomic<bool> ECUThreadCanAcceptCommands_{false};
    std::atomic<bool> ECUNewCommandAvailable_{false};
    std::atomic<bool> ECUCommandResultAvailable_{false};
    std::atomic<bool> stopRequested_{false};
    std::mutex commandMutex_;
    std::mutex sessionMutex_;
    mutable std::mutex initPacketsMutex_;

    ESP32ScannerConfig config_{};
    PendingCommand pendingCommand_{};
    ECUmmpacket ECUResponse_{};
    ECUResponseCollection initPackets_{};
    int ECUPacketCounter_{0};
    TaskHandle_t Task1_{nullptr};
    std::atomic<OperationKind> activeOperation_{OperationKind::None};

    std::atomic<uint32_t> packets_sent_{0};
    std::atomic<uint32_t> packets_received_{0};
    std::atomic<uint32_t> timeouts_{0};
    std::atomic<uint32_t> retries_{0};
    std::atomic<uint32_t> reconnects_{0};
    std::atomic<uint32_t> ack_errors_{0};
    std::atomic<uint32_t> packet_errors_{0};
    std::atomic<uint32_t> last_packet_ms_{0};
    std::atomic<uint8_t> last_collection_table_{0};

    static void updatePacketCounter(ESP32Monomotronic &mm,
                                    const ECUmmpacket &p);
    static void sendInitPins(uint8_t port1, uint8_t port2, uint8_t value);

    ECUByte ECURead(uint32_t timeout = 1000);
    bool ECUWrite(uint8_t b);
    ECUByte ECUReadAndResponse(uint32_t timeout = 1000);
    bool ECUWriteWaitResponse(uint8_t b, uint32_t timeout = 1000);
    bool ECUReadSequential(uint8_t *buffer, uint8_t size, uint32_t timeout);
    bool ECUWriteSequential(const uint8_t *data, uint8_t size,
                            uint32_t timeout);
    optional<ECUmmpacket> ECUReadPacket(uint32_t timeout = 1000);
    bool ECUWritePacket(uint8_t frameid, const uint8_t *data = nullptr,
                        uint8_t length = 0, uint32_t timeout = 1000);
    optional<ECUResponseCollection>
    ECURequestData(uint8_t frameid, uint8_t expectedFrame,
                   const uint8_t *data = nullptr, uint8_t length = 0,
                   uint32_t timeout = 1000);
    bool waitForResponse(optional<ECUmmpacket> &response, uint32_t timeout_ms);
    bool baudInit();
    void recordPacketActivity(bool sent);
    bool tryStartOperation(OperationKind operation);
    void finishOperation();
    void resetInitPackets();
    bool appendInitPacket(const ECUmmpacket &packet);
    void debug_regiter_err(const char *file, int line) { debug_line_ = line; }
    static void commThread(void *mm);

  public:
    int getThreadErrorCode() const { return ECUThreadErr_; }
    int getThreadTaskState() const { return taskState_; }
    bool isECUConnected() const { return ECUConnected_; }
    bool isEchoBytePresent() const { return baudEchoOK_; }
    int getDebugLine() const { return debug_line_; }
    const ESP32ScannerConfig &getConfig() const { return config_; }
    void setConfig(const ESP32ScannerConfig &config) { config_ = config; }
    bool getInitPacketsSnapshot(ECUResponseCollection &out) const;
    ECUStatusSnapshot getStatusSnapshot() const;
    bool isBusy() const {
        return activeOperation_.load() != OperationKind::None;
    }
    bool isInitReady() const { return initPacketsOk_; }

    optional<ECUmmpacket> getECUResponse();
    bool sendECURequest(uint8_t frameid, const uint8_t *data = nullptr,
                        uint8_t length = 0);

    eTaskState getThreadState() const {
        if (inited_) {
            return eTaskGetState(Task1_);
        }
        return eDeleted;
    }

    bool canAcceptCommands() const { return ECUThreadCanAcceptCommands_; }

    optional<ECUResponseCollection> ECUReadErrors();
    optional<ECUResponseCollection> ECUReadSensor(uint8_t sensorID);
    optional<ECUResponseCollection>
    readECUMemory(uint8_t addressHigh, uint8_t addressLow, uint8_t length);
    optional<ECUResponseCollection> requestSensorCollection();
    optional<ECUmmpacket> ECUCleanErrors();
    uint8_t determineCollectionTable();
    ECUHealthSnapshot getHealthSnapshot() const;
    const char *getCurrentOperationName() const;
    const char *getProtocolStateName() const;

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

    enum ErrorCode {
        ERR_NONE = 0,
        ERR_HANDSHAKE_KEY = 1,
        ERR_HANDSHAKE_TIMEOUT = 3,
        ERR_KEEPALIVE = 4,
        ERR_PACKET_WRITE = 7,
        ERR_PACKET_READ = 9,
        ERR_CLEAR_SEND_TIMEOUT = 12,
        ERR_CLEAR_RESPONSE_TIMEOUT = 13,
        ERR_PACKET_END = 14,
        ERR_PACKET_SIZE = 15,
        ERR_PACKET_UNEXPECTED = 16,
        ERR_TASK_CREATE = 17
    };

    static const char *errorPacketToString(const ECUmmpacket &p, bool &present,
                                           TextLocale locale);

    bool init();
    void stop() { stopRequested_ = true; }

    ESP32Monomotronic();
};

#endif
