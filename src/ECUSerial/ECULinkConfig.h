#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class ECUInitMode : uint8_t { FiveBaudBreak };

enum class ECUKnownProfile : uint8_t { FiatTipo16Ie, RenaultClio16_1999 };

struct ECULinkConfig {
    std::string port{"/dev/ttyUSB0"};
    uint32_t session_baud{4800};
    ECUInitMode init_mode{ECUInitMode::FiveBaudBreak};
    uint8_t init_address{0x10};
    bool enable_logging{true};
    std::optional<ECUKnownProfile> profile{};
};

std::optional<uint32_t> ParseECUBaudRate(std::string_view text) noexcept;
std::optional<uint32_t> ParseECUBaudRate(uint32_t value) noexcept;

std::optional<ECUKnownProfile>
ParseKnownProfile(std::string_view text) noexcept;

std::string ToString(uint32_t baud);
const char *ToString(ECUKnownProfile profile) noexcept;

ECULinkConfig MakeKnownProfileConfig(ECUKnownProfile profile,
                                     std::string_view port = "/dev/ttyUSB0",
                                     bool enable_logging = true) noexcept;
