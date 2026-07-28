#include "../ESP32/optional.h"
#include <gtest/gtest.h>
#include <optional>
#include <type_traits>

TEST(ESP32OptionalStdTest, UsesStdOptionalWhenAvailable) {
    EXPECT_TRUE((std::is_same<optional<int>, std::optional<int>>::value));
}

TEST(ESP32OptionalStdTest, SupportsStdOptionalOperationsThroughAdapter) {
    optional<int> value;
    EXPECT_FALSE(value.has_value());

    value = 12;
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), 12);
    EXPECT_EQ(*value, 12);

    value = nullopt;
    EXPECT_FALSE(value.has_value());
}
