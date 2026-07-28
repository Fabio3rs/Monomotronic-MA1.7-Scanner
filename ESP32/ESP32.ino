/*
https://github.com/Fabio3rs/Monomotronic-MA1.7-Scanner

Firmware principal ESP32 para o scanner Bosch Monomotronic MA1.7.
*/
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFiClient.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "ESP32Monomotronic.h"
#include "SensorCatalog.h"
#include "WiFiConfig.h"

ESP32Monomotronic scanner;
AsyncWebServer server(80);
DNSServer captiveDnsServer;
String serialCommandBuffer;
std::atomic<bool> technicalModeEnabled{false};

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

namespace {
enum class EndpointAccess : uint8_t { Always, ReadyOnly, Technical };
constexpr size_t kBaseJsonReserve = 256;
constexpr size_t kPacketJsonReserve = 160;
constexpr size_t kSensorJsonReserve = 112;
constexpr size_t kCatalogEntryJsonReserve = 104;
constexpr size_t kErrorJsonReserve = 128;
constexpr uint16_t kCaptiveDnsPort = 53;
constexpr uint32_t kCaptiveDnsTtlSeconds = 60;
bool captiveDnsRunning = false;
bool spiffsMounted = false;

struct LocalizedText {
    const char *en;
    const char *pt_br;
};

struct KnownEcuProfile {
    const char *id;
    uint32_t session_baud;
    LocalizedText label;
};

constexpr KnownEcuProfile kKnownEcuProfiles[] = {
    {"fiat-tipo-1.6ie",
     4800,
     {"Fiat Tipo 1.6ie (Bosch MA1.7) - 4800 baud",
      "Fiat Tipo 1.6ie (Bosch MA1.7) - 4800 baud"}},
    {"renault-clio-1.6-1999",
     9600,
     {"Renault Clio 1.6 (1999) - 9600 baud",
      "Renault Clio 1.6 (1999) - 9600 baud"}},
};

constexpr size_t kKnownEcuProfileCount =
    sizeof(kKnownEcuProfiles) / sizeof(kKnownEcuProfiles[0]);

const KnownEcuProfile *selectedProfile = &kKnownEcuProfiles[0];

TextLocale ResolveLocale(const String &locale) {
    String normalized = locale;
    normalized.toLowerCase();
    normalized.replace('_', '-');
    return normalized.startsWith("pt") ? TextLocale::PtBr : TextLocale::En;
}

TextLocale ResolveRequestLocale(AsyncWebServerRequest *request) {
    if (request != nullptr && request->hasParam("lang")) {
        return ResolveLocale(request->getParam("lang")->value());
    }
    return TextLocale::En;
}

const char *Localize(TextLocale locale, const LocalizedText &text) {
    return locale == TextLocale::PtBr ? text.pt_br : text.en;
}

const KnownEcuProfile &GetDefaultProfile() { return kKnownEcuProfiles[0]; }

const KnownEcuProfile &GetSelectedProfile() {
    return selectedProfile != nullptr ? *selectedProfile : GetDefaultProfile();
}

const KnownEcuProfile *FindKnownProfileById(const String &id) {
    for (size_t i = 0; i < kKnownEcuProfileCount; ++i) {
        if (id == kKnownEcuProfiles[i].id) {
            return &kKnownEcuProfiles[i];
        }
    }
    return nullptr;
}

void ApplyKnownProfile(const KnownEcuProfile &profile) {
    ESP32ScannerConfig config = scanner.getConfig();
    config.session_baud = profile.session_baud;
    scanner.setConfig(config);
    selectedProfile = &profile;
}

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

void AppendUnsigned(String &out, unsigned long value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lu", value);
    out += buffer;
}

void AppendSigned(String &out, long value) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    out += buffer;
}

void AppendFloat3(String &out, float value) {
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(value));
    out += buffer;
}

void AppendBool(String &out, bool value) { out += value ? "true" : "false"; }

String BuildCaptivePortalUrl() {
    return String("http://") + WiFi.softAPIP().toString() + "/";
}

String BuildScannerApUrl() {
    return BuildCaptivePortalUrl();
}

bool IsWebUiAvailable() {
    return spiffsMounted && SPIFFS.exists("/index.html") && SPIFFS.exists("/app.js");
}

bool IsCaptiveApActive() {
    const wifi_mode_t mode = WiFi.getMode();
    return captiveDnsRunning &&
           (mode == WIFI_AP || mode == WIFI_AP_STA) &&
           WiFi.softAPIP()[0] != 0;
}

