#include "ConnectionModal.h"

#include "../../app_data.h"
#include "../../utils/Colors.h"
#include "../../utils/Layout.h"
#include "imgui.h"
#include <algorithm>
#include <cassert>

namespace {
constexpr const char *kPopupName = "ECU Connection";
constexpr size_t kMaxDisplayPorts = 20;
} // namespace

namespace UI::Modals {

ConnectionModal::ConnectionModal(
    std::unique_ptr<IPortEnumerator> port_enum) noexcept
    : port_enum_(std::move(port_enum)) {
    assert(port_enum_ && "Port enumerator must not be null");
}

void ConnectionModal::SetECUInfo(std::string_view model,
                                 std::string_view version) noexcept {
    ecu_model_.assign(model);
    ecu_version_.assign(version);
    state_ = State::Connected;
}

void ConnectionModal::SetConnectionError(std::string_view message) noexcept {
    error_message_.assign(message);
    state_ = State::Error;
    if (on_error_) {
        on_error_(error_message_);
    }
}

std::optional<ConnectionConfig> ConnectionModal::Render() noexcept {
    if (!should_open_) {
        state_ = State::Closed;
        return std::nullopt;
    }

    if (!ports_initialized_) {
        RefreshPorts();
        ports_initialized_ = true;
        state_ = State::Idle;
    }

    const ImVec2 modal_size = Layout::GetModalSize(
        Layout::Modal::MAX_WIDTH_RATIO, Layout::Modal::MAX_HEIGHT_RATIO);
    ImGui::SetNextWindowSize(modal_size, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        ImVec2((ImGui::GetIO().DisplaySize.x - modal_size.x) * 0.5f,
               (ImGui::GetIO().DisplaySize.y - modal_size.y) * 0.5f),
        ImGuiCond_FirstUseEver);

    std::optional<ConnectionConfig> result;

    const bool popup_open = ImGui::BeginPopupModal(
        kPopupName, &should_open_,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    if (!popup_open) {
        return std::nullopt;
    }

    RenderTitle();
    ImGui::Separator();

    RenderPortList();
    ImGui::Spacing();
    RenderSimulationToggle();
    ImGui::Spacing();
    RenderECUInfoFeedback();
    ImGui::Spacing();

    if (state_ == State::Error) {
        RenderErrorMessage();
        ImGui::Spacing();
    }

    ImGui::Separator();

    if (RenderActionButtons()) {
        result = ConnectionConfig{};
        if (!ports_.empty() && selected_port_idx_ >= 0 &&
            static_cast<size_t>(selected_port_idx_) < ports_.size()) {
            result->port = ports_[static_cast<size_t>(selected_port_idx_)].port_name;
        }
        result->use_simulation = use_simulation_;
        if (!ecu_model_.empty()) {
            result->ecu_model = ecu_model_;
        }
        if (!ecu_version_.empty()) {
            result->ecu_version = ecu_version_;
        }
        should_open_ = false;
    }

    ImGui::EndPopup();
    return result;
}

void ConnectionModal::RenderTitle() const noexcept {
    ImGui::TextColored(Colors::Status::OK, "Select port and mode");
    if (!ports_.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(Colors::Status::WARN, "(%zu available)",
                           ports_.size());
    }
}

void ConnectionModal::RenderPortList() noexcept {
    ImGui::Text("Available Ports:");

    if (ports_.empty()) {
        ImGui::TextColored(Colors::Status::WARN, "No serial ports found");
        ImGui::TextColored(Colors::Status::WARN,
                           "Check USB connection or drivers");
        return;
    }

    const size_t count = std::min(ports_.size(), kMaxDisplayPorts);
    for (size_t i = 0; i < count; ++i) {
        const auto &port = ports_[i];
        const bool disabled = !port.is_available;
        const bool is_selected = static_cast<int>(i) == selected_port_idx_;
        const char *bullet = is_selected ? "●" : "○";
        const ImVec4 color =
            is_selected ? Colors::Status::OK : Colors::Status::STALE;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, disabled ? 0.4f : 1.0f);
        ImGui::TextColored(color, "%s %s", bullet, port.port_name.c_str());
        if (!port.description.empty()) {
            ImGui::SameLine();
            ImGui::Text("(%s)", port.description.c_str());
        }
        if (disabled) {
            ImGui::SameLine();
            ImGui::TextColored(Colors::Status::CRITICAL, "[in use]");
        }

        if (!disabled && ImGui::IsItemClicked()) {
            selected_port_idx_ = static_cast<int>(i);
        }
        ImGui::PopStyleVar();
    }
}

void ConnectionModal::RenderSimulationToggle() noexcept {
    ImGui::Text("Mode:");

    bool real_ecu_selected = !use_simulation_;
    if (ImGui::RadioButton("Real ECU", real_ecu_selected)) {
        use_simulation_ = false;
    }
    ImGui::SameLine();
    ImGui::TextColored(Colors::Status::OK, "Use actual ECU data");

    bool sim_selected = use_simulation_;
    if (ImGui::RadioButton("Simulation", sim_selected)) {
        use_simulation_ = true;
    }
    ImGui::SameLine();
    ImGui::TextColored(Colors::Status::WARN, "Test data only");
}

void ConnectionModal::RenderECUInfoFeedback() const noexcept {
    ImGui::Text("ECU Information:");
    if (state_ == State::Connecting) {
        ImGui::TextColored(Colors::Status::WARN, "Waiting for ECU response...");
        return;
    }
    if (ecu_model_.empty()) {
        ImGui::TextColored(Colors::Status::STALE,
                           "Connect to see ECU model and firmware");
        return;
    }

    ImGui::TextColored(Colors::Status::OK, "✓ Model: %s", ecu_model_.c_str());
    if (!ecu_version_.empty()) {
        ImGui::TextColored(Colors::Status::OK, "  FW: %s",
                           ecu_version_.c_str());
    }
}

void ConnectionModal::RenderErrorMessage() const noexcept {
    ImGui::TextColored(Colors::Status::CRITICAL, "Connection error:");
    ImGui::TextColored(Colors::Status::CRITICAL, "  %s",
                       error_message_.c_str());
}

bool ConnectionModal::RenderActionButtons() noexcept {
    bool confirmed = false;

    const float btn_width = Layout::Button::MODAL_WIDTH;
    const float btn_height = Layout::Button::MODAL_HEIGHT;
    const float spacing = Layout::Padding::MEDIUM;
    const float total = btn_width * 2 + spacing;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - total) * 0.5f);

    const bool has_ports = !ports_.empty();

    if (ImGui::Button("\u2713 Connect", ImVec2(btn_width, btn_height)) &&
        has_ports) {
        state_ = State::Connecting;
        confirmed = true;
        if (on_connect_ && selected_port_idx_ >= 0 &&
            static_cast<size_t>(selected_port_idx_) < ports_.size()) {
            on_connect_(ports_[static_cast<size_t>(selected_port_idx_)].port_name,
                        use_simulation_);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("\u2715 Cancel", ImVec2(btn_width, btn_height))) {
        should_open_ = false;
        state_ = State::Closed;
    }

    return confirmed;
}

void ConnectionModal::RefreshPorts() noexcept {
    ports_ = port_enum_->EnumeratePorts();
    if (ports_.empty()) {
        selected_port_idx_ = -1;
        return;
    }
    selected_port_idx_ = 0;
}

std::vector<IPortEnumerator::PortInfo>
SystemPortEnumerator::EnumeratePorts() const noexcept {
    // TODO: implement platform-specific enumeration
    return {{"COM3", "USB Serial", true}, {"/dev/ttyUSB0", "USB Serial", true}};
}

} // namespace UI::Modals
