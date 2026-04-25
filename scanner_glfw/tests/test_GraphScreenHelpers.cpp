#include "../app_data.h"
#include "../core/StateManager.h"
#include "../screens/GraphScreen.h"
#include "../utils/Colors.h"
#include <gtest/gtest.h>

class GraphScreenTestAccess : public GraphScreen {
  public:
    void LoadState() { LoadStateFromManager(); }
    void SyncPinned() { SyncPinnedSensorsToGraph(); }
    void InitializeMetadata() { InitializeGraphMetadata(); }
    void CacheColors() { CacheColorConversions(); }

    const std::vector<bool> &auto_scale_y() const { return auto_scale_y_; }
    const std::vector<float> &last_plot_min_y() const {
        return last_plot_min_y_;
    }
    const std::vector<float> &last_plot_max_y() const {
        return last_plot_max_y_;
    }
    const std::array<ImU32, 6> &cached_graph_colors() const {
        return cached_graph_colors_;
    }
};

class GraphScreenHelpersTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto &state = StateManager::Instance();
        state.Initialize(":memory:");
        graphSensorIndices.clear();
        graphFrozen = false;
    }

    void TearDown() override { StateManager::Instance().Shutdown(); }
};

TEST_F(GraphScreenHelpersTest, LoadStateFromManager_LoadsGraphSensors) {
    auto &state = StateManager::Instance();
    state.SaveGraphSensors({2, 4, 6});
    graphSensorIndices = {9};

    GraphScreenTestAccess screen;
    screen.LoadState();

    EXPECT_EQ(graphSensorIndices, (std::vector<int>{2, 4, 6}));
}

TEST_F(GraphScreenHelpersTest, SyncPinnedSensorsToGraph_AddsAndSaves) {
    auto &state = StateManager::Instance();
    graphSensorIndices = {0};
    state.SavePinnedSensors({1, 2, 3, 4});

    GraphScreenTestAccess screen;
    screen.SyncPinned();

    const std::vector<int> expected{0, 1, 2, 3};
    EXPECT_EQ(graphSensorIndices, expected);
    EXPECT_EQ(state.LoadGraphSensors(), expected);
}

TEST_F(GraphScreenHelpersTest, InitializeGraphMetadata_ResizesVectors) {
    graphSensorIndices = {5, 6};

    GraphScreenTestAccess screen;
    screen.InitializeMetadata();

    const auto &auto_scale = screen.auto_scale_y();
    const auto &min_vals = screen.last_plot_min_y();
    const auto &max_vals = screen.last_plot_max_y();

    EXPECT_EQ(auto_scale.size(), graphSensorIndices.size());
    EXPECT_EQ(min_vals.size(), graphSensorIndices.size());
    EXPECT_EQ(max_vals.size(), graphSensorIndices.size());

    for (bool enabled : auto_scale) {
        EXPECT_TRUE(enabled);
    }
    for (float value : min_vals) {
        EXPECT_FLOAT_EQ(value, 0.0f);
    }
    for (float value : max_vals) {
        EXPECT_FLOAT_EQ(value, 100.0f);
    }
}

TEST_F(GraphScreenHelpersTest, CacheColorConversions_CachesGraphColors) {
    GraphScreenTestAccess screen;
    screen.CacheColors();

    const auto &cached = screen.cached_graph_colors();
    for (size_t i = 0; i < cached.size(); ++i) {
        if (i >= static_cast<size_t>(Colors::GraphColorsCount)) {
            break;
        }
        EXPECT_EQ(cached[i],
                  ImGui::ColorConvertFloat4ToU32(Colors::GraphColors[i]));
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