bool IsApiPath(const String &path) { return path.startsWith("/api/"); }

void AddNoCacheHeaders(AsyncWebServerResponse *response) {
    if (response == nullptr) {
        return;
    }
    response->addHeader("Cache-Control",
                        "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
}

void RedirectToCaptivePortal(AsyncWebServerRequest *request) {
    const String redirectUrl = BuildCaptivePortalUrl();
    AsyncWebServerResponse *response =
        request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", redirectUrl);
    AddNoCacheHeaders(response);
    request->send(response);
}

void SendCaptivePortalPage(AsyncWebServerRequest *request) {
    const String redirectUrl = BuildCaptivePortalUrl();
    String html;
    html.reserve(1024 + redirectUrl.length());
    html = "<!doctype html><html><head><meta charset=\"utf-8\">"
           "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
           "<style>"
           ":root{color-scheme:light;font-family:Segoe UI,Tahoma,sans-serif;}"
           "body{margin:0;background:linear-gradient(180deg,#f6f2e8,#f1efe7);color:#1f2a30;}"
           ".shell{max-width:560px;margin:0 auto;padding:24px 16px;}"
           ".card{background:#fffaf0;border:1px solid #d8cfbb;border-radius:20px;padding:20px;"
           "box-shadow:0 12px 30px rgba(40,47,40,.08);}"
           "h1{margin:0 0 8px;font-size:1.4rem;}p{line-height:1.45;margin:0 0 12px;}"
           ".meta{color:#6e756f;font-size:.95rem;}.link{display:inline-block;margin-top:8px;"
           "padding:10px 14px;border-radius:999px;background:#0f6c7c;color:#fff;text-decoration:none;}"
           ".mono{font-family:Consolas,monospace;word-break:break-all;}</style>"
           "<meta http-equiv=\"refresh\" content=\"0;url=";
    html += redirectUrl;
    html += "\"><title>MA1.7 Scanner</title></head><body><div class=\"shell\"><div class=\"card\">"
            "<h1>MA1.7 ESP32 Scanner</h1>"
            "<p>Abrindo a interface web do scanner.</p>"
            "<p class=\"meta\">Se a pagina principal nao abrir sozinha, toque no botao abaixo.</p>"
            "<p class=\"mono\">";
    html += redirectUrl;
    html += "</p><p><a class=\"link\" href=\"";
    html += redirectUrl;
    html += "\">Abrir interface do scanner</a></p></div></div>"
            "<script>location.replace('";
    html += redirectUrl;
    html += "');</script></body></html>";

    AsyncWebServerResponse *response =
        request->beginResponse(200, "text/html", html);
    AddNoCacheHeaders(response);
    request->send(response);
}

void SendFallbackUiPage(AsyncWebServerRequest *request) {
    const String apUrl = BuildScannerApUrl();
    const String apIp = WiFi.softAPIP().toString();

    String html;
    html.reserve(1600 + apUrl.length() + apIp.length());
    html = "<!doctype html><html><head><meta charset=\"utf-8\">"
           "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
           "<title>MA1.7 ESP32 Scanner</title>"
           "<style>"
           ":root{font-family:Segoe UI,Tahoma,sans-serif;color-scheme:light;}"
           "body{margin:0;background:linear-gradient(180deg,#f6f2e8,#f1efe7);color:#1f2a30;}"
           ".shell{max-width:680px;margin:0 auto;padding:24px 16px 32px;}"
           ".card{background:#fffaf0;border:1px solid #d8cfbb;border-radius:20px;padding:20px;"
           "box-shadow:0 12px 30px rgba(40,47,40,.08);}"
           "h1{margin:0 0 10px;font-size:1.45rem;}h2{margin:18px 0 8px;font-size:1rem;}"
           "p,li{line-height:1.5;}ul{padding-left:18px;margin:10px 0;}"
           ".pill{display:inline-block;padding:6px 10px;border-radius:999px;background:#d9efe8;"
           "color:#0f6c7c;margin:0 8px 8px 0;}.mono{font-family:Consolas,monospace;word-break:break-all;}"
           ".warn{background:#fff0ed;border:1px solid #efc0b6;color:#aa3d2d;border-radius:14px;padding:12px;}"
           ".link{display:inline-block;margin-top:10px;padding:10px 14px;border-radius:999px;"
           "background:#0f6c7c;color:#fff;text-decoration:none;}</style></head><body><div class=\"shell\">"
           "<div class=\"card\"><h1>MA1.7 ESP32 Scanner</h1>"
           "<div><span class=\"pill\">SSID ";
    html += kWifiApSsid;
    html += "</span><span class=\"pill\">IP ";
    html += apIp;
    html += "</span></div>"
            "<p class=\"warn\">A interface web completa nao foi encontrada no SPIFFS deste ESP32.</p>"
            "<p>O captive portal e o ponto de acesso estao funcionando, mas os arquivos web ainda nao foram gravados na particao de filesystem.</p>"
            "<h2>Como corrigir</h2>"
            "<ul><li>Grave o firmware normalmente.</li>"
            "<li>Depois rode o upload do filesystem com <span class=\"mono\">pio run -e esp32dev -t uploadfs</span>.</li>"
            "<li>Recarregue esta pagina apos o upload.</li></ul>"
            "<h2>Acesso manual</h2><p class=\"mono\">";
    html += apUrl;
    html += "</p><a class=\"link\" href=\"";
    html += apUrl;
    html += "\">Tentar abrir a pagina principal</a></div></div></body></html>";

    AsyncWebServerResponse *response =
        request->beginResponse(200, "text/html", html);
    AddNoCacheHeaders(response);
    request->send(response);
}

void SendBootConfigJs(AsyncWebServerRequest *request) {
    String js;
    js.reserve(256);
    js = "window.__MA17_BOOT__={\"apSsid\":\"";
    AppendJsonEscaped(js, kWifiApSsid);
    js += "\",\"apIp\":\"";
    AppendJsonEscaped(js, WiFi.softAPIP().toString().c_str());
    js += "\",\"apUrl\":\"";
    AppendJsonEscaped(js, BuildScannerApUrl().c_str());
    js += "\",\"webUiAvailable\":";
    AppendBool(js, IsWebUiAvailable());
    js += "};";
    AsyncWebServerResponse *response =
        request->beginResponse(200, "application/javascript", js);
    AddNoCacheHeaders(response);
    request->send(response);
}

void StartCaptiveDns() {
    captiveDnsServer.stop();
    captiveDnsServer.setTTL(kCaptiveDnsTtlSeconds);
    captiveDnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    captiveDnsRunning =
        captiveDnsServer.start(kCaptiveDnsPort, "*", WiFi.softAPIP());

    if (!captiveDnsRunning) {
        Serial.println("[DNS] Failed to start captive DNS");
        return;
    }

    Serial.println("[DNS] Captive DNS active");
    Serial.printf("[DNS] Port: %u\n", kCaptiveDnsPort);
    Serial.printf("[DNS] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
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

bool ParseBoolValue(const String &text, bool &value) {
    if (text == "1" || text == "true" || text == "on") {
        value = true;
        return true;
    }
    if (text == "0" || text == "false" || text == "off") {
        value = false;
        return true;
    }
    return false;
}

void AppendPacketTextEscaped(String &out, const ECUmmpacket &packet) {
    for (uint8_t i = 0; i < packet.data_length; ++i) {
        const char c = static_cast<char>(packet.data[i]);
        if (c >= 32 && c < 127) {
            switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            default:
                out += c;
                break;
            }
        }
    }
}

bool IsReadyForCommands(const ECUStatusSnapshot &status);

size_t EstimatePacketJsonCapacity(const ECUmmpacket &packet) {
    return kPacketJsonReserve + static_cast<size_t>(packet.data_length) * 5;
}

void AppendPacketJson(String &out, const ECUmmpacket &packet) {
    out += "{";
    out += "\"size\":";
    AppendUnsigned(out, packet.size);
    out += ",\"counter\":";
    AppendUnsigned(out, packet.counter);
    out += ",\"frame\":";
    AppendUnsigned(out, packet.frametypeid);
    out += ",\"data\":[";
    for (uint8_t i = 0; i < packet.data_length; ++i) {
        if (i > 0) {
            out += ",";
        }
        AppendUnsigned(out, packet.data[i]);
    }
    out += "]}";
}

void AppendStatusFields(String &out, const ECUStatusSnapshot &status) {
    const KnownEcuProfile &profile = GetSelectedProfile();
    out += "\"protocol_state\":\"";
    out += status.protocol_state;
    out += "\",\"current_operation\":\"";
    out += status.current_operation;
    out += "\",\"connected\":";
    AppendBool(out, status.connected);
    out += ",\"can_accept_commands\":";
    AppendBool(out, status.can_accept_commands);
    out += ",\"busy\":";
    AppendBool(out, status.busy);
    out += ",\"init_ready\":";
    AppendBool(out, status.init_ready);
    out += ",\"echo_byte_present\":";
    AppendBool(out, status.echo_byte_present);
    out += ",\"thread_started\":";
    AppendBool(out, status.thread_started);
    out += ",\"thread_state\":";
    AppendSigned(out, status.task_state);
    out += ",\"freertos_state\":";
    AppendSigned(out, status.freertos_state);
    out += ",\"error_code\":";
    AppendSigned(out, status.error_code);
    out += ",\"debug_line\":";
    AppendSigned(out, status.debug_line);
    out += ",\"technical_mode_enabled\":";
    AppendBool(out, technicalModeEnabled.load(std::memory_order_relaxed));
    out += ",\"selected_profile_id\":\"";
    AppendJsonEscaped(out, profile.id);
    out += "\",\"session_baud\":";
    AppendUnsigned(out, profile.session_baud);
}

void AppendResponseMeta(String &out, const ECUStatusSnapshot &status) {
    const ECUHealthSnapshot health = scanner.getHealthSnapshot();
    const uint32_t now = millis();
    const wl_status_t wifiStatus = WiFi.status();
    const bool wifiConnected = wifiStatus == WL_CONNECTED;

    out += ",\"meta\":{";
    out += "\"generated_at_ms\":";
    AppendUnsigned(out, now);
    out += ",\"uptime_ms\":";
    AppendUnsigned(out, now);
    out += ",\"free_heap_bytes\":";
    AppendUnsigned(out, ESP.getFreeHeap());
    out += ",\"wifi_connected\":";
    AppendBool(out, wifiConnected);
    out += ",\"ap_active\":";
    AppendBool(out, WiFi.softAPIP()[0] != 0);
    out += ",\"ap_ssid\":\"";
    AppendJsonEscaped(out, kWifiApSsid);
    out += "\",\"ap_ip\":\"";
    AppendJsonEscaped(out, WiFi.softAPIP().toString().c_str());
    out += "\",\"web_ui_available\":";
    AppendBool(out, IsWebUiAvailable());
    out += ",\"wifi_rssi_dbm\":";
    if (wifiConnected) {
        AppendSigned(out, WiFi.RSSI());
    } else {
        out += "null";
    }
    out += ",\"ready_for_commands\":";
    AppendBool(out, IsReadyForCommands(status));
    out += ",\"last_packet_age_ms\":";
    if (health.last_packet_ms == 0 || health.last_packet_ms > now) {
        out += "null";
    } else {
        AppendUnsigned(out, now - health.last_packet_ms);
    }
    out += "}";
}

String BuildErrorJson(const char *errorCode, const char *message) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve + strlen(errorCode) + strlen(message));
    result = "{\"ok\":false,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"error_code\":\"";
    AppendJsonEscaped(result, errorCode);
    result += "\",\"message\":\"";
    AppendJsonEscaped(result, message);
    result += "\"}";
    return result;
}

