#include "ECULinkConfig.h"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace {
std::string NormalizeProfile(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());

    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch))));
        } else if (ch == '-' || ch == '_' || ch == ' ') {
            normalized.push_back('-');
        }
    }

    return normalized;
}
} // namespace

std::optional<uint32_t> ParseECUBaudRate(std::string_view text) noexcept {
    uint32_t baud = 0;
    const char *begin = text.data();
    const char *end = text.data() + text.size();
    const auto parse_result = std::from_chars(begin, end, baud);
    if (parse_result.ec != std::errc() || parse_result.ptr != end) {
        return std::nullopt;
    }

    return ParseECUBaudRate(baud);
}

std::optional<uint32_t> ParseECUBaudRate(uint32_t value) noexcept {
    if (value == 0) {
        return std::nullopt;
    }

    return value;
}

std::optional<ECUKnownProfile>
ParseKnownProfile(std::string_view text) noexcept {
    const std::string normalized = NormalizeProfile(text);

    if (normalized == "tipo-1-6ie" || normalized == "tipo-16ie" ||
        normalized == "fiat-tipo-1-6ie" || normalized == "fiat-tipo-16ie") {
        return ECUKnownProfile::FiatTipo16Ie;
    }

    if (normalized == "clio-1-6-1999" || normalized == "clio-16-1999" ||
        normalized == "renault-clio-1-6-1999" ||
        normalized == "renault-clio-16-1999") {
        return ECUKnownProfile::RenaultClio16_1999;
    }

    return std::nullopt;
}

std::string ToString(uint32_t baud) { return std::to_string(baud); }

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
        config.session_baud = 4800;
        break;
    case ECUKnownProfile::RenaultClio16_1999:
        config.session_baud = 9600;
        break;
    }

    return config;
}
