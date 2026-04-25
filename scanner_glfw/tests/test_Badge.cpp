#include "../ui/components/Badge.h"
#include <gtest/gtest.h>

TEST(BadgeTest, ShouldRenderCount_ReturnsFalseForZero) {
    EXPECT_FALSE(UI::Components::Badge::ShouldRenderCount(0));
}

TEST(BadgeTest, ClampCount_LimitsToMaxDisplay) {
    EXPECT_EQ(UI::Components::Badge::ClampCount(120, 99), 99);
    EXPECT_EQ(UI::Components::Badge::ClampCount(-5, 99), 0);
}

TEST(BadgeTest, CalculateCornerCenter_UsesMarginAndRadius) {
    const ImVec2 btn_min(10.0f, 20.0f);
    const ImVec2 btn_max(110.0f, 70.0f);
    const ImVec2 center = UI::Components::Badge::CalculateCornerCenter(
        btn_min, btn_max, 5.0f, 12.0f);
    EXPECT_FLOAT_EQ(center.x, 93.0f);
    EXPECT_FLOAT_EQ(center.y, 37.0f);
}

TEST(BadgeTest, CalculateRadiusForTextWidth_RespectsMinRadius) {
    UI::Components::BadgeConfig config;
    config.min_radius = 10.0f;
    config.padding = 6.0f;

    EXPECT_FLOAT_EQ(
        UI::Components::Badge::CalculateRadiusForTextWidth(8.0f, config),
        10.0f);
    EXPECT_FLOAT_EQ(
        UI::Components::Badge::CalculateRadiusForTextWidth(30.0f, config),
        21.0f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