String BuildErrorJson(const char *errorCode, TextLocale locale,
                      const LocalizedText &message) {
    return BuildErrorJson(errorCode, Localize(locale, message));
}

String BuildStatusJson() {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    ECUResponseCollection initPackets;
    const bool hasInitPackets = scanner.getInitPacketsSnapshot(initPackets);

    String result;
    result.reserve(kBaseJsonReserve + initPackets.count * 48);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
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
            AppendPacketTextEscaped(result, packet);
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
    String result;
    result.reserve(kBaseJsonReserve + 160);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"health\":{";
    result += "\"packets_sent\":";
    AppendUnsigned(result, health.packets_sent);
    result += ",\"packets_received\":";
    AppendUnsigned(result, health.packets_received);
    result += ",\"timeouts\":";
    AppendUnsigned(result, health.timeouts);
    result += ",\"retries\":";
    AppendUnsigned(result, health.retries);
    result += ",\"reconnects\":";
    AppendUnsigned(result, health.reconnects);
    result += ",\"ack_errors\":";
    AppendUnsigned(result, health.ack_errors);
    result += ",\"packet_errors\":";
    AppendUnsigned(result, health.packet_errors);
    result += ",\"last_packet_ms\":";
    AppendUnsigned(result, health.last_packet_ms);
    result += ",\"last_collection_table\":";
    AppendUnsigned(result, health.last_collection_table);
    result += "}}";
    return result;
}

