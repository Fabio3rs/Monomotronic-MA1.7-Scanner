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
    EXPECT_EQ(result.config.session_baud, 4800u);
    EXPECT_EQ(result.baud_source, ECUSerialBaudSource::DefaultProfile);
    ASSERT_TRUE(result.config.profile.has_value());
    EXPECT_EQ(result.config.profile.value(), ECUKnownProfile::FiatTipo16Ie);
}

TEST(ECUSerialCLITest, SupportsPositionalCompatibility) {
    const auto result = ParseArgs({"/dev/ttyS1", "9600"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyS1");
    EXPECT_EQ(result.config.session_baud, 9600u);
    EXPECT_EQ(result.baud_source, ECUSerialBaudSource::ExplicitOverride);
}

TEST(ECUSerialCLITest, AcceptsArbitraryBaudValues) {
    const auto result = ParseArgs({"--port", "/dev/ttyS1", "--baud", "10400"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyS1");
    EXPECT_EQ(result.config.session_baud, 10400u);
    EXPECT_EQ(result.baud_source, ECUSerialBaudSource::ExplicitOverride);
}

TEST(ECUSerialCLITest, AppliesProfileAndExplicitOverrides) {
    const auto result =
        ParseArgs({"--profile", "clio-1.6-1999", "--port", "/dev/ttyACM0"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(result.config.port, "/dev/ttyACM0");
    EXPECT_EQ(result.config.session_baud, 9600u);
    EXPECT_EQ(result.baud_source, ECUSerialBaudSource::DefaultProfile);
    ASSERT_TRUE(result.config.profile.has_value());
    EXPECT_EQ(result.config.profile.value(),
              ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECUSerialCLITest, ExplicitBaudWinsBeforeOrAfterProfile) {
    const auto before =
        ParseArgs({"--baud", "9600", "--profile", "tipo-1.6ie"});
    ASSERT_EQ(before.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(before.config.session_baud, 9600u);
    EXPECT_EQ(before.baud_source, ECUSerialBaudSource::ExplicitOverride);

    const auto after = ParseArgs({"--profile", "tipo-1.6ie", "--baud", "9600"});
    ASSERT_EQ(after.status, ECUSerialCLIParseStatus::Ok);
    EXPECT_EQ(after.config.session_baud, 9600u);
    EXPECT_EQ(after.baud_source, ECUSerialBaudSource::ExplicitOverride);
}

TEST(ECUSerialCLITest, ReportsUnknownProfile) {
    const auto result = ParseArgs({"--profile", "unknown"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "Unknown profile: unknown. Available profiles: "
                              "fiat-tipo-1.6ie, renault-clio-1.6-1999");
}

TEST(ECUSerialCLITest, ReportsInvalidBaud) {
    const auto result = ParseArgs({"--baud", "0"});
    ASSERT_EQ(result.status, ECUSerialCLIParseStatus::Error);
    EXPECT_EQ(result.message, "Unsupported baud rate: 0");
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
    EXPECT_NE(result.message.find("Profiles:"), std::string::npos);
    EXPECT_NE(result.message.find("fiat-tipo-1.6ie"), std::string::npos);
    EXPECT_NE(result.message.find("renault-clio-1.6-1999"), std::string::npos);
    EXPECT_NE(result.message.find("--baud overrides the profile baud"),
              std::string::npos);
}
