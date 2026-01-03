#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace UI::Modals {

class SensorListEditorModal {
  public:
    using OnConfirmCallback = std::function<void(const std::vector<int> &)>;

    SensorListEditorModal() = default;

    void Open(const std::vector<int> &selected_indices,
              std::size_t sensor_count);
    void Open();
    void Close();
    void Render();

    std::vector<int> GetCurrentSelection() const;
    void SetOnConfirm(OnConfirmCallback callback);
    bool IsOpen() const { return is_open_; }

  private:
    void InitializeSelection(const std::vector<int> &selected_indices,
                             std::size_t sensor_count);
    void RenderSensorTree();
    bool RenderButtons();
    bool IsSensorSelected(std::size_t index) const;
    void ToggleSensorSelection(std::size_t index);

    bool is_open_ = false;
    std::vector<bool> selection_;
    OnConfirmCallback on_confirm_;
};

} // namespace UI::Modals
