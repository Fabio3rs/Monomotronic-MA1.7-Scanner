#pragma once

#include "ECULinkConfig.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

enum class ECUSerialCLIParseStatus : uint8_t {
    Ok,
    HelpRequested,
    Error
};

struct ECUSerialCLIParseResult {
    ECUSerialCLIParseStatus status{ECUSerialCLIParseStatus::Error};
    ECULinkConfig config{};
    std::string message;
};

std::string GetECUSerialUsage();

ECUSerialCLIParseResult
ParseECUSerialCLIArgs(std::span<const std::string_view> args) noexcept;

std::vector<std::string_view> MakeCLIArgViews(int argc, char **argv);
