#include "../ui/components/TopBarComponents.h"
#include <gtest/gtest.h>
#include <vector>

TEST(TopBarComponentsTest, ECUStatusIndicator_StatusColor) {
    EXPECT_EQ(UI::Components::ECUStatusIndicator::GetStatusColor(true),
              IM_COL32(0, 255, 0, 255));
    EXPECT_EQ(UI::Components::ECUStatusIndicator::GetStatusColor(false),
              IM_COL32(128, 128, 128, 255));
}

TEST(TopBarComponentsTest, SignalStrengthBars_CalculatesStrength) {
    EXPECT_FLOAT_EQ(
        UI::Components::SignalStrengthBars::CalculateSignalStrength(0.0f, 0.0f),
        1.0f);
    EXPECT_FLOAT_EQ(UI::Components::SignalStrengthBars::CalculateSignalStrength(
                        200.0f, 1.0f),
                    0.0f);
}

TEST(TopBarComponentsTest, SignalStrengthBars_LatencyColorThresholds) {
    EXPECT_EQ(UI::Components::SignalStrengthBars::GetLatencyColor(30.0f),
              IM_COL32(0, 255, 0, 255));
    EXPECT_EQ(UI::Components::SignalStrengthBars::GetLatencyColor(80.0f),
              IM_COL32(255, 165, 0, 255));
    EXPECT_EQ(UI::Components::SignalStrengthBars::GetLatencyColor(200.0f),
              IM_COL32(255, 0, 0, 255));
}

TEST(TopBarComponentsTest, TableStatusDisplay_TableLabels) {
    const UI::Components::TableStatus idle =
        UI::Components::TableStatusDisplay::GetStatus(0);
    EXPECT_STREQ(idle.label, "TABLE: -");
    EXPECT_EQ(idle.color, IM_COL32(160, 160, 160, 255));

    const UI::Components::TableStatus table1 =
        UI::Components::TableStatusDisplay::GetStatus(1);
    EXPECT_STREQ(table1.label, "TABLE: T1");
    EXPECT_EQ(table1.color, IM_COL32(0, 200, 255, 255));

    const UI::Components::TableStatus table2 =
        UI::Components::TableStatusDisplay::GetStatus(2);
    EXPECT_STREQ(table2.label, "TABLE: T2");
    EXPECT_EQ(table2.color, IM_COL32(0, 255, 170, 255));
}

TEST(TopBarComponentsTest, SensorFreshnessDisplay_AgeColors) {
    EXPECT_EQ(UI::Components::SensorFreshnessDisplay::GetAgeColor(0.1),
              IM_COL32(0, 255, 0, 255));
    EXPECT_EQ(UI::Components::SensorFreshnessDisplay::GetAgeColor(0.5),
              IM_COL32(255, 165, 0, 255));
    EXPECT_EQ(UI::Components::SensorFreshnessDisplay::GetAgeColor(2.0),
              IM_COL32(255, 0, 0, 255));
}

TEST(TopBarComponentsTest, SensorFreshnessDisplay_EmptySensorsUsesFallback) {
    const std::vector<SensorState> sensors;
    EXPECT_DOUBLE_EQ(
        UI::Components::SensorFreshnessDisplay::CalculateAgeSeconds(sensors,
                                                                    5.0),
        999.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