String BuildProfilesJson(TextLocale locale) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve +
                   kKnownEcuProfileCount * kCatalogEntryJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"default_profile_id\":\"";
    AppendJsonEscaped(result, GetDefaultProfile().id);
    result += "\",\"profiles\":[";

    for (size_t i = 0; i < kKnownEcuProfileCount; ++i) {
        if (i > 0) {
            result += ",";
        }
        result += "{";
        result += "\"id\":\"";
        AppendJsonEscaped(result, kKnownEcuProfiles[i].id);
        result += "\",\"label\":\"";
        AppendJsonEscaped(result, Localize(locale, kKnownEcuProfiles[i].label));
        result += "\",\"session_baud\":";
        AppendUnsigned(result, kKnownEcuProfiles[i].session_baud);
        result += "}";
    }

    result += "]}";
    return result;
}

String BuildCatalogJson(TextLocale locale) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve +
                   GetSensorCatalogCount() * kCatalogEntryJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
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
        AppendJsonEscaped(result, GetSensorDisplayName(entry, locale));
        result += "\",\"unit\":\"";
        AppendJsonEscaped(result, entry.unit);
        result += "\",\"id\":";
        AppendUnsigned(result, entry.id);
        result += ",\"subcommand\":";
        AppendUnsigned(result, entry.subcommand);
        result += ",\"length\":";
        AppendUnsigned(result, entry.length);
        result += ",\"table\":";
        AppendUnsigned(result, entry.collection_table);
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
                        TextLocale locale, int &httpCode, String &payload) {
    if (access == EndpointAccess::Always) {
        return true;
    }

    if (access == EndpointAccess::Technical &&
        !technicalModeEnabled.load(std::memory_order_relaxed)) {
        httpCode = 403;
        payload = BuildErrorJson(
            "technical_mode_disabled", locale,
            {"Enable technical mode before using this endpoint",
             "Habilite o modo tecnico antes de usar este endpoint"});
        return false;
    }

    if (access == EndpointAccess::Technical) {
        return true;
    }

    if (status.busy) {
        httpCode = 409;
        payload = BuildErrorJson("busy", locale,
                                 {"Another ECU operation is in flight",
                                  "Outra operacao da ECU esta em andamento"});
        return false;
    }

    if (!IsReadyForCommands(status)) {
        httpCode = 412;
        payload = BuildErrorJson("not_ready", locale,
                                 {"Operation requires ECU ready state",
                                  "A operacao exige a ECU em estado pronto"});
        return false;
    }

    return true;
}

