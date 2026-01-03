#include "../core/MockECUBackend.h"
#include <gtest/gtest.h>

// GoogleTest suite for MockECUBackend
// Tests cover:
// - ReadDTCs success and error cases
// - ClearDTCs functionality
// - Simulation mode control
// - DTC injection for testing
// - Boundary conditions (MAX_DTCS)

class MockECUBackendTest : public ::testing::Test {
  protected:
    void SetUp() override {
        backend_ = &MockECUBackend::Instance();
        backend_->SetSimulationMode(true);
        backend_->ClearSimulatedDTCs();
    }

    void TearDown() override {
        backend_->ClearSimulatedDTCs();
        backend_->SetSimulationMode(true);
    }

    MockECUBackend *backend_;
};

// Test: ReadDTCs returns success when simulation enabled
TEST_F(MockECUBackendTest, ReadDTCs_ReturnsSuccess) {
    auto result = backend_->ReadDTCs();

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_FALSE(result.dtcs.empty()); // Should have default DTCs
}

// Test: ReadDTCs returns default DTCs when empty
TEST_F(MockECUBackendTest, ReadDTCs_ReturnsDefaultDTCs_WhenEmpty) {
    auto result = backend_->ReadDTCs();

    EXPECT_TRUE(result.success);
    EXPECT_GE(result.dtcs.size(), 1); // At least one default DTC

    // Verify DTC structure
    if (!result.dtcs.empty()) {
        const auto &first_dtc = result.dtcs[0];
        EXPECT_FALSE(first_dtc.code.empty());
        EXPECT_FALSE(first_dtc.description.empty());
    }
}

// Test: ClearDTCs removes all DTCs
TEST_F(MockECUBackendTest, ClearDTCs_RemovesAllDTCs) {
    // First, inject some DTCs
    backend_->InjectRandomDTCs(5);
    auto read_result = backend_->ReadDTCs();
    EXPECT_GE(read_result.dtcs.size(), 5);

    // Now clear them
    auto clear_result = backend_->ClearDTCs();
    EXPECT_TRUE(clear_result.success);
    EXPECT_TRUE(clear_result.error_message.empty());

    // Verify DTCs are cleared
    read_result = backend_->ReadDTCs();

    EXPECT_TRUE(read_result.success);
    EXPECT_TRUE(read_result.dtcs.empty());
}

// Test: Simulation mode disabled returns error
TEST_F(MockECUBackendTest, ReadDTCs_ReturnsError_WhenSimulationDisabled) {
    backend_->SetSimulationMode(false);

    auto result = backend_->ReadDTCs();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_TRUE(result.dtcs.empty());
}

// Test: ClearDTCs returns error when simulation disabled
TEST_F(MockECUBackendTest, ClearDTCs_ReturnsError_WhenSimulationDisabled) {
    backend_->SetSimulationMode(false);

    auto result = backend_->ClearDTCs();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// Test: InjectRandomDTCs adds specified number of DTCs
TEST_F(MockECUBackendTest, InjectRandomDTCs_AddsCorrectCount) {
    constexpr int inject_count = 10;

    backend_->InjectRandomDTCs(inject_count);
    auto result = backend_->ReadDTCs();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.dtcs.size(), inject_count);
}

// Test: InjectRandomDTCs respects MAX_DTCS boundary
TEST_F(MockECUBackendTest, InjectRandomDTCs_RespectsMaxBoundary) {
    // NASA Power of 10: Bounded operations
    const int excessive_count = MockECUBackend::MAX_DTCS + 50;

    backend_->InjectRandomDTCs(excessive_count);
    auto result = backend_->ReadDTCs();

    EXPECT_TRUE(result.success);
    EXPECT_LE(result.dtcs.size(), MockECUBackend::MAX_DTCS);
}

// Test: Generated DTCs have valid structure
TEST_F(MockECUBackendTest, GeneratedDTCs_HaveValidStructure) {
    backend_->InjectRandomDTCs(3);
    auto result = backend_->ReadDTCs();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.dtcs.size(), 3);

    for (const auto &dtc : result.dtcs) {
        // DTC code should not be empty
        EXPECT_FALSE(dtc.code.empty());

        // DTC code should start with valid prefix (P, C, B, U)
        EXPECT_TRUE(dtc.code[0] == 'P' || dtc.code[0] == 'C' ||
                    dtc.code[0] == 'B' || dtc.code[0] == 'U');

        // Description should not be empty
        EXPECT_FALSE(dtc.description.empty());

        // Active status should be boolean (always true or false)
        // This test just verifies the field exists and can be accessed
        EXPECT_TRUE(dtc.active == true || dtc.active == false);
    }
}

// Test: ClearSimulatedDTCs removes all internal DTCs
TEST_F(MockECUBackendTest, ClearSimulatedDTCs_RemovesInternalState) {
    backend_->InjectRandomDTCs(5);

    backend_->ClearSimulatedDTCs();

    // After clearing internal state, ReadDTCs should reinitialize defaults
    auto result = backend_->ReadDTCs();
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.dtcs.empty()); // Should have defaults again
}

// Main function for running tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
