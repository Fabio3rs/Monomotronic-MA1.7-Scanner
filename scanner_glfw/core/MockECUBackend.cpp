#include "MockECUBackend.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>

// I.11: Singleton implementation
MockECUBackend &MockECUBackend::Instance() {
    static MockECUBackend instance;
    return instance;
}

// E.2: Return structured result with success status and error message
MockECUBackend::DTCReadResult MockECUBackend::ReadDTCs() {
    DTCReadResult result;

    if (!simulation_enabled_) {
        result.success = false;
        result.error_message = "Simulation mode disabled - cannot read DTCs";
        return result;
    }

    // Initialize default DTCs only once to allow clear to persist.
    if (simulated_dtcs_.empty() && seed_defaults_on_read_) {
        InitializeDefaultDTCs();
        seed_defaults_on_read_ = false;
    }

    result.success = true;
    result.dtcs = simulated_dtcs_;
    result.error_message = "";

    return result;
}

MockECUBackend::DTCClearResult MockECUBackend::ClearDTCs() {
    DTCClearResult result;

    if (!simulation_enabled_) {
        result.success = false;
        result.error_message = "Simulation mode disabled - cannot clear DTCs";
        return result;
    }

    // E.3: Clear state atomically
    simulated_dtcs_.clear();
    seed_defaults_on_read_ = false;

    result.success = true;
    result.error_message = "";

    return result;
}

void MockECUBackend::SetSimulationMode(bool enabled) {
    simulation_enabled_ = enabled;
}

void MockECUBackend::InjectRandomDTCs(int count) {
    // NASA Power of 10: Bounded loop with clear limit
    const int safe_count =
        std::min(count, static_cast<int>(MAX_DTCS) -
                            static_cast<int>(simulated_dtcs_.size()));

    for (int i = 0; i < safe_count; ++i) {
        simulated_dtcs_.push_back(
            GenerateRandomDTC(static_cast<int>(simulated_dtcs_.size())));
    }

    if (safe_count > 0) {
        seed_defaults_on_read_ = false;
    }
}

void MockECUBackend::ClearSimulatedDTCs() {
    simulated_dtcs_.clear();
    seed_defaults_on_read_ = true;
}

// Private helper methods

void MockECUBackend::InitializeDefaultDTCs() {
    // ES.10: Named constants for clarity
    simulated_dtcs_ = {
        {"P0171", "System Too Lean (Bank 1)", true},
        {"P0301", "Cylinder 1 Misfire Detected", true},
        {"P0420", "Catalyst System Efficiency Below Threshold (Bank 1)", false},
        {"P0562", "System Voltage Low", false},
        {"C0040", "Right Front Wheel Speed Sensor Circuit", true}};
}

SimulatedDTC MockECUBackend::GenerateRandomDTC(int index) {
    // Simple DTC code generation for testing
    const char *prefixes[] = {"P0", "P1", "C0", "B0", "U0"};
    const char *descriptions[] = {
        "Engine Management System Fault", "Transmission Control Fault",
        "ABS System Malfunction",         "Airbag System Warning",
        "Network Communication Error",    "Sensor Circuit Malfunction",
        "Actuator Control Fault",         "Fuel System Lean/Rich",
        "Emission Control System",        "Ignition System Misfire"};

    const int prefix_idx = index % 5;
    const int desc_idx = index % 10;
    const int code_num = (index * 37 + 100) % 1000; // Simple hash for variety

    SimulatedDTC dtc;
    dtc.code = std::string(prefixes[prefix_idx]) + std::to_string(code_num);
    dtc.description = descriptions[desc_idx];
    dtc.active = (index % 3) == 0; // Every 3rd DTC is active

    return dtc;
}
