#include "../src/ECUSerial/ECUSerialCLI.h"
#include <array>
#include <gtest/gtest.h>

namespace {
ECUSerialCLIParseResult
ParseArgs(std::initializer_list<std::string_view> args) {
    std::vector<std::string_view> values(args);
    return ParseECUSerialCLIArgs(values);
}
} // namespace

TEST(ECUSerialCLITest, DefaultsMatchMA17Profile) {
    const auto result = ParseArgs({});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyUSB0");
    EXPECT_EQ(result.config.session_baud, ECUBaudRate::BR4800);
    ASSERT_TRUE(result.config.profile.has_value());
    EXPECT_EQ(result.config.profile.value(), ECUKnownProfile::FiatTipo16Ie);
}

TEST(ECUSerialCLITest, SupportsPositionalCompatibility) {
    const auto result = ParseArgs({"/dev/ttyS1", "9600"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyS1");
    EXPECT_EQ(result.config.session_baud, ECUBaudRate::BR9600);
}

TEST(ECUSerialCLITest, AppliesProfileAndExplicitOverrides) {
    const auto result = ParseArgs(
        {"--profile", "clio-1.6-1999", "--port", "/dev/ttyACM0"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyACM0");
    EXPECT_EQ(result.config.session_baud, ECUBaudRate::BR9600);
    ASSERT_TRUE(result.config.profile.has_value());
    EXPECT_EQ(result.config.profile.value(),
              ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECUSerialCLITest, ExplicitBaudWinsBeforeOrAfterProfile) {
    const auto before =
        ParseArgs({"--baud", "9600", "--profile", "tipo-1.6ie"});
    ASSERT_EQ(before.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(before.config.session_baud, ECUBaudRate::BR9600);

    const auto after =
        ParseArgs({"--profile", "tipo-1.6ie", "--baud", "9600"});
    ASSERT_EQ(after.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(after.config.session_baud, ECUBaudRate::BR9600);
}

TEST(ECUSerialCLITest, ReportsUnknownProfile) {
    const auto result = ParseArgs({"--profile", "unknown"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "Unknown profile: unknown");
}

TEST(ECUSerialCLITest, ReportsUnsupportedBaud) {
    const auto result = ParseArgs({"--baud", "12345"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message,
              "Unsupported baud rate: 12345 (supported: 4800, 9600)");
}

TEST(ECUSerialCLITest, ReportsFlagsMissingValues) {
    auto result = ParseArgs({"--port"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "--port requires a value");

    result = ParseArgs({"--baud"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "--baud requires a value");

    result = ParseArgs({"--profile"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "--profile requires a value");
}

TEST(ECUSerialCLITest, HelpDoesNotProduceError) {
    const auto result = ParseArgs({"--help"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::HelpRequested);
    EXPECT_EQ(result.message, GetECUSerialUsage());
}
