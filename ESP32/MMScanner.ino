/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

Firmware principal ESP32 para o scanner Bosch Monomotronic MA1.7.
*/
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFiClient.h>
#include <cstdlib>

#include "ESP32Monomotronic.h"
#include "SensorCatalog.h"
#include "WiFiConfig.h"

ESP32Monomotronic scanner;
AsyncWebServer server(80);
String serialCommandBuffer;

namespace {
enum class EndpointAccess : uint8_t { Always, ReadyOnly, Technical };

void AppendJsonEscaped(String &out, const char *text) {
    if (text == nullptr) {
        return;
    }

    while (*text) {
        const char c = *text++;
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
}

bool ParseByteValue(const String &text, uint8_t &value) {
    char *end = nullptr;
    unsigned long parsed = strtoul(text.c_str(), &end, 0);
    if (end == text.c_str() || *end != '\0' || parsed > 0xFFUL) {
        return false;
    }
    value = static_cast<uint8_t>(parsed);
    return true;
}

String PacketText(const ECUmmpacket &packet) {
    String value;
    for (uint8_t i = 0; i < packet.data_length; ++i) {
        const char c = static_cast<char>(packet.data[i]);
        if (c >= 32 && c < 127) {
            value += c;
        }
    }
    return value;
}

void AppendPacketJson(String &out, const ECUmmpacket &packet) {
    out += "{";
    out += "\"size\":";
    out += String(packet.size);
    out += ",\"counter\":";
    out += String(packet.counter);
    out += ",\"frame\":";
    out += String(packet.frametypeid);
    out += ",\"data\":[";
    for (uint8_t i = 0; i < packet.data_length; ++i) {
        if (i > 0) {
            out += ",";
        }
        out += String(packet.data[i]);
    }
    out += "]}";
}

void AppendStatusFields(String &out, const ECUStatusSnapshot &status) {
    out += "\"protocol_state\":\"";
    out += status.protocol_state;
    out += "\",\"current_operation\":\"";
    out += status.current_operation;
    out += "\",\"connected\":";
    out += status.connected ? "true" : "false";
    out += ",\"can_accept_commands\":";
    out += status.can_accept_commands ? "true" : "false";
    out += ",\"busy\":";
    out += status.busy ? "true" : "false";
    out += ",\"init_ready\":";
    out += status.init_ready ? "true" : "false";
    out += ",\"echo_byte_present\":";
    out += status.echo_byte_present ? "true" : "false";
    out += ",\"thread_started\":";
    out += status.thread_started ? "true" : "false";
    out += ",\"thread_state\":";
    out += String(status.task_state);
    out += ",\"freertos_state\":";
    out += String(status.freertos_state);
    out += ",\"error_code\":";
    out += String(status.error_code);
    out += ",\"debug_line\":";
    out += String(status.debug_line);
}

String BuildErrorJson(const char *errorCode, const char *message) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":false,";
    AppendStatusFields(result, status);
    result += ",\"error_code\":\"";
    AppendJsonEscaped(result, errorCode);
    result += "\",\"message\":\"";
    AppendJsonEscaped(result, message);
    result += "\"}";
    return result;
}

String BuildStatusJson() {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    ECUResponseCollection initPackets;
    const bool hasInitPackets = scanner.getInitPacketsSnapshot(initPackets);

    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"init_strings\":[";

    bool first = true;
    if (hasInitPackets) {
        for (size_t i = 0; i < initPackets.count; ++i) {
            const ECUmmpacket &packet = initPackets.packets[i];
            if (packet.frametypeid != ESP32Monomotronic::ECU_INIT_STRING) {
                continue;
            }
            if (!first) {
                result += ",";
            }
            result += "\"";
            const String initText = PacketText(packet);
            AppendJsonEscaped(result, initText.c_str());
            result += "\"";
            first = false;
        }
    }
    result += "]}";
    return result;
}

String BuildHealthJson() {
    const ECUHealthSnapshot health = scanner.getHealthSnapshot();
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"health\":{";
    result += "\"packets_sent\":";
    result += String(health.packets_sent);
    result += ",\"packets_received\":";
    result += String(health.packets_received);
    result += ",\"timeouts\":";
    result += String(health.timeouts);
    result += ",\"retries\":";
    result += String(health.retries);
    result += ",\"reconnects\":";
    result += String(health.reconnects);
    result += ",\"ack_errors\":";
    result += String(health.ack_errors);
    result += ",\"packet_errors\":";
    result += String(health.packet_errors);
    result += ",\"last_packet_ms\":";
    result += String(health.last_packet_ms);
    result += ",\"last_collection_table\":";
    result += String(health.last_collection_table);
    result += "}}";
    return result;
}

String BuildCatalogJson() {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"technical\":true,\"sensors\":[";
    const SensorCatalogEntry *catalog = GetSensorCatalog();
    const size_t count = GetSensorCatalogCount();

    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            result += ",";
        }
        const SensorCatalogEntry &entry = catalog[i];
        result += "{";
        result += "\"key\":\"";
        AppendJsonEscaped(result, entry.key);
        result += "\",\"name\":\"";
        AppendJsonEscaped(result, entry.display_name);
        result += "\",\"unit\":\"";
        AppendJsonEscaped(result, entry.unit);
        result += "\",\"id\":";
        result += String(entry.id);
        result += ",\"subcommand\":";
        result += String(entry.subcommand);
        result += ",\"length\":";
        result += String(entry.length);
        result += ",\"table\":";
        result += String(entry.collection_table);
        result += "}";
    }

    result += "]}";
    return result;
}

