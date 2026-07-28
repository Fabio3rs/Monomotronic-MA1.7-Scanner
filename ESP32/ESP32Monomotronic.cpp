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

#include "ESP32Monomotronic.h"
#include "SensorCatalog.h"

#include "driver/uart.h"

namespace {
constexpr uint32_t kIdleYieldMs = 1;

struct ErrorCodeEntry {
    int code;
    const char *description;
};

constexpr std::array<ErrorCodeEntry, 11> kErrorCodeEntries{{
    {0x1A01, "Atuador de marcha lenta - Sinal ruim? Motor com do "
             "atuador de passo com defeito? Travado?"},
    {0x0302, "Sensor de rotacao do virabrequim | Sem sinal/motor desligado"},
    {0x0402, "Interruptor do atuador de marcha lenta | Curto ao GND ou VCC"},
    {0x0602, "Erro TPS"},
    {0x0A02, "Sensor de temperatura do liquido de arrefecimento"},
    {0x0B02, "Sensor de temperatura do ar circuito aberto ou curto para GND "
             "ou VCC"},
    {0x0D02, "Sonda lambda | problema sinal"},
    {0x3102, "Erro de correcao de mistura"},
    {0x0303, "Sensor de rotacao do virabrequim | Erro de sincronismo do "
             "sensor de posicao do virabrequim"},
    {0x3A46, "Imobilizador | Ativo"},
    {0xFFFF, "Erro de ECU | Defeito no computador ou selecionado "
             "incorretamente OU problema de conexao"},
}};

uint32_t DeadlineFromNow(uint32_t timeout_ms) { return millis() + timeout_ms; }

bool DeadlineReached(uint32_t deadline_ms) {
    return static_cast<int32_t>(millis() - deadline_ms) >= 0;
}

const char *OperationName(ESP32Monomotronic::OperationKind operation) {
    switch (operation) {
    case ESP32Monomotronic::OperationKind::ReadErrors:
        return "read_errors";
    case ESP32Monomotronic::OperationKind::ReadSensor:
        return "read_sensor";
    case ESP32Monomotronic::OperationKind::ReadMemory:
        return "read_memory";
    case ESP32Monomotronic::OperationKind::ReadCollection:
        return "read_collection";
    case ESP32Monomotronic::OperationKind::ClearErrors:
        return "clear_errors";
    case ESP32Monomotronic::OperationKind::DetermineCollectionTable:
        return "determine_collection_table";
    case ESP32Monomotronic::OperationKind::None:
    default:
        return "none";
    }
}

constexpr uart_port_t kUartPort = UART_NUM_2;

void configureUartLowLatency() {
    // Mantém também o estado interno do HardwareSerial consistente.
    Serial2.setRxFIFOFull(1);
    Serial2.setRxTimeout(1);

    // Remove apenas o idle adicional do hardware.
    ESP_ERROR_CHECK(uart_set_tx_idle_num(kUartPort, 0));
}

} // namespace

void ESP32Monomotronic::sendInitPins(uint8_t port1, uint8_t port2,
                                     uint8_t value) {
    digitalWrite(port1, value);
    digitalWrite(port2, value);
    delay(200);
}

void ESP32Monomotronic::configureSerial() {
    if (Serial2) {
        Serial2.end();
    }

    // TX ring buffer já é zero por padrão no Arduino-ESP32 2.0.6.
    Serial2.begin(config_.session_baud, SERIAL_8N1);
    configureUartLowLatency();
}

bool ESP32Monomotronic::baudInit() {
    if (config_.disable_uart_in_slow_init) {
        Serial2.end();
    }

    pinMode(config_.tx_init_pin, OUTPUT);
    pinMode(config_.aux_init_pin, OUTPUT);

    digitalWrite(config_.tx_init_pin, HIGH);
    digitalWrite(config_.aux_init_pin, HIGH);
    delay(200);

    const uint8_t address = config_.init_address;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const bool high = (address & (1 << bit)) == 0;
        sendInitPins(config_.tx_init_pin, config_.aux_init_pin,
                     high ? HIGH : LOW);
    }

    digitalWrite(config_.tx_init_pin, LOW);
    digitalWrite(config_.aux_init_pin, LOW);

    if (config_.disable_uart_in_slow_init) {
        configureSerial();
    }
    return true;
}

