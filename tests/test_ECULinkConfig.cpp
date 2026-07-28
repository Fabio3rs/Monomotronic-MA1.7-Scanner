#include "../src/ECUSerial/ECULinkConfig.h"
#include <gtest/gtest.h>

TEST(ECULinkConfigTest, ParseBaudRatesFromString) {
    ASSERT_EQ(ParseECUBaudRate("4800"), 4800u);
    ASSERT_EQ(ParseECUBaudRate("9600"), 9600u);
    ASSERT_EQ(ParseECUBaudRate("12345"), 12345u);
}

TEST(ECULinkConfigTest, RejectInvalidBaudRates) {
    EXPECT_FALSE(ParseECUBaudRate("0").has_value());
    EXPECT_FALSE(ParseECUBaudRate("abc").has_value());
    EXPECT_FALSE(ParseECUBaudRate(0).has_value());
}

TEST(ECULinkConfigTest, ParseKnownProfileAliases) {
    ASSERT_EQ(ParseKnownProfile("tipo-1.6ie"), ECUKnownProfile::FiatTipo16Ie);
    ASSERT_EQ(ParseKnownProfile("fiat_tipo_16ie"),
              ECUKnownProfile::FiatTipo16Ie);
    ASSERT_EQ(ParseKnownProfile("clio-1.6-1999"),
              ECUKnownProfile::RenaultClio16_1999);
    ASSERT_EQ(ParseKnownProfile("renault clio 16 1999"),
              ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECULinkConfigTest, BuildKnownProfilesWithExpectedDefaults) {
    const ECULinkConfig tipo = MakeKnownProfileConfig(
        ECUKnownProfile::FiatTipo16Ie, "/dev/test0", false);
    EXPECT_EQ(tipo.port, "/dev/test0");
    EXPECT_EQ(tipo.session_baud, 4800u);
    EXPECT_EQ(tipo.init_mode, ECUInitMode::FiveBaudBreak);
    EXPECT_EQ(tipo.init_address, 0x10);
    EXPECT_FALSE(tipo.enable_logging);
    ASSERT_TRUE(tipo.profile.has_value());
    EXPECT_EQ(tipo.profile.value(), ECUKnownProfile::FiatTipo16Ie);

    const ECULinkConfig clio =
        MakeKnownProfileConfig(ECUKnownProfile::RenaultClio16_1999);
    EXPECT_EQ(clio.session_baud, 9600u);
    ASSERT_TRUE(clio.profile.has_value());
    EXPECT_EQ(clio.profile.value(), ECUKnownProfile::RenaultClio16_1999);
}

TEST(ECULinkConfigTest, StringAndNumericConversionsAreStable) {
    EXPECT_EQ(ToString(4800), "4800");
    EXPECT_EQ(ToString(12345), "12345");
    EXPECT_STREQ(ToString(ECUKnownProfile::FiatTipo16Ie), "fiat-tipo-1.6ie");
}
