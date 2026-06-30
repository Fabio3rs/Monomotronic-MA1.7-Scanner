#include "../src/ECUSerial/ECULinkConfig.h"
#include <gtest/gtest.h>

TEST(ECULinkConfigTest, ParseSupportedBaudRatesFromString) {
    ASSERT_EQ(ParseECUBaudRate("4800"), ECUBaudRate::BR4800);
    ASSERT_EQ(ParseECUBaudRate("9600"), ECUBaudRate::BR9600);
}

TEST(ECULinkConfigTest, RejectUnsupportedBaudRates) {
    EXPECT_FALSE(ParseECUBaudRate("12345").has_value());
    EXPECT_FALSE(ParseECUBaudRate(12345).has_value());
}

TEST(ECULinkConfigTest, ParseKnownProfileAliases) {
    ASSERT_EQ(ParseKnownProfile("tipo-1.6ie"),
              ECUKnownProfile::FiatTipo16Ie);
    ASSERT_EQ(ParseKnownProfile("fiat_tipo_16ie"),
              ECUKnownProfile::FiatTipo16Ie);
    ASSERT_EQ(ParseKnownProfile("clio-1.6-1999"),
              ECUKnownProfile::RenaultClio16_1999);
    ASSERT_EQ(ParseKnownProfile("renault clio 16 1999"),
              ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECULinkConfigTest, BuildKnownProfilesWithExpectedDefaults) {
    const ECULinkConfig tipo =
        MakeKnownProfileConfig(ECUKnownProfile::FiatTipo16Ie, "/dev/test0",
                               false);
    EXPECT_EQ(tipo.port, "/dev/test0");
    EXPECT_EQ(tipo.session_baud, ECUBaudRate::BR4800);
    EXPECT_EQ(tipo.init_mode, ECUInitMode::FiveBaudBreak);
    EXPECT_EQ(tipo.init_address, 0x10);
    EXPECT_FALSE(tipo.enable_logging);
    ASSERT_TRUE(tipo.profile.has_value());
    EXPECT_EQ(tipo.profile.value(), ECUKnownProfile::FiatTipo16Ie);

    const ECULinkConfig clio =
        MakeKnownProfileConfig(ECUKnownProfile::RenaultClio16_1999);
    EXPECT_EQ(clio.session_baud, ECUBaudRate::BR9600);
    ASSERT_TRUE(clio.profile.has_value());
    EXPECT_EQ(clio.profile.value(), ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECULinkConfigTest, StringAndNumericConversionsAreStable) {
    EXPECT_STREQ(ToString(ECUBaudRate::BR4800), "4800");
    EXPECT_STREQ(ToString(ECUKnownProfile::FiatTipo16Ie), "fiat-tipo-1.6ie");
    EXPECT_EQ(ToUint(ECUBaudRate::BR9600), 9600u);
}
