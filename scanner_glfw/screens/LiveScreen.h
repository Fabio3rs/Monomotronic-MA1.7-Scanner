#pragma once

#include "../ui/SensorTable.h"
#include "../ui/modals/SensorListEditorModal.h"
#include "BaseScreen.h"
#include <string>
#include <vector>

class LiveScreen : public BaseScreen {
  public:
    LiveScreen();
    ~LiveScreen() override = default;

    // BaseScreen interface
    void OnEnter() override;
    void OnExit() override;
    void Update(float delta_time) override;
    void Render() override;
    bool HandleGesture(const GestureEvent &event) override;

  private:
    // UI state
    bool show_all_sensors_ = false;
    char filter_buffer_[256] = "";
    bool paused_ = false;

    // Snapshot state
    float snapshot_flash_alpha_ = 0.0f;
    bool snap_button_feedback_ = false;
    double snap_feedback_time_ = 0.0;

    // Paused sensor snapshot (values frozen when paused)
    std::vector<SensorState> paused_sensor_snapshot_;

    // Components
    SensorTable sensor_table_;
    UI::Modals::SensorListEditorModal sensor_list_modal_;

    // Helper methods
    void RenderTopControls();
    void RenderSensorTable();
    void RenderAlertBanner();

    std::vector<int> GetVisibleSensorIndices() const;
    void ApplySubscriptions();
    bool HasCriticalSensors() const;

    // Callbacks for SensorTable
    void OnPinToggle(int sensor_idx, bool pinned);
    bool IsSensorPinned(int sensor_idx) const;

    // Recording methods
    bool StartRecording();
    void StopRecording();
    void WriteRecordingSample();

    // Feedback state
    bool show_recording_toast_ = false;
    float recording_toast_timer_ = 0.0f;
    std::string recording_toast_message_;
    bool recording_toast_error_ = false;

    // Snapshot methods
    bool TakeSnapshot();
};
