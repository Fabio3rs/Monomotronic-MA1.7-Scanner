#include "ECUSerialCLI.h"

#include <optional>

std::string GetECUSerialUsage() {
    return "Usage:\n"
           "  ecuserial [port]\n"
           "  ecuserial [port] [baud]\n"
           "  ecuserial --port <path> [--profile <name>] [--baud <rate>]\n"
           "\n"
           "Profiles:\n"
           "  fiat-tipo-1.6ie      default baud 4800\n"
           "  renault-clio-1.6-1999 default baud 9600\n"
           "\n"
           "Behavior:\n"
           "  --profile selects protocol defaults for the target ECU.\n"
           "  --baud overrides the profile baud when both are provided.\n"
           "  Positional mode remains compatible: [port] [baud].\n";
}

ECUSerialCLIParseResult
ParseECUSerialCLIArgs(std::span<const std::string_view> args) noexcept {
    ECUSerialCLIParseResult result;
    result.status = ECUSerialCLIParseStatus::Ok;
    result.config = MakeKnownProfileConfig(ECUKnownProfile::FiatTipo16Ie);

    bool positional_port_consumed = false;
    std::optional<uint32_t> explicit_baud;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string_view arg = args[i];

        if (arg == "--help" || arg == "-h") {
            result.status = ECUSerialCLIParseStatus::HelpRequested;
            result.message = GetECUSerialUsage();
            return result;
        }

        if (arg == "--port") {
            if (i + 1 >= args.size()) {
                result.status = ECUSerialCLIParseStatus::Error;
                result.message = "--port requires a value";
                return result;
            }
            result.config.port = std::string(args[++i]);
            positional_port_consumed = true;
            continue;
        }

        if (arg == "--baud") {
            if (i + 1 >= args.size()) {
                result.status = ECUSerialCLIParseStatus::Error;
                result.message = "--baud requires a value";
                return result;
            }

            const auto baud = ParseECUBaudRate(args[++i]);
            if (!baud.has_value()) {
                result.status = ECUSerialCLIParseStatus::Error;
                result.message =
                    "Unsupported baud rate: " + std::string(args[i]);
                return result;
            }

            explicit_baud = baud.value();
            result.config.session_baud = explicit_baud.value();
            result.baud_source = ECUSerialBaudSource::ExplicitOverride;
            continue;
        }

        if (arg == "--profile") {
            if (i + 1 >= args.size()) {
                result.status = ECUSerialCLIParseStatus::Error;
                result.message = "--profile requires a value";
                return result;
            }

            const auto profile = ParseKnownProfile(args[++i]);
            if (!profile.has_value()) {
                result.status = ECUSerialCLIParseStatus::Error;
                result.message = "Unknown profile: " + std::string(args[i]) +
                                 ". Available profiles: fiat-tipo-1.6ie, "
                                 "renault-clio-1.6-1999";
                return result;
            }

            const std::string current_port = result.config.port;
            const bool current_logging = result.config.enable_logging;
            result.config = MakeKnownProfileConfig(
                profile.value(), current_port, current_logging);
            if (explicit_baud.has_value()) {
                result.config.session_baud = explicit_baud.value();
                result.baud_source = ECUSerialBaudSource::ExplicitOverride;
            } else {
                result.baud_source = ECUSerialBaudSource::DefaultProfile;
            }
            continue;
        }

        if (!positional_port_consumed) {
            result.config.port = std::string(arg);
            positional_port_consumed = true;
            continue;
        }

        const auto baud = ParseECUBaudRate(arg);
        if (baud.has_value()) {
            explicit_baud = baud.value();
            result.config.session_baud = explicit_baud.value();
            result.baud_source = ECUSerialBaudSource::ExplicitOverride;
            continue;
        }

        result.status = ECUSerialCLIParseStatus::Error;
        result.message = "Unexpected argument: " + std::string(arg);
        return result;
    }

    return result;
}

std::vector<std::string_view> MakeCLIArgViews(int argc, char **argv) {
    std::vector<std::string_view> args;
    args.reserve(argc > 1 ? static_cast<size_t>(argc - 1) : 0);

    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    return args;
}
