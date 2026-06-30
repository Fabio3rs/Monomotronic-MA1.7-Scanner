#include "ECULinkConfig.h"

#include <algorithm>
#include <cctype>

namespace {
std::string NormalizeProfile(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());

    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else if (ch == '-' || ch == '_' || ch == ' ') {
            normalized.push_back('-');
        }
    }

    return normalized;
}
} // namespace

std::optional<ECUBaudRate> ParseECUBaudRate(std::string_view text) noexcept {
    if (text == "4800") {
        return ECUBaudRate::BR4800;
    }
    if (text == "9600") {
        return ECUBaudRate::BR9600;
    }
    return std::nullopt;
}

std::optional<ECUBaudRate> ParseECUBaudRate(uint32_t value) noexcept {
    switch (value) {
    case 4800:
        return ECUBaudRate::BR4800;
    case 9600:
        return ECUBaudRate::BR9600;
    default:
        return std::nullopt;
    }
}

std::optional<ECUKnownProfile> ParseKnownProfile(std::string_view text) noexcept {
    const std::string normalized = NormalizeProfile(text);

    if (normalized == "tipo-1-6ie" || normalized == "tipo-16ie" ||
        normalized == "fiat-tipo-1-6ie" ||
        normalized == "fiat-tipo-16ie") {
        return ECUKnownProfile::FiatTipo16Ie;
    }

    if (normalized == "clio-1-6-1999" || normalized == "clio-16-1999" ||
        normalized == "renault-clio-1-6-1999" ||
        normalized == "renault-clio-16-1999") {
        return ECUKnownProfile::RenaultClio16_1999;
    }

    return std::nullopt;
}

const char *ToString(ECUBaudRate baud) noexcept {
    switch (baud) {
    case ECUBaudRate::BR4800:
        return "4800";
    case ECUBaudRate::BR9600:
        return "9600";
    default:
        return "unknown";
    }
}

const char *ToString(ECUKnownProfile profile) noexcept {
    switch (profile) {
    case ECUKnownProfile::FiatTipo16Ie:
        return "fiat-tipo-1.6ie";
    case ECUKnownProfile::RenaultClio16_1999:
        return "renault-clio-1.6-1999";
    default:
        return "unknown";
    }
}

uint32_t ToUint(ECUBaudRate baud) noexcept {
    return static_cast<uint32_t>(baud);
}

ECULinkConfig MakeKnownProfileConfig(ECUKnownProfile profile,
                                     std::string_view port,
                                     bool enable_logging) noexcept {
    ECULinkConfig config;
    config.port = std::string(port);
    config.enable_logging = enable_logging;
    config.profile = profile;
    config.init_mode = ECUInitMode::FiveBaudBreak;
    config.init_address = 0x10;

    switch (profile) {
    case ECUKnownProfile::FiatTipo16Ie:
        config.session_baud = ECUBaudRate::BR4800;
        break;
    case ECUKnownProfile::RenaultClio16_1999:
        config.session_baud = ECUBaudRate::BR9600;
        break;
    }

    return config;
}