void ESP32Monomotronic::updatePacketCounter(ESP32Monomotronic &mm,
                                            const ECUmmpacket &p) {
    if (p.counter > mm.ECUPacketCounter_) {
        mm.ECUPacketCounter_ = p.counter;
    }
}

void ESP32Monomotronic::recordPacketActivity(bool sent) {
    if (sent) {
        packets_sent_.fetch_add(1, std::memory_order_relaxed);
    } else {
        packets_received_.fetch_add(1, std::memory_order_relaxed);
    }
    last_packet_ms_.store(millis(), std::memory_order_relaxed);
}

bool ESP32Monomotronic::tryStartOperation(OperationKind operation) {
    OperationKind expected = OperationKind::None;
    return activeOperation_.compare_exchange_strong(expected, operation);
}

void ESP32Monomotronic::finishOperation() {
    activeOperation_.store(OperationKind::None, std::memory_order_relaxed);
}

void ESP32Monomotronic::resetInitPackets() {
    std::lock_guard<std::mutex> lock(initPacketsMutex_);
    initPackets_.clear();
}

bool ESP32Monomotronic::appendInitPacket(const ECUmmpacket &packet) {
    std::lock_guard<std::mutex> lock(initPacketsMutex_);
    return initPackets_.add(packet);
}

bool ESP32Monomotronic::getInitPacketsSnapshot(
    ECUResponseCollection &out) const {
    if (!initPacketsOk_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(initPacketsMutex_);
    out = initPackets_;
    return true;
}

const char *ESP32Monomotronic::getCurrentOperationName() const {
    return OperationName(activeOperation_.load(std::memory_order_relaxed));
}

const char *ESP32Monomotronic::getProtocolStateName() const {
    if (isBusy()) {
        return "busy";
    }

    if (ECUConnected_ && ECUThreadCanAcceptCommands_) {
        return "ready";
    }

    if (inited_.load(std::memory_order_acquire) &&
        (taskState_.load(std::memory_order_relaxed) == 1 ||
         taskState_.load(std::memory_order_relaxed) == 2)) {
        return "handshaking";
    }

    if (ECUThreadErr_.load(std::memory_order_relaxed) != ERR_NONE) {
        return "error";
    }

    return "disconnected";
}

ECUStatusSnapshot ESP32Monomotronic::getStatusSnapshot() const {
    ECUStatusSnapshot snapshot;
    snapshot.protocol_state = getProtocolStateName();
    snapshot.current_operation = getCurrentOperationName();
    snapshot.connected = ECUConnected_.load(std::memory_order_relaxed);
    snapshot.can_accept_commands =
        ECUThreadCanAcceptCommands_.load(std::memory_order_relaxed);
    snapshot.busy = isBusy();
    snapshot.init_ready = initPacketsOk_.load(std::memory_order_relaxed);
    snapshot.echo_byte_present = baudEchoOK_.load(std::memory_order_relaxed);
    snapshot.thread_started = inited_.load(std::memory_order_acquire);
    snapshot.task_state = taskState_.load(std::memory_order_relaxed);
    snapshot.freertos_state = static_cast<int>(getThreadState());
    snapshot.error_code = ECUThreadErr_.load(std::memory_order_relaxed);
    snapshot.debug_line = debug_line_.load(std::memory_order_relaxed);
    return snapshot;
}

ECUByte ESP32Monomotronic::ECURead(uint32_t timeout) {
    const uint32_t deadline = DeadlineFromNow(timeout);

    while (!DeadlineReached(deadline)) {
        if (Serial2.available() > 0) {
            uint8_t code = static_cast<uint8_t>(Serial2.read() & 0xFF);
            return code;
        }
        delay(kIdleYieldMs);
    }

    timeouts_.fetch_add(1, std::memory_order_relaxed);
    return nullopt;
}

bool ESP32Monomotronic::ECUWrite(uint8_t b) {
    Serial2.write(b);
    Serial2.flush();

    const uint32_t deadline = DeadlineFromNow(config_.write_timeout_ms);
    while (!DeadlineReached(deadline)) {
        if (Serial2.available() > 0) {
            int code = Serial2.read();
            if (code != b) {
                ack_errors_.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            return true;
        }
        delay(kIdleYieldMs);
    }

    timeouts_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

ECUByte ESP32Monomotronic::ECUReadAndResponse(uint32_t timeout) {
    ECUByte val = ECURead(timeout);

    if (val.has_value()) {
        uint8_t v = val.value();
        Serial2.write(static_cast<uint8_t>(~v));
        Serial2.flush();
        return val;
    }

    return nullopt;
}

bool ESP32Monomotronic::ECUWriteWaitResponse(uint8_t b, uint32_t timeout) {
    if (!ECUWrite(b)) {
        return false;
    }

    ECUByte val = ECURead(timeout);
    if (!val.has_value()) {
        return false;
    }

    if (val.value() != static_cast<uint8_t>(~b)) {
        ack_errors_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    return true;
}

bool ESP32Monomotronic::ECUReadSequential(uint8_t *buffer, uint8_t size,
                                          uint32_t timeout) {
    for (uint8_t i = 0; i < size; ++i) {
        ECUByte r = ECUReadAndResponse(timeout);
        if (!r.has_value()) {
            return false;
        }
        buffer[i] = r.value();
    }

    return true;
}

bool ESP32Monomotronic::ECUWriteSequential(const uint8_t *data, uint8_t size,
                                           uint32_t timeout) {
    for (uint8_t i = 0; i < size; ++i) {
        if (!ECUWriteWaitResponse(data[i], timeout)) {
            return false;
        }
    }

    return true;
}

optional<ECUmmpacket> ESP32Monomotronic::ECUReadPacket(uint32_t timeout) {
    ECUmmpacket result;

    ECUByte size = ECUReadAndResponse(timeout);
    if (!size.has_value()) {
        ECUThreadErr_ = ERR_PACKET_READ;
        return nullopt;
    }
    result.size = size.value();

    if (result.size < 3 || result.size > (3 + ECUmmpacket::MAX_DATA_SIZE)) {
        ECUThreadErr_ = ERR_PACKET_SIZE;
        packet_errors_.fetch_add(1, std::memory_order_relaxed);
        return nullopt;
    }

    ECUByte counter = ECUReadAndResponse(timeout);
    if (!counter.has_value()) {
        ECUThreadErr_ = ERR_PACKET_READ;
        return nullopt;
    }
    result.counter = counter.value();

    ECUByte frameid = ECUReadAndResponse(timeout);
    if (!frameid.has_value()) {
        ECUThreadErr_ = ERR_PACKET_READ;
        return nullopt;
    }
    result.frametypeid = frameid.value();

    result.data_length = static_cast<uint8_t>(result.size - 3);
    if (result.data_length > 0 &&
        !ECUReadSequential(result.data.data(), result.data_length, timeout)) {
        ECUThreadErr_ = ERR_PACKET_READ;
        return nullopt;
    }

    ECUByte end = ECURead(timeout);
    if (!end.has_value() || end.value() != 0x03) {
        ECUThreadErr_ = ERR_PACKET_END;
        packet_errors_.fetch_add(1, std::memory_order_relaxed);
        return nullopt;
    }
    result.end = end.value();

    recordPacketActivity(false);
    return result;
}

bool ESP32Monomotronic::ECUWritePacket(uint8_t frameid, const uint8_t *data,
                                       uint8_t length, uint32_t timeout) {
    if (length > ECUmmpacket::MAX_DATA_SIZE) {
        return false;
    }

    const uint8_t packetSize = static_cast<uint8_t>(3 + length);

    if (!ECUWriteWaitResponse(packetSize, timeout)) {
        return false;
    }
    if (!ECUWriteWaitResponse(++ECUPacketCounter_, timeout)) {
        return false;
    }
    if (!ECUWriteWaitResponse(frameid, timeout)) {
        return false;
    }
    if (length > 0 && !ECUWriteSequential(data, length, timeout)) {
        return false;
    }
    if (!ECUWrite(0x03)) {
        return false;
    }

    recordPacketActivity(true);
    return true;
}

optional<ECUmmpacket> ESP32Monomotronic::getECUResponse() {
    if (ECUCommandResultAvailable_) {
        ECUCommandResultAvailable_ = false;
        return ECUResponse_;
    }
    return nullopt;
}

bool ESP32Monomotronic::sendECURequest(uint8_t frameid, const uint8_t *data,
                                       uint8_t length) {
    if (!ECUThreadCanAcceptCommands_ || length > pendingCommand_.data.size()) {
        return false;
    }

    std::lock_guard<std::mutex> lck(commandMutex_);
    pendingCommand_.frameid = frameid;
    pendingCommand_.data_length = length;
    for (uint8_t i = 0; i < length; ++i) {
        pendingCommand_.data[i] = data[i];
    }
    ECUNewCommandAvailable_ = true;
    return true;
}

bool ESP32Monomotronic::waitForResponse(optional<ECUmmpacket> &response,
                                        uint32_t timeout_ms) {
    const uint32_t deadline = DeadlineFromNow(timeout_ms);
    while (!DeadlineReached(deadline) && !stopRequested_) {
        response = getECUResponse();
        if (response.has_value()) {
            return true;
        }
        delay(kIdleYieldMs);
    }

    timeouts_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

optional<ECUResponseCollection>
ESP32Monomotronic::ECURequestData(uint8_t frameid, uint8_t expectedFrame,
                                  const uint8_t *data, uint8_t length,
                                  uint32_t timeout) {
    if (!sendECURequest(frameid, data, length)) {
        retries_.fetch_add(1, std::memory_order_relaxed);
        return nullopt;
    }

    ECUResponseCollection responses;
    uint8_t currentFrame = 0;
    do {
        optional<ECUmmpacket> packet;
        if (!waitForResponse(packet, timeout) || !packet.has_value()) {
            return nullopt;
        }

        currentFrame = packet.value().frametypeid;
        if (currentFrame != ECU_ACK_CODE && currentFrame != expectedFrame) {
            ECUThreadErr_ = ERR_PACKET_UNEXPECTED;
            packet_errors_.fetch_add(1, std::memory_order_relaxed);
            return nullopt;
        }

        if (!responses.add(packet.value())) {
            ECUThreadErr_ = ERR_PACKET_SIZE;
            packet_errors_.fetch_add(1, std::memory_order_relaxed);
            return nullopt;
        }

        if (currentFrame != ECU_ACK_CODE) {
            bool sendACK = false;
            do {
                sendACK = sendECURequest(ECU_ACK_CODE, nullptr, 0);
                if (!sendACK) {
                    delay(kIdleYieldMs);
                }
            } while (!sendACK);
        }
    } while (currentFrame != ECU_ACK_CODE);

    return responses;
}

optional<ECUResponseCollection> ESP32Monomotronic::ECUReadErrors() {
    if (!tryStartOperation(OperationKind::ReadErrors)) {
        return nullopt;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    optional<ECUResponseCollection> result =
        ECURequestData(ECU_READ_ERRORS_CODE, ECU_ERROR_DATA_CODE, nullptr, 0,
                       config_.command_timeout_ms);
    finishOperation();
    return result;
}

optional<ECUResponseCollection>
ESP32Monomotronic::ECUReadSensor(uint8_t sensorID) {
    if (!tryStartOperation(OperationKind::ReadSensor)) {
        return nullopt;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    const uint8_t request[]{0x01u, 0x00u, sensorID};
    optional<ECUResponseCollection> result =
        ECURequestData(ECU_DATA_MEMORY_READ, ECU_READ_DATA_CODE, request,
                       sizeof(request), config_.command_timeout_ms);
    finishOperation();
    return result;
}

optional<ECUResponseCollection>
ESP32Monomotronic::readECUMemory(uint8_t addressHigh, uint8_t addressLow,
                                 uint8_t length) {
    if (!tryStartOperation(OperationKind::ReadMemory)) {
        return nullopt;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    if (length == 0 || length > 0x0C) {
        ECUThreadErr_ = ERR_PACKET_SIZE;
        finishOperation();
        return nullopt;
    }

    const uint8_t request[]{length, addressHigh, addressLow};
    optional<ECUResponseCollection> result =
        ECURequestData(ECU_DATA_MEMORY_READ, ECU_READ_DATA_CODE, request,
                       sizeof(request), config_.command_timeout_ms);
    finishOperation();
    return result;
}

optional<ECUResponseCollection> ESP32Monomotronic::requestSensorCollection() {
    if (!tryStartOperation(OperationKind::ReadCollection)) {
        return nullopt;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    optional<ECUResponseCollection> result =
        ECURequestData(ECU_REQ_SENSOR_COLLECTION, ECU_RESP_SENSOR_COLLECTION,
                       nullptr, 0, config_.command_timeout_ms);
    finishOperation();
    return result;
}

optional<ECUmmpacket> ESP32Monomotronic::ECUCleanErrors() {
    if (!tryStartOperation(OperationKind::ClearErrors)) {
        return nullopt;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    if (!canAcceptCommands()) {
        finishOperation();
        return nullopt;
    }

    optional<ECUmmpacket> ecuptmp;
    const uint32_t deadline = DeadlineFromNow(config_.command_timeout_ms);

    bool sent = false;
    do {
        sent = sendECURequest(ECU_CLEAR_ERRORS_CODE, nullptr, 0);
        if (!sent) {
            delay(10);
        }
    } while (!sent && !DeadlineReached(deadline));

    if (!sent) {
        ECUThreadErr_ = ERR_CLEAR_SEND_TIMEOUT;
        timeouts_.fetch_add(1, std::memory_order_relaxed);
        finishOperation();
        return nullopt;
    }

    while (!DeadlineReached(deadline) && !stopRequested_) {
        ecuptmp = getECUResponse();
        if (ecuptmp.has_value()) {
            finishOperation();
            return ecuptmp;
        }
        delay(kIdleYieldMs);
    }

    ECUThreadErr_ = ERR_CLEAR_RESPONSE_TIMEOUT;
    timeouts_.fetch_add(1, std::memory_order_relaxed);
    finishOperation();
    return nullopt;
}

uint8_t ESP32Monomotronic::determineCollectionTable() {
    if (!tryStartOperation(OperationKind::DetermineCollectionTable)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(sessionMutex_);
    const uint8_t request[]{0x01u, 0x00u, 0xB3u};
    optional<ECUResponseCollection> response =
        ECURequestData(ECU_DATA_MEMORY_READ, ECU_READ_DATA_CODE, request,
                       sizeof(request), config_.command_timeout_ms);
    if (!response.has_value()) {
        finishOperation();
        return 0;
    }

    for (size_t i = 0; i < response.value().count; ++i) {
        const ECUmmpacket &pkt = response.value().packets[i];
        if (pkt.frametypeid == ECU_ACK_CODE || pkt.data_length == 0) {
            continue;
        }

        const uint8_t ram_b3 = pkt.data[0];
        if (ram_b3 & 0x01) {
            last_collection_table_.store(2, std::memory_order_relaxed);
            finishOperation();
            return 2;
        }
        if (ram_b3 & 0x02) {
            last_collection_table_.store(1, std::memory_order_relaxed);
            finishOperation();
            return 1;
        }
    }

    last_collection_table_.store(0, std::memory_order_relaxed);
    finishOperation();
    return 0;
}

const char *ESP32Monomotronic::errorPacketToString(const ECUmmpacket &p,
                                                   bool &present,
                                                   TextLocale locale) {
    present = false;
    if (p.frametypeid != ECU_ERROR_DATA_CODE) {
        return locale == TextLocale::PtBr ? "Nao e um pacote de erro"
                                          : "Not an error packet";
    }

    if (p.data_length < 3) {
        return locale == TextLocale::PtBr ? "Pacote de erro invalido"
                                          : "Invalid error packet";
    }

    /*
    1E - Error present?
    9E - Error intermitent?
    */
    int errcode =
        static_cast<int>(p.data[0]) | (static_cast<int>(p.data[1]) << 8);
    present = (p.data[2] == 0x1E || p.data[2] == 0x03);

    for (const ErrorCodeEntry &entry : kErrorCodeEntries) {
        if (entry.code == errcode) {
            return entry.description;
        }
    }

    return locale == TextLocale::PtBr ? "Codigo de erro desconhecido"
                                      : "Unknown error code";
}

ECUHealthSnapshot ESP32Monomotronic::getHealthSnapshot() const {
    ECUHealthSnapshot snapshot;
    snapshot.packets_sent = packets_sent_.load(std::memory_order_relaxed);
    snapshot.packets_received =
        packets_received_.load(std::memory_order_relaxed);
    snapshot.timeouts = timeouts_.load(std::memory_order_relaxed);
    snapshot.retries = retries_.load(std::memory_order_relaxed);
    snapshot.reconnects = reconnects_.load(std::memory_order_relaxed);
    snapshot.ack_errors = ack_errors_.load(std::memory_order_relaxed);
    snapshot.packet_errors = packet_errors_.load(std::memory_order_relaxed);
    snapshot.last_packet_ms = last_packet_ms_.load(std::memory_order_relaxed);
    snapshot.last_collection_table =
        last_collection_table_.load(std::memory_order_relaxed);
    return snapshot;
}

void ESP32Monomotronic::commThread(void *vpmm) {
    ESP32Monomotronic &mm = *(ESP32Monomotronic *)vpmm;
    mm.taskState_ = 0;
    mm.baudEchoOK_ = false;
    mm.ECUConnected_ = false;
    mm.initPacketsOk_ = false;
    mm.ECUThreadCanAcceptCommands_ = false;
    mm.stopRequested_ = false;

    mm.configureSerial();
    Serial2.flush();

    while (!mm.stopRequested_) {
        mm.taskState_ = 0;
        mm.ECUThreadErr_ = ERR_NONE;
        mm.resetInitPackets();
        mm.ECUConnected_ = false;
        mm.initPacketsOk_ = false;
        mm.ECUThreadCanAcceptCommands_ = false;
        mm.ECUNewCommandAvailable_ = false;
        mm.ECUCommandResultAvailable_ = false;
        mm.finishOperation();
        mm.reconnects_.fetch_add(1, std::memory_order_relaxed);

        while (Serial2.available() > 0) {
            Serial2.read();
        }
        Serial2.flush();

        mm.baudInit();

        uint8_t key1 = 0;
        uint8_t key2 = 0;
        uint8_t key3 = 0;
        uint8_t key4 = 0;
        int state = 0;
        mm.taskState_ = 1;
        const uint32_t keyDeadline = DeadlineFromNow(6000);

        while (!DeadlineReached(keyDeadline) && state != 5 &&
               !mm.stopRequested_) {
            ECUByte code = mm.ECURead(200);
            if (!code.has_value()) {
                continue;
            }

            switch (state) {
            case 0:
                if (code.value() == 0x55) {
                    ++state;
                } else if (code.value() == 0x00) {
                    mm.baudEchoOK_ = true;
                }
                break;
            case 1:
                key1 = code.value();
                (void)key1;
                ++state;
                break;
            case 2:
                key2 = code.value();
                ++state;
                break;
            case 3:
                key3 = code.value();
                (void)key3;
                ++state;
                break;
            case 4:
                key4 = code.value();
                (void)key4;
                ++state;
                break;
            default:
                break;
            }
        }

        if (state <= 1 || !mm.ECUWrite(static_cast<uint8_t>(~key2 & 0xFF))) {
            mm.debug_regiter_err(__FILE__, __LINE__);
            mm.ECUThreadErr_ =
                (state <= 1) ? ERR_HANDSHAKE_TIMEOUT : ERR_HANDSHAKE_KEY;
            delay(1000);
            continue;
        }

        mm.taskState_ = 2;
        mm.ECUConnected_ = true;

        while (true) {
            optional<ECUmmpacket> p =
                mm.ECUReadPacket(mm.config_.command_timeout_ms);
            if (!p.has_value()) {
                mm.debug_regiter_err(__FILE__, __LINE__);
                mm.ECUConnected_ = false;
                break;
            }

            updatePacketCounter(mm, p.value());
            if (!mm.appendInitPacket(p.value())) {
                mm.ECUThreadErr_ = ERR_PACKET_SIZE;
                mm.ECUConnected_ = false;
                break;
            }

            if (p.value().frametypeid == ECU_ACK_CODE) {
                break;
            }

            if (!mm.ECUWritePacket(ECU_ACK_CODE, nullptr, 0,
                                   mm.config_.write_timeout_ms)) {
                mm.debug_regiter_err(__FILE__, __LINE__);
                mm.ECUThreadErr_ = ERR_KEEPALIVE;
                mm.ECUConnected_ = false;
                break;
            }
        }

        if (!mm.ECUConnected_) {
            delay(1000);
            continue;
        }

        mm.taskState_ = 4;
        mm.initPacketsOk_ = true;
        mm.ECUThreadCanAcceptCommands_ = true;
        uint32_t lastPacketTime = millis();

        while (!mm.stopRequested_) {
            if (mm.ECUNewCommandAvailable_) {
                PendingCommand commandCopy;
                {
                    std::lock_guard<std::mutex> lck(mm.commandMutex_);
                    commandCopy = mm.pendingCommand_;
                }

                if (!mm.ECUWritePacket(
                        commandCopy.frameid, commandCopy.data.data(),
                        commandCopy.data_length, mm.config_.write_timeout_ms)) {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr_ = ERR_PACKET_WRITE;
                    mm.ECUConnected_ = false;
                    break;
                }

                optional<ECUmmpacket> response =
                    mm.ECUReadPacket(mm.config_.command_timeout_ms);
                if (!response.has_value()) {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr_ = ERR_PACKET_READ;
                    mm.ECUConnected_ = false;
                    break;
                }

                updatePacketCounter(mm, response.value());
                mm.ECUResponse_ = response.value();
                mm.ECUCommandResultAvailable_ = true;
                mm.ECUNewCommandAvailable_ = false;
                lastPacketTime = millis();
            }

            if ((millis() - lastPacketTime) > mm.config_.keep_alive_ms) {
                if (!mm.ECUWritePacket(ECU_ACK_CODE, nullptr, 0,
                                       mm.config_.write_timeout_ms)) {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr_ = ERR_KEEPALIVE;
                    mm.ECUConnected_ = false;
                    break;
                }

                optional<ECUmmpacket> ack =
                    mm.ECUReadPacket(mm.config_.command_timeout_ms);
                if (!ack.has_value() ||
                    ack.value().frametypeid !=
                        static_cast<uint8_t>(ECU_ACK_CODE)) {
                    mm.debug_regiter_err(__FILE__, __LINE__);
                    mm.ECUThreadErr_ = ERR_KEEPALIVE;
                    mm.ECUConnected_ = false;
                    break;
                }

                updatePacketCounter(mm, ack.value());
                lastPacketTime = millis();
            }

            delay(kIdleYieldMs);
        }

        mm.ECUThreadCanAcceptCommands_ = false;
        mm.initPacketsOk_ = false;
        mm.ECUConnected_ = false;
        delay(1000);
    }

    mm.ECUThreadCanAcceptCommands_ = false;
    mm.finishOperation();
    mm.inited_.store(false, std::memory_order_release);
    vTaskDelete(nullptr);
}

bool ESP32Monomotronic::init() {
    std::lock_guard<std::mutex> lock(initMutex_);
    if (inited_.load(std::memory_order_acquire)) {
        return false;
    }

    taskState_ = 0;
    ECUPacketCounter_ = 0;
    resetInitPackets();
    finishOperation();
    Task1_ = nullptr;

    const BaseType_t created = xTaskCreatePinnedToCore(
        commThread, "ECUComm", 12000, this, 32, &Task1_, 1);
    if (created != pdPASS || Task1_ == nullptr) {
        ECUThreadErr_ = ERR_TASK_CREATE;
        return false;
    }

    inited_.store(true, std::memory_order_release);
    return true;
}

ESP32Monomotronic::ESP32Monomotronic() {
    taskState_ = 0;
    ECUThreadErr_ = ERR_NONE;
    ECUThreadCanAcceptCommands_ = false;
    ECUNewCommandAvailable_ = false;
    ECUCommandResultAvailable_ = false;
    stopRequested_ = false;
}