String BuildErrorsJson(TextLocale locale) {
    optional<ECUResponseCollection> response = scanner.ECUReadErrors();
    if (!response.has_value()) {
        if (scanner.isBusy()) {
            return BuildErrorJson("busy", locale,
                                  {"Another ECU operation is in flight",
                                   "Outra operacao da ECU esta em andamento"});
        }
        return BuildErrorJson("read_failed", locale,
                              {"Failed to read ECU errors",
                               "Falha ao ler os erros da ECU"});
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve +
                   response.value().count * kErrorJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"errors\":[";

    bool first = true;
    for (size_t i = 0; i < response.value().count; ++i) {
        const ECUmmpacket &packet = response.value().packets[i];
        if (packet.frametypeid == ESP32Monomotronic::ECU_ACK_CODE) {
            continue;
        }

        bool present = false;
        const char *description =
            ESP32Monomotronic::errorPacketToString(packet, present, locale);

        if (!first) {
            result += ",";
        }

        result += "{";
        result += "\"present\":";
        AppendBool(result, present);
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

String BuildClearErrorsJson(TextLocale locale) {
    optional<ECUmmpacket> response = scanner.ECUCleanErrors();
    if (!response.has_value()) {
        if (scanner.isBusy()) {
            return BuildErrorJson("busy", locale,
                                  {"Another ECU operation is in flight",
                                   "Outra operacao da ECU esta em andamento"});
        }
        return BuildErrorJson("clear_failed", locale,
                              {"Failed to clear ECU errors",
                               "Falha ao limpar os erros da ECU"});
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve + EstimatePacketJsonCapacity(response.value()));
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"packet\":";
    AppendPacketJson(result, response.value());
    result += "}";
    return result;
}

String BuildMemoryReadJson(TextLocale locale, uint8_t hi, uint8_t lo,
                           uint8_t len) {
    optional<ECUResponseCollection> response = scanner.readECUMemory(hi, lo, len);
    if (!response.has_value()) {
        if (scanner.isBusy()) {
            return BuildErrorJson("busy", locale,
                                  {"Another ECU operation is in flight",
                                   "Outra operacao da ECU esta em andamento"});
        }
        return BuildErrorJson("memory_read_failed", locale,
                              {"Failed to read ECU memory",
                               "Falha ao ler a memoria da ECU"});
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve +
                   response.value().count * kPacketJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"technical\":true,\"request\":{\"hi\":";
    AppendUnsigned(result, hi);
    result += ",\"lo\":";
    AppendUnsigned(result, lo);
    result += ",\"len\":";
    AppendUnsigned(result, len);
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

String BuildCollectionJson(TextLocale locale) {
    uint8_t tableId = scanner.determineCollectionTable();
    if (tableId == 0) {
        if (scanner.isBusy()) {
            return BuildErrorJson(
                "busy", locale,
                {"Another ECU operation is in flight",
                 "Outra operacao da ECU esta em andamento"});
        }
        tableId = 1;
    }

    optional<ECUResponseCollection> response = scanner.requestSensorCollection();
    if (!response.has_value()) {
        if (scanner.isBusy()) {
            return BuildErrorJson("busy", locale,
                                  {"Another ECU operation is in flight",
                                   "Outra operacao da ECU esta em andamento"});
        }
        return BuildErrorJson("collection_failed", locale,
                              {"Failed to read sensor collection",
                               "Falha ao ler a coleta de sensores"});
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve +
                   response.value().count * kCollectionSlots * kSensorJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"table\":";
    AppendUnsigned(result, tableId);
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
            AppendUnsigned(result, sensorId);
            result += ",\"subcommand\":";
            AppendUnsigned(result, subcmd);
            result += ",\"raw\":";
            AppendUnsigned(result, packet.data[i]);
            result += ",\"slot\":";
            AppendUnsigned(result, i);
            if (entry != nullptr) {
                result += ",\"key\":\"";
                AppendJsonEscaped(result, entry->key);
                result += "\",\"name\":\"";
                AppendJsonEscaped(result, GetSensorDisplayName(*entry, locale));
                result += "\",\"unit\":\"";
                AppendJsonEscaped(result, entry->unit);
                result += "\",\"value\":";
                AppendFloat3(result, entry->decode(packet.data[i]));
            }
            result += "}";
            first = false;
        }
    }

    result += "]}";
    return result;
}

String BuildConnectJson(TextLocale locale,
                        const KnownEcuProfile *requestedProfile = nullptr) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    if (strcmp(status.protocol_state, "handshaking") == 0) {
        return BuildErrorJson("already_connecting", locale,
                              {"Handshake already in progress",
                               "O handshake ja esta em andamento"});
    }

    if (status.thread_started) {
        return BuildErrorJson("already_running", locale,
                              {"ECU worker is already running",
                               "A thread da ECU ja esta em execucao"});
    }

    if (requestedProfile != nullptr) {
        ApplyKnownProfile(*requestedProfile);
    }

    const bool started = scanner.init();
    if (!started) {
        return BuildErrorJson("connect_failed", locale,
                              {"Failed to start ECU worker",
                               "Falha ao iniciar a thread da ECU"});
    }

    return BuildStatusJson();
}

String BuildTechnicalModeJson(bool enabled) {
    technicalModeEnabled.store(enabled, std::memory_order_relaxed);
    return BuildStatusJson();
}

String BuildRebootJson(TextLocale locale) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    String result;
    result.reserve(kBaseJsonReserve);
    result = "{\"ok\":true,";
    AppendStatusFields(result, status);
    AppendResponseMeta(result, status);
    result += ",\"message\":\"";
    AppendJsonEscaped(result, Localize(locale, {"ESP32 reboot requested",
                                                "Reinicio do ESP32 solicitado"}));
    result += "\"}";
    return result;
}

void SendJson(AsyncWebServerRequest *request, int httpCode,
              const String &payload) {
    request->send(httpCode, "application/json", payload);
}

void SendGuardedJson(AsyncWebServerRequest *request, EndpointAccess access,
                     String (*builder)(TextLocale)) {
    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    const TextLocale locale = ResolveRequestLocale(request);
    int httpCode = 200;
    String payload;
    if (!IsOperationAllowed(status, access, locale, httpCode, payload)) {
        SendJson(request, httpCode, payload);
        return;
    }

    SendJson(request, 200, builder(locale));
}

void PrintSerialHelp() {
    Serial.println("Commands: help, status, connect, health, errors, clear, "
                   "catalog, f4, mem <hi> <lo> <len>");
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
        Serial.println(BuildConnectJson(TextLocale::En));
        return;
    }

    if (command == "catalog") {
        Serial.println(BuildCatalogJson(TextLocale::En));
        return;
    }

    const ECUStatusSnapshot status = scanner.getStatusSnapshot();
    int httpCode = 200;
    String payload;
    if ((command == "errors" || command == "clear" || command.startsWith("f4") ||
         command.startsWith("mem ")) &&
        !IsOperationAllowed(status, EndpointAccess::ReadyOnly, TextLocale::En,
                            httpCode, payload)) {
        Serial.println(payload);
        return;
    }

    if (command == "errors") {
        Serial.println(BuildErrorsJson(TextLocale::En));
        return;
    }

    if (command == "clear") {
        Serial.println(BuildClearErrorsJson(TextLocale::En));
        return;
    }

    if (command.startsWith("f4")) {
        Serial.println(BuildCollectionJson(TextLocale::En));
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
        Serial.println(BuildMemoryReadJson(TextLocale::En, hi, lo, len));
        return;
    }

    Serial.println(BuildErrorJson("unknown_command", "Unknown command"));
}

void ConfigureRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (IsWebUiAvailable()) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/index.html", "text/html");
            AddNoCacheHeaders(response);
            request->send(response);
            return;
        }
        SendFallbackUiPage(request);
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (IsWebUiAvailable()) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/app.js",
                                       "application/javascript");
            AddNoCacheHeaders(response);
            request->send(response);
            return;
        }
        AsyncWebServerResponse *response =
            request->beginResponse(
                503, "application/javascript",
                "console.error('MA1.7 web UI unavailable: uploadfs required.');");
        AddNoCacheHeaders(response);
        request->send(response);
    });

    server.on("/manifest.webmanifest", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  if (IsWebUiAvailable()) {
                      AsyncWebServerResponse *response =
                          request->beginResponse(SPIFFS,
                                                 "/manifest.webmanifest",
                                                 "application/manifest+json");
                      AddNoCacheHeaders(response);
                      request->send(response);
                      return;
                  }
                  request->send(404, "text/plain", "manifest unavailable");
              });

    server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (IsWebUiAvailable()) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/favicon.svg",
                                       "image/svg+xml");
            AddNoCacheHeaders(response);
            request->send(response);
            return;
        }
        request->send(404, "text/plain", "favicon unavailable");
    });

    server.on("/icon.svg", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (IsWebUiAvailable()) {
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, "/icon.svg", "image/svg+xml");
            AddNoCacheHeaders(response);
            request->send(response);
            return;
        }
        request->send(404, "text/plain", "icon unavailable");
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse(
            302, "text/plain", "");
        response->addHeader("Location", "/favicon.svg");
        AddNoCacheHeaders(response);
        request->send(response);
    });

    server.on("/boot.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendBootConfigJs(request);
    });

    server.on("/ram", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%lu",
                 static_cast<unsigned long>(ESP.getFreeHeap()));
        request->send(200, "text/plain", buffer);
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildStatusJson());
    });

    server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request) {
        const TextLocale locale = ResolveRequestLocale(request);
        const KnownEcuProfile *profile = nullptr;
        if (request->hasParam("profile")) {
            profile = FindKnownProfileById(request->getParam("profile")->value());
            if (profile == nullptr) {
                SendJson(request, 400,
                         BuildErrorJson("invalid_request", locale,
                                        {"invalid profile",
                                         "perfil invalido"}));
                return;
            }
        }
        SendJson(request, 200, BuildConnectJson(locale, profile));
    });

    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendCaptivePortalPage(request);
    });

    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendCaptivePortalPage(request);
    });

    server.on("/hotspot-detect.html", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  SendCaptivePortalPage(request);
              });

    server.on("/connecttest.txt", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  SendCaptivePortalPage(request);
              });

    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendCaptivePortalPage(request);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildStatusJson());
    });

    server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendJson(request, 200, BuildHealthJson());
    });

    server.on("/api/config/profiles", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  SendJson(request, 200,
                           BuildProfilesJson(ResolveRequestLocale(request)));
              });

    server.on("/api/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        const TextLocale locale = ResolveRequestLocale(request);
        const KnownEcuProfile *profile = nullptr;
        if (request->hasParam("profile")) {
            profile = FindKnownProfileById(request->getParam("profile")->value());
            if (profile == nullptr) {
                SendJson(request, 400,
                         BuildErrorJson("invalid_request", locale,
                                        {"invalid profile",
                                         "perfil invalido"}));
                return;
            }
        }
        SendJson(request, 200, BuildConnectJson(locale, profile));
    });

    server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
        const TextLocale locale = ResolveRequestLocale(request);
        SendJson(request, 200, BuildRebootJson(locale));
        delay(200);
        ESP.restart();
    });

    server.on("/api/technical-mode", HTTP_POST,
              [](AsyncWebServerRequest *request) {
                  const TextLocale locale = ResolveRequestLocale(request);
                  if (!request->hasParam("enabled")) {
                      SendJson(request, 400,
                               BuildErrorJson("invalid_request", locale,
                                              {"missing enabled",
                                               "faltando enabled"}));
                      return;
                  }

                  bool enabled = false;
                  if (!ParseBoolValue(request->getParam("enabled")->value(),
                                      enabled)) {
                      SendJson(request, 400,
                               BuildErrorJson("invalid_request", locale,
                                              {"invalid enabled",
                                               "enabled invalido"}));
                      return;
                  }

                  SendJson(request, 200, BuildTechnicalModeJson(enabled));
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
        const TextLocale locale = ResolveRequestLocale(request);
        int httpCode = 200;
        String payload;
        if (!IsOperationAllowed(status, EndpointAccess::Technical, locale,
                                httpCode, payload)) {
            SendJson(request, httpCode, payload);
            return;
        }

        if (!IsOperationAllowed(status, EndpointAccess::ReadyOnly, locale,
                                httpCode, payload)) {
            SendJson(request, httpCode, payload);
            return;
        }

        if (!request->hasParam("hi") || !request->hasParam("lo") ||
            !request->hasParam("len")) {
            SendJson(request, 400, BuildErrorJson("invalid_request", locale,
                                                  {"missing hi/lo/len",
                                                   "faltando hi/lo/len"}));
            return;
        }

        uint8_t hi = 0;
        uint8_t lo = 0;
        uint8_t len = 0;
        if (!ParseByteValue(request->getParam("hi")->value(), hi) ||
            !ParseByteValue(request->getParam("lo")->value(), lo) ||
            !ParseByteValue(request->getParam("len")->value(), len)) {
            SendJson(request, 400,
                     BuildErrorJson("invalid_request", locale,
                                    {"invalid hi/lo/len",
                                     "hi/lo/len invalidos"}));
            return;
        }

        SendJson(request, 200, BuildMemoryReadJson(locale, hi, lo, len));
    });

    server.on("/api/sensors/collection", HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  const ECUStatusSnapshot status = scanner.getStatusSnapshot();
                  const TextLocale locale = ResolveRequestLocale(request);
                  int httpCode = 200;
                  String payload;
                  if (!IsOperationAllowed(status, EndpointAccess::ReadyOnly,
                                          locale, httpCode, payload)) {
                      SendJson(request, httpCode, payload);
                      return;
                  }

                  SendJson(request, 200, BuildCollectionJson(locale));
              });

    server.onNotFound([](AsyncWebServerRequest *request) {
        const String path = request->url();
        const auto method = request->method();

        if (IsApiPath(path)) {
            request->send(404, "text/plain", "Not found");
            return;
        }

        if (IsCaptiveApActive() &&
            (method == HTTP_GET || method == HTTP_HEAD)) {
            RedirectToCaptivePortal(request);
            return;
        }

        request->send(404, "text/plain", "Not found");
    });
}
} // namespace

void setup() {
    const ESP32ScannerConfig config{};
    scanner.setConfig(config);
    ApplyKnownProfile(GetDefaultProfile());

    pinMode(config.tx_init_pin, OUTPUT);
    pinMode(config.aux_init_pin, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

    serialCommandBuffer.reserve(128);

    spiffsMounted = SPIFFS.begin();
    if (!spiffsMounted) {
        Serial.println("An error occurred while mounting SPIFFS");
    } else if (!IsWebUiAvailable()) {
        Serial.println("[SPIFFS] Mounted, but /index.html or /app.js is missing");
    }

    WiFi.begin(kWifiStaSsid, kWifiStaPassword);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("Setting AP (Access Point)...");
    const bool apStarted = WiFi.softAP(kWifiApSsid, kWifiApPassword);
    if (!apStarted) {
        Serial.println("[AP] Failed to start softAP");
    }

    const IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);

    if (apStarted) {
        StartCaptiveDns();
    }

    ConfigureRoutes();
    server.begin();

    Serial.println("HTTP server started");
    PrintSerialHelp();
}

void loop() {
    if (captiveDnsRunning) {
        captiveDnsServer.processNextRequest();
    }

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
