#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Modals {

// Simple abstraction to enumerate available ports (DI-friendly for tests)
class IPortEnumerator {
  public:
    struct PortInfo {
        std::string port_name;
        std::string description;
        bool is_available = true;
    };

    virtual ~IPortEnumerator() = default;
    virtual std::vector<PortInfo> EnumeratePorts() const noexcept = 0;
};

struct ConnectionConfig {
    std::string port;
    bool use_simulation = false;
    std::optional<std::string> ecu_model;
    std::optional<std::string> ecu_version;
};

class ConnectionModal {
  public:
    enum class State { Closed, Idle, Connecting, Connected, Error };

    explicit ConnectionModal(std::unique_ptr<IPortEnumerator> port_enum) noexcept;
    ~ConnectionModal() = default;

    void Show() noexcept { should_open_ = true; }
    void Close() noexcept { should_open_ = false; }
    bool IsOpen() const noexcept { return should_open_; }
    State GetState() const noexcept { return state_; }

    // UI callbacks
    void SetConnectionCallback(
        std::function<void(std::string_view port, bool simulate)> cb) noexcept {
        on_connect_ = std::move(cb);
    }
    void SetErrorCallback(
        std::function<void(std::string_view message)> cb) noexcept {
        on_error_ = std::move(cb);
    }

    // Feedback from backend
    void SetECUInfo(std::string_view model, std::string_view version) noexcept;
    void SetConnectionError(std::string_view message) noexcept;

    // Render modal and return config when user confirms Connect
    std::optional<ConnectionConfig> Render() noexcept;

  private:
    // State
    State state_ = State::Closed;
    bool should_open_ = false;
    bool ports_initialized_ = false;
    bool use_simulation_ = false;
    int selected_port_idx_ = 0;
    std::string error_message_;
    std::string ecu_model_;
    std::string ecu_version_;

    // Data + deps
    std::vector<IPortEnumerator::PortInfo> ports_;
    std::unique_ptr<IPortEnumerator> port_enum_;
    std::function<void(std::string_view, bool)> on_connect_;
    std::function<void(std::string_view)> on_error_;

    // Helpers
    void RefreshPorts() noexcept;
    void RenderTitle() const noexcept;
    void RenderPortList() noexcept;
    void RenderSimulationToggle() noexcept;
    void RenderECUInfoFeedback() const noexcept;
    void RenderErrorMessage() const noexcept;
    bool RenderActionButtons() noexcept;
};

// Minimal concrete enumerator placeholder (platform-specific impl TBD)
class SystemPortEnumerator : public IPortEnumerator {
  public:
    std::vector<PortInfo> EnumeratePorts() const noexcept override;
};

} // namespace UI::Modals
