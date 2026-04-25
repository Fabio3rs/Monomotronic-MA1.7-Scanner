#include "../app_data.h"
#include "../core/RecordingManager.h"
#include "../utils/FileIO.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <string_view>

// Globals are now inline in app_data.h; no need to redefine here.
// Provide only the out-of-line symbols that RecordingManager needs but that
// live in app_data.cpp (not in the header).
std::optional<ECUInfo> g_ecu_info;
std::function<void(const ECUInfo &)> on_ecu_info_received;
std::function<void(const std::string &)> on_connection_error;

// Define SensorState constructor (normally in app_data.cpp) for linkage
SensorState::SensorState(std::string_view n, int i, int s,
                         std::function<double(int)> d, std::string_view u,
                         std::string_view desc, double dmin, double dmax,
                         double amin, double amax, int len, SensorPollMode mode)
    : name(n), id(i), subcmd(s), dataLength(len), lastRaw(0.0), lastValue(0.0),
      maxHistory(300), decoder(d), unit(u), description(desc), displayMin(dmin),
      displayMax(dmax), alertMin(amin), alertMax(amax),
      status(SensorStatus::OK), lastUpdateTime(0.0), pollMode(mode),
      pendingStatus(SensorStatus::OK), statusTransitionStartTime(0.0),
      alertTriggerDelay(4.0), alertClearDelay(4.0), inStatusTransition(false) {
    history.reserve(maxHistory);
}

namespace {
SensorState MakeSensor(const char *name, int id, int subcmd = 0) {
    SensorState s;
    s.name = name;
    s.id = id;
    s.subcmd = subcmd;
    s.unit = "u";
    s.lastValue = 1.0;
    s.history.push_back(1.0f);
    return s;
}

std::vector<SensorState> MakeSensors(int count) {
    std::vector<SensorState> sensors;
    sensors.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        sensors.push_back(MakeSensor("S", i));
    }
    return sensors;
}

std::string TempRecordingPath(const std::string &full_path) {
    const std::string marker = "/recordings/";
    const auto pos = full_path.find(marker);
    if (pos == std::string::npos) {
        return full_path;
    }
    return full_path.substr(pos);
}
} // namespace

class RecordingManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Direct recordings to /tmp for tests
        setenv("HOME", "/tmp", 1);
        system("mkdir -p /tmp/.config");
        RecordingManager::Instance().StopRecording();
    }

    void TearDown() override {
        const std::string path = RecordingManager::Instance().GetCurrentPath();
        if (!path.empty()) {
            std::remove(path.c_str());
        }
    }
};

TEST_F(RecordingManagerTest, StartStopSucceeds) {
    auto sensors = MakeSensors(2);

    ASSERT_TRUE(RecordingManager::Instance().StartRecording(sensors, {0, 1}))
        << RecordingManager::Instance().GetLastError();
    RecordingManager::Instance().Update(sensors, 1, 0.1, 0.1);
    const std::string path = RecordingManager::Instance().GetCurrentPath();
    ASSERT_FALSE(path.empty());
    RecordingManager::Instance().StopRecording();

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());
    std::string header;
    std::getline(file, header);
    // header line starts with '# recording_start_local'
    ASSERT_TRUE(header.rfind("# recording_start_local", 0) == 0);
}

TEST_F(RecordingManagerTest, SampleIntervalRespectsThrottle) {
    auto sensors = MakeSensors(1);
    RecordingManager::Instance().SetSampleInterval(
        std::chrono::milliseconds(100));

    ASSERT_TRUE(RecordingManager::Instance().StartRecording(sensors, {0}))
        << RecordingManager::Instance().GetLastError();

    // First sample should write
    RecordingManager::Instance().Update(sensors, 1, 0.1, 0.1);
    const uint64_t after_first = RecordingManager::Instance().GetSampleCount();
    EXPECT_EQ(after_first, 1u);

    // Immediate second should be throttled
    RecordingManager::Instance().Update(sensors, 2, 0.15, 0.05);
    EXPECT_EQ(RecordingManager::Instance().GetSampleCount(), 1u);

    // Allow time to pass for next sample
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    RecordingManager::Instance().Update(sensors, 3, 0.25, 0.10);
    EXPECT_EQ(RecordingManager::Instance().GetSampleCount(), 2u);

    RecordingManager::Instance().StopRecording();
}

TEST_F(RecordingManagerTest, InvalidIndicesFailStart) {
    auto sensors = MakeSensors(2);
    const bool started =
        RecordingManager::Instance().StartRecording(sensors, {5, 6});
    EXPECT_FALSE(started);
    EXPECT_FALSE(RecordingManager::Instance().IsRecording());
    EXPECT_FALSE(RecordingManager::Instance().GetLastError().empty());
}

TEST_F(RecordingManagerTest, DuplicateSequenceIgnored) {
    auto sensors = MakeSensors(1);
    ASSERT_TRUE(RecordingManager::Instance().StartRecording(sensors, {0}))
        << RecordingManager::Instance().GetLastError();

    RecordingManager::Instance().Update(sensors, 1, 0.1, 0.1);
    RecordingManager::Instance().Update(sensors, 1, 0.2, 0.1); // duplicate seq
    EXPECT_EQ(RecordingManager::Instance().GetSampleCount(), 1u);

    RecordingManager::Instance().StopRecording();
}
