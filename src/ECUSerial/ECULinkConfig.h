#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class ECUInitMode : uint8_t {
    FiveBaudBreak
};

enum class ECUBaudRate : uint32_t {
    BR4800 = 4800,
    BR9600 = 9600
};

enum class ECUKnownProfile : uint8_t {
    FiatTipo16Ie,
    RenaultClio16_1999
};

struct ECULinkConfig {
    std::string port{"/dev/ttyUSB0"};
    ECUBaudRate session_baud{ECUBaudRate::BR4800};
    ECUInitMode init_mode{ECUInitMode::FiveBaudBreak};
    uint8_t init_address{0x10};
    bool enable_logging{true};
    std::optional<ECUKnownProfile> profile{};
};

std::optional<ECUBaudRate> ParseECUBaudRate(std::string_view text) noexcept;
std::optional<ECUBaudRate> ParseECUBaudRate(uint32_t value) noexcept;

std::optional<ECUKnownProfile> ParseKnownProfile(std::string_view text) noexcept;

const char *ToString(ECUBaudRate baud) noexcept;
const char *ToString(ECUKnownProfile profile) noexcept;

uint32_t ToUint(ECUBaudRate baud) noexcept;

ECULinkConfig MakeKnownProfileConfig(
    ECUKnownProfile profile, std::string_view port = "/dev/ttyUSB0",
    bool enable_logging = true) noexcept;