bool IsReadyForCommands(const ECUStatusSnapshot &status) {
    return status.connected && status.can_accept_commands && status.init_ready &&
           strcmp(status.protocol_state, "ready") == 0;
}

bool IsOperationAllowed(const ECUStatusSnapshot &status, EndpointAccess access,
                        int &httpCode, String &payload) {
    if (access == EndpointAccess::Always || access == EndpointAccess::Technical) {
        return true;
    }

    if (status.busy) {
        httpCode = 409;
        payload = BuildErrorJson("busy", "Another ECU operation is in flight");
        return false;
    }

    if (!IsReadyForCommands(status)) {
        httpCode = 412;
        payload = BuildErrorJson("not_ready",
                                 "Operation requires ECU ready state");
        return false;
    }

    return true;
}

String BuildErrorsJson() {
    optional<ECUResponseCollection> response = scanner.ECUReadErrors();
    if (!response.has_value()) {
        return BuildErrorJson("read_failed", "Failed to read ECU errors");
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"errors\":[";

    bool first = true;
    for (size_t i = 0; i < response.value().count; ++i) {
        const ECUmmpacket &packet = response.value().packets[i];
        if (packet.frametypeid == ESP32Monomotronic::ECU_ACK_CODE) {
            continue;
        }

        bool present = false;
        const char *description =
            ESP32Monomotronic::errorPacketToString(packet, present);

        if (!first) {
            result += ",";
        }

        result += "{";
        result += "\"present\":";
        result += present ? "true" : "false";
        result += ",\"description\":\"";
        AppendJsonEscaped(result, description);
        result += "\",\"packet\":";
        AppendPacketJson(result, packet);
        result += "}";
        first = false;
    }

    result += "]}";
    return result;
}

String BuildClearErrorsJson() {
    optional<ECUmmpacket> response = scanner.ECUCleanErrors();
    if (!response.has_value()) {
        return BuildErrorJson("clear_failed", "Failed to clear ECU errors");
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"packet\":";
    AppendPacketJson(result, response.value());
    result += "}";
    return result;
}

String BuildMemoryReadJson(uint8_t hi, uint8_t lo, uint8_t len) {
    optional<ECUResponseCollection> response = scanner.readECUMemory(hi, lo, len);
    if (!response.has_value()) {
        return BuildErrorJson("memory_read_failed",
                              "Failed to read ECU memory");
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"technical\":true,\"request\":{\"hi\":";
    result += String(hi);
    result += ",\"lo\":";
    result += String(lo);
    result += ",\"len\":";
    result += String(len);
    result += "},\"packets\":[";
    for (size_t i = 0; i < response.value().count; ++i) {
        if (i > 0) {
            result += ",";
        }
        AppendPacketJson(result, response.value().packets[i]);
    }
    result += "]}";
    return result;
}

String BuildCollectionJson(uint8_t requestedTable) {
    uint8_t tableId = requestedTable;
    if (tableId == 0) {
        tableId = scanner.determineCollectionTable();
        if (tableId == 0) {
            tableId = 1;
        }
    }

    optional<ECUResponseCollection> response = scanner.requestSensorCollection();
    if (!response.has_value()) {
        return BuildErrorJson("collection_failed",
                              "Failed to read sensor collection");
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    result += ",\"table\":";
    result += String(tableId);
    result += ",\"sensors\":[";

    const std::array<KlineEntry, kCollectionSlots> *table =
        GetCollectionTable(tableId);
    bool first = true;
    for (size_t p = 0; p < response.value().count; ++p) {
        const ECUmmpacket &packet = response.value().packets[p];
        if (packet.frametypeid == ESP32Monomotronic::ECU_ACK_CODE ||
            table == nullptr) {
            continue;
        }

        const uint8_t limit = packet.data_length < kCollectionSlots
                                  ? packet.data_length
                                  : kCollectionSlots;
        for (uint8_t i = 0; i < limit; ++i) {
            const uint8_t sensorId = (*table)[i].first;
            const uint8_t subcmd = (*table)[i].second;
            const SensorCatalogEntry *entry =
                FindSensorCatalogEntry(subcmd, sensorId);

            if (!first) {
                result += ",";
            }

            result += "{";
            result += "\"id\":";
            result += String(sensorId);
            result += ",\"subcommand\":";
            result += String(subcmd);
            result += ",\"raw\":";
            result += String(packet.data[i]);
            result += ",\"slot\":";
            result += String(i);
            if (entry != nullptr) {
                result += ",\"key\":\"";
                AppendJsonEscaped(result, entry->key);
                result += "\",\"name\":\"";
                AppendJsonEscaped(result, entry->display_name);
                result += "\",\"unit\":\"";
                AppendJsonEscaped(result, entry->unit);
                result += "\",\"value\":";
                result += String(entry->decode(packet.data[i]), 3);
            }
            result += "}";
            first = false;
        }
    }

    result += "]}";
    return result;
}

String BuildConnectJson() {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    if (strcmp(status.protocol_state, "handshaking") == 0) {
        return BuildErrorJson("already_connecting",
                              "Handshake already in progress");
    }

    if (status.thread_started) {
        return BuildErrorJson("already_running",
                              "ECU worker is already running");
    }

    const bool started = scanner.init();
    if (!started) {
        return BuildErrorJson("connect_failed", "Failed to start ECU worker");
    }

    return BuildStatusJson();
}

void SendJson(AsyncWebServerRequest *request, int httpCode,
              const String &payload) {
    request->send(httpCode, "application/json", payload);
}

void SendGuardedJson(AsyncWebServerRequest *request, EndpointAccess access,
                     String (*builder)()) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    int httpCode = 200;
    String payload;
    if (!IsOperationAllowed(status, access, httpCode, payload)) {
        SendJson(request, httpCode, payload);
        return;
    }

    SendJson(request, 200, builder());
}

void PrintSerialHelp() {
    Serial.println("Commands: help, status, connect, health, errors, clear, "
                   "catalog, f4 [table], mem <hi> <lo> <len>");
}

void HandleSerialCommand(String command) {
    command.trim();
    if (command.length() == 0) {
        return;
    }

    if (command == "help") {
        PrintSerialHelp();
        return;
    }

    if (command == "status") {
        Serial.println(BuildStatusJson());
        return;
    }

    if (command == "health") {
        Serial.println(BuildHealthJson());
        return;
    }

    if (command == "connect") {
        Serial.println(BuildConnectJson());
        return;
    }

    if (command == "catalog") {
        Serial.println(BuildCatalogJson());
        return;
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    int httpCode = 200;
    String payload;
    if ((command == "errors" || command == "clear" || command.startsWith("f4") ||
         command.startsWith("mem ")) &&
        !IsOperationAllowed(status, EndpointAccess::ReadyOnly, httpCode, payload)) {
        Serial.println(payload);
        return;
    }

    if (command == "errors") {
        Serial.println(BuildErrorsJson());
        return;
    }

    if (command == "clear") {
        Serial.println(BuildClearErrorsJson());
        return;
    }

    if (command.startsWith("f4")) {
        uint8_t table = 0;
        const int space = command.indexOf(' ');
        if (space > 0) {
            ParseByteValue(command.substring(space + 1), table);
        }
        Serial.println(BuildCollectionJson(table));
        return;
    }

    if (command.startsWith("mem ")) {
        const int first = command.indexOf(' ');
        const int second = command.indexOf(' ', first + 1);
        const int third = command.indexOf(' ', second + 1);
        if (first < 0 || second < 0 || third < 0) {
            Serial.println(BuildErrorJson("invalid_request",
                                          "usage: mem <hi> <lo> <len>"));
            return;
        }

        uint8_t hi = 0;
        uint8_t lo = 0;
        uint8_t len = 0;
        if (!ParseByteValue(command.substring(first + 1, second), hi) ||
            !ParseByteValue(command.substring(second + 1, third), lo) ||
            !ParseByteValue(command.substring(third + 1), len)) {
            Serial.println(BuildErrorJson("invalid_request",
                                          "invalid mem parameters"));
            return;
        }
        Serial.println(BuildMemoryReadJson(hi, lo, len));
        return;
    }

    Serial.println(BuildErrorJson("unknown_command", "Unknown command"));
}

void ConfigureRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/app.js", "application/javascript");
    });

    server.on("/ram", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildStatusJson());
    });

    server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildConnectJson());
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildStatusJson());
    });

    server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildHealthJson());
    });

    server.on("/api/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildConnectJson());
    });

    server.on("/api/sensors/catalog", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  SendGuardedJson(request, EndpointAccess::Technical,
                                  BuildCatalogJson);
              });

    server.on("/api/errors", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendGuardedJson(request, EndpointAccess::ReadyOnly, BuildErrorsJson);
    });

    server.on("/api/errors/clear", HTTP_POST,
              [](AsyncWebServerRequest *request) {
                  SendGuardedJson(request, EndpointAccess::ReadyOnly,
                                  BuildClearErrorsJson);
              });

    server.on("/api/memory/read", HTTP_GET, [](AsyncWebServerRequest *request) {
        const ECUStatusSnapshot status = scanner.getStatusSnapshot();
        int httpCode = 200;
        String payload;
        if (!IsOperationAllowed(status, EndpointAccess::ReadyOnly, httpCode,
                                payload)) {
            SendJson(request, httpCode, payload);
            return;
        }

        if (!request->hasParam("hi") || !request->hasParam("lo") ||
            !request->hasParam("len")) {
            SendJson(request, 400,
                     BuildErrorJson("invalid_request", "missing hi/lo/len"));
            return;
        }

        uint8_t hi = 0;
        uint8_t lo = 0;
        uint8_t len = 0;
        if (!ParseByteValue(request->getParam("hi")->value(), hi) ||
            !ParseByteValue(request->getParam("lo")->value(), lo) ||
            !ParseByteValue(request->getParam("len")->value(), len)) {
            SendJson(request, 400, BuildErrorJson("invalid_request",
                                                  "invalid hi/lo/len"));
            return;
        }

        SendJson(request, 200, BuildMemoryReadJson(hi, lo, len));
    });

    server.on("/api/sensors/collection", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  const ECUStatusSnapshot status = scanner.getStatusSnapshot();
                  int httpCode = 200;
                  String payload;
                  if (!IsOperationAllowed(status, EndpointAccess::ReadyOnly,
                                          httpCode, payload)) {
                      SendJson(request, httpCode, payload);
                      return;
                  }

                  uint8_t table = 0;
                  if (request->hasParam("table")) {
                      ParseByteValue(request->getParam("table")->value(), table);
                  }
                  SendJson(request, 200, BuildCollectionJson(table));
              });
}
} // namespace

void setup() {
    const ESP32ScannerConfig config{};
    scanner.setConfig(config);

    pinMode(config.tx_init_pin, OUTPUT);
    pinMode(config.aux_init_pin, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    Serial2.begin(config.session_baud, SERIAL_8N1);

    serialCommandBuffer.reserve(128);

    if (!SPIFFS.begin()) {
        Serial.println("An error occurred while mounting SPIFFS");
    }

    WiFi.begin(ssid, password);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setting AP (Access Point)...");
    WiFi.softAP("Test_WiFi", "d3e4a0b1c2");

    const IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);

    ConfigureRoutes();
    server.begin();

    Serial.println("HTTP server started");
    PrintSerialHelp();
}

void loop() {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (serialCommandBuffer.length() > 0) {
                HandleSerialCommand(serialCommandBuffer);
                serialCommandBuffer = "";
            }
        } else {
            serialCommandBuffer += c;
        }
    }
    delay(1);
}
