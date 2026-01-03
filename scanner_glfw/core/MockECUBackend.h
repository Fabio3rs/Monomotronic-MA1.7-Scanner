#pragma once

#include "../app_data.h" // For SimulatedDTC
#include <string>
#include <vector>

// C++ Core Guidelines:
// - E.2: Return structured result with error information (no exceptions)
// - I.11: Singleton pattern for global ECU backend access
// - R.1: RAII for resource management
// - F.15: Simple and conventional interface

// NASA Power of 10:
// - Bounded operations: MAX_DTCS limit
// - Simple control flow: no recursion, clear state machine

class MockECUBackend {
  public:
    // E.2: Structured error handling - return success status + error message
    struct DTCReadResult {
        bool success;
        std::vector<SimulatedDTC> dtcs;
        std::string error_message;
    };

    struct DTCClearResult {
        bool success;
        std::string error_message;
    };

    // I.11: Singleton instance - testable via SetSimulationMode
    static MockECUBackend &Instance();

    // F.15: Simple, conventional interface
    [[nodiscard]] DTCReadResult ReadDTCs();
    [[nodiscard]] DTCClearResult ClearDTCs();

    // Simulation control for testing
    void SetSimulationMode(bool enabled);
    void InjectRandomDTCs(int count);
    void ClearSimulatedDTCs();

    // NASA Power of 10: Bounded operations
    static constexpr size_t MAX_DTCS = 100;

  private:
    // Private constructor for singleton
    MockECUBackend() = default;

    // Delete copy/move (singleton)
    MockECUBackend(const MockECUBackend &) = delete;
    MockECUBackend &operator=(const MockECUBackend &) = delete;
    MockECUBackend(MockECUBackend &&) = delete;
    MockECUBackend &operator=(MockECUBackend &&) = delete;

    // Internal state
    std::vector<SimulatedDTC> simulated_dtcs_;
    bool simulation_enabled_ = true;
    bool seed_defaults_on_read_ = true;

    // Helper methods
    void InitializeDefaultDTCs();
    SimulatedDTC GenerateRandomDTC(int index);
};
