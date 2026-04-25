#include "SensorListEditorModal.h"
#include "../../app_data.h"
#include "../../utils/Layout.h"
#include "imgui.h"
#include <algorithm>
#include <utility>

namespace {
constexpr const char *kPopupName = "Edit Custom List";
} // namespace

namespace UI::Modals {

void SensorListEditorModal::Open(const std::vector<int> &selected_indices,
                                 std::size_t sensor_count) {
    InitializeSelection(selected_indices, sensor_count);
    Open();
}

void SensorListEditorModal::Open() {
    if (!is_open_) {
        is_open_ = true;
        ImGui::OpenPopup(kPopupName);
    }
}

void SensorListEditorModal::Close() {
    if (is_open_) {
        is_open_ = false;
        ImGui::CloseCurrentPopup();
    }
}

void SensorListEditorModal::Render() {
    if (!is_open_) {
        return;
    }

    const ImVec2 modal_size = Layout::GetModalSize(
        Layout::Modal::MAX_WIDTH_RATIO, Layout::Modal::MAX_HEIGHT_RATIO);
    ImGui::SetNextWindowSize(modal_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2((ImGui::GetIO().DisplaySize.x - modal_size.x) * 0.5f,
               (ImGui::GetIO().DisplaySize.y - modal_size.y) * 0.5f),
        ImGuiCond_Always);

    const bool popup_open = ImGui::BeginPopupModal(kPopupName, &is_open_,
                                                   ImGuiWindowFlags_NoResize |
                                                       ImGuiWindowFlags_NoMove);

    if (popup_open) {
        if (ImGui::IsMouseClicked(0) &&
            !ImGui::IsWindowHovered(
                ImGuiHoveredFlags_ChildWindows |
                ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            Close();
        }

        RenderSensorTree();
        const bool confirmed = RenderButtons();

        ImGui::EndPopup();

        if (confirmed && on_confirm_) {
            on_confirm_(GetCurrentSelection());
        }
    }
}

std::vector<int> SensorListEditorModal::GetCurrentSelection() const {
    std::vector<int> selected;
    selected.reserve(selection_.size());
    for (std::size_t i = 0; i < selection_.size(); ++i) {
        if (selection_[i]) {
            selected.push_back(static_cast<int>(i));
        }
    }
    return selected;
}

void SensorListEditorModal::SetOnConfirm(OnConfirmCallback callback) {
    on_confirm_ = std::move(callback);
}

void SensorListEditorModal::InitializeSelection(
    const std::vector<int> &selected_indices, std::size_t sensor_count) {
    selection_.assign(sensor_count, false);
    for (int idx : selected_indices) {
        if (idx >= 0 && static_cast<std::size_t>(idx) < selection_.size()) {
            selection_[static_cast<std::size_t>(idx)] = true;
        }
    }
}

void SensorListEditorModal::RenderSensorTree() {
    ImGui::Text("Select sensors to display in custom view:");
    ImGui::Spacing();
    ImGui::Separator();

    const ImVec2 modal_size = ImGui::GetWindowSize();
    const float child_width =
        std::min(Layout::Modal::CHILD_MAX_WIDTH,
                 modal_size.x - Layout::Modal::CHILD_HORIZONTAL_PADDING);
    const float child_height =
        std::min(Layout::Modal::CHILD_MAX_HEIGHT,
                 modal_size.y - Layout::Modal::CHILD_VERTICAL_PADDING);

    ImGui::BeginChild("SensorSelection", ImVec2(child_width, child_height),
                      true);

    const std::size_t sensor_count = simulatedSensors.size();
    if (selection_.size() != sensor_count) {
        selection_.resize(sensor_count, false);
    }

    for (const auto &cat_pair : sensorCategories) {
        if (ImGui::CollapsingHeader(cat_pair.first.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int idx : cat_pair.second) {
                if (idx < 0 ||
                    static_cast<std::size_t>(idx) >= simulatedSensors.size()) {
                    continue;
                }

                const std::size_t index = static_cast<std::size_t>(idx);
                bool is_selected = IsSensorSelected(index);
                if (ImGui::Checkbox(simulatedSensors[idx].name.c_str(),
                                    &is_selected)) {
                    ToggleSensorSelection(index);
                }
            }
        }
    }

    ImGui::EndChild();
}

bool SensorListEditorModal::RenderButtons() {
    bool confirmed = false;

    ImGui::Spacing();
    ImGui::Separator();

    const float btn_width = Layout::Button::MODAL_WIDTH;
    const float btn_height = Layout::Button::MODAL_HEIGHT;
    const float spacing = Layout::Padding::MEDIUM;
    const float total_btn_width = btn_width * 2 + spacing;

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total_btn_width) * 0.5f);

    if (ImGui::Button("\u2713 OK", ImVec2(btn_width, btn_height))) {
        confirmed = true;
        Close();
    }

    ImGui::SameLine();
    if (ImGui::Button("\u2715 Cancel", ImVec2(btn_width, btn_height))) {
        Close();
    }

    return confirmed;
}

bool SensorListEditorModal::IsSensorSelected(std::size_t index) const {
    return index < selection_.size() && selection_[index];
}

void SensorListEditorModal::ToggleSensorSelection(std::size_t index) {
    if (index < selection_.size()) {
        selection_[index] = !selection_[index];
    }
}

} // namespace UI::Modals
