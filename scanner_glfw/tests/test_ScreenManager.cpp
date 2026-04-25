#include "../core/ScreenManager.h"
#include "../screens/BaseScreen.h"
#include <gtest/gtest.h>

class ScreenManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ScreenManager::Instance().Shutdown();
        SetCurrentScreen(Screen::DASH);
        ScreenManager::Instance().Initialize();
    }

    void TearDown() override { ScreenManager::Instance().Shutdown(); }
};

TEST_F(ScreenManagerTest, Initialize_SetsInitialScreen) {
    auto *dash = ScreenManager::Instance().GetScreen(Screen::DASH);
    ASSERT_NE(dash, nullptr);
    EXPECT_TRUE(dash->IsActive());
}

TEST_F(ScreenManagerTest, SetCurrentScreen_TransitionsAndUpdatesState) {
    auto *dash = ScreenManager::Instance().GetScreen(Screen::DASH);
    auto *live = ScreenManager::Instance().GetScreen(Screen::LIVE);
    ASSERT_NE(dash, nullptr);
    ASSERT_NE(live, nullptr);

    ScreenManager::Instance().SetCurrentScreen(Screen::LIVE);

    EXPECT_EQ(ScreenManager::Instance().GetCurrentScreen(), Screen::LIVE);
    EXPECT_TRUE(live->IsActive());
    EXPECT_FALSE(dash->IsActive());
}

TEST_F(ScreenManagerTest, Update_TransitionsFromGlobalScreenState) {
    auto *dash = ScreenManager::Instance().GetScreen(Screen::DASH);
    auto *graph = ScreenManager::Instance().GetScreen(Screen::GRAPH);
    ASSERT_NE(dash, nullptr);
    ASSERT_NE(graph, nullptr);

    SetCurrentScreen(Screen::GRAPH);
    ScreenManager::Instance().Update(0.0f);

    EXPECT_EQ(ScreenManager::Instance().GetCurrentScreen(), Screen::GRAPH);
    EXPECT_TRUE(graph->IsActive());
    EXPECT_FALSE(dash->IsActive());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
