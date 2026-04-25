#include "../utils/ColorConversion.h"
#include <gtest/gtest.h>

TEST(ColorConversionTest, GetColorChannel_ExtractsCorrectChannel) {
    ImU32 color = IM_COL32(12, 34, 56, 78);
    EXPECT_EQ(UI::Colors::GetColorChannel(color, IM_COL32_R_SHIFT), 12);
    EXPECT_EQ(UI::Colors::GetColorChannel(color, IM_COL32_G_SHIFT), 34);
    EXPECT_EQ(UI::Colors::GetColorChannel(color, IM_COL32_B_SHIFT), 56);
    EXPECT_EQ(UI::Colors::GetColorChannel(color, IM_COL32_A_SHIFT), 78);
}

TEST(ColorConversionTest, NormalizeChannel_MapsToUnitRange) {
    EXPECT_FLOAT_EQ(UI::Colors::NormalizeChannel(0), 0.0f);
    EXPECT_FLOAT_EQ(UI::Colors::NormalizeChannel(255), 1.0f);
    EXPECT_NEAR(UI::Colors::NormalizeChannel(128), 128.0f / 255.0f, 1e-6f);
}

TEST(ColorConversionTest, ImU32ToImVec4_ConvertsCorrectly) {
    ImU32 color = IM_COL32(10, 20, 30, 255);
    ImVec4 vec = UI::Colors::ImU32ToImVec4(color);
    EXPECT_NEAR(vec.x, 10.0f / 255.0f, 1e-6f);
    EXPECT_NEAR(vec.y, 20.0f / 255.0f, 1e-6f);
    EXPECT_NEAR(vec.z, 30.0f / 255.0f, 1e-6f);
    EXPECT_NEAR(vec.w, 1.0f, 1e-6f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
