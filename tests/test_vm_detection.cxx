#include <gtest/gtest.h>
#include <algorithm>

#include <Identy.h>
#include "test_config.hxx"

namespace identy::test
{

class VMDetectionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mb_ = identy::snap_motherboard();
        mb_ex_ = identy::snap_motherboard_ex();
    }

    identy::Motherboard mb_;
    identy::MotherboardEx mb_ex_;
};

// ============================================================================
// Environment-Aware Tests
// ============================================================================

TEST_F(VMDetectionTest, AssumeVirtual_MatchesExpectedEnvironment)
{
    bool is_vm = identy::vm::assume_virtual(mb_);

    if constexpr (kExpectVirtualMachine) {
        EXPECT_TRUE(is_vm)
            << "Expected VM detection in CI/cloud environment. "
            << "If running locally, use -DIDENTY_TEST_EXPECT_BAREMETAL=ON";
    } else {
        EXPECT_FALSE(is_vm)
            << "Expected bare metal detection on local machine. "
            << "If running in VM, use -DIDENTY_TEST_EXPECT_VM=ON";
    }
}

TEST_F(VMDetectionTest, AssumeVirtualEx_MatchesExpectedEnvironment)
{
    bool is_vm = identy::vm::assume_virtual(mb_ex_);

    if constexpr (kExpectVirtualMachine) {
        EXPECT_TRUE(is_vm)
            << "Expected VM detection (extended) in CI/cloud environment";
    } else {
        EXPECT_FALSE(is_vm)
            << "Expected bare metal detection (extended) on local machine";
    }
}

TEST_F(VMDetectionTest, AnalyzeFull_ReturnsValidVerdict)
{
    auto verdict = identy::vm::analyze_full(mb_);

    // Confidence should be a valid enum value
    EXPECT_GE(static_cast<int>(verdict.confidence),
              static_cast<int>(vm::VMConfidence::Unlikely));
    EXPECT_LE(static_cast<int>(verdict.confidence),
              static_cast<int>(vm::VMConfidence::DefinitelyVM));
}

TEST_F(VMDetectionTest, AnalyzeFull_ConfidenceMatchesEnvironment)
{
    auto verdict = identy::vm::analyze_full(mb_);

    if constexpr (kExpectVirtualMachine) {
        EXPECT_GE(verdict.confidence, vm::VMConfidence::Probable)
            << "In VM environment, confidence should be at least Probable";
    } else {
        EXPECT_LE(verdict.confidence, vm::VMConfidence::Possible)
            << "On bare metal, confidence should be at most Possible";
    }
}

TEST_F(VMDetectionTest, AnalyzeFull_DetectionFlagsInVM)
{
    if constexpr (kExpectVirtualMachine) {
        auto verdict = identy::vm::analyze_full(mb_);
        EXPECT_FALSE(verdict.detections.empty())
            << "In VM environment, at least one detection flag should be set";
    }
}

TEST_F(VMDetectionTest, AnalyzeFullEx_ReturnsValidVerdict)
{
    auto verdict = identy::vm::analyze_full(mb_ex_);

    EXPECT_GE(static_cast<int>(verdict.confidence),
              static_cast<int>(vm::VMConfidence::Unlikely));
    EXPECT_LE(static_cast<int>(verdict.confidence),
              static_cast<int>(vm::VMConfidence::DefinitelyVM));
}

// ============================================================================
// HeuristicVerdict Tests
// ============================================================================

TEST_F(VMDetectionTest, HeuristicVerdict_IsVirtualConsistentWithConfidence)
{
    auto verdict = identy::vm::analyze_full(mb_);

    bool is_virtual = verdict.is_virtual();
    bool confidence_indicates_vm = verdict.confidence >= vm::VMConfidence::Probable;

    EXPECT_EQ(is_virtual, confidence_indicates_vm)
        << "is_virtual() should be true iff confidence >= Probable";
}

TEST_F(VMDetectionTest, HeuristicVerdict_DetectionsMatchConfidence)
{
    auto verdict = identy::vm::analyze_full(mb_);

    // If confidence is DefinitelyVM, there should be detections
    if (verdict.confidence == vm::VMConfidence::DefinitelyVM) {
        EXPECT_FALSE(verdict.detections.empty())
            << "DefinitelyVM confidence requires at least one detection flag";
    }

    // If no detections, confidence should be Unlikely
    if (verdict.detections.empty()) {
        EXPECT_EQ(verdict.confidence, vm::VMConfidence::Unlikely)
            << "Empty detections should result in Unlikely confidence";
    }
}

// ============================================================================
// VMConfidence Enum Tests
// ============================================================================

TEST(VMConfidenceTest, OrderingCorrect)
{
    EXPECT_LT(vm::VMConfidence::Unlikely, vm::VMConfidence::Possible);
    EXPECT_LT(vm::VMConfidence::Possible, vm::VMConfidence::Probable);
    EXPECT_LT(vm::VMConfidence::Probable, vm::VMConfidence::DefinitelyVM);
}

// ============================================================================
// DefaultWeightPolicy Tests
// ============================================================================

TEST(DefaultWeightPolicyTest, GetStrength_AllFlagsHandled)
{
    // Test that all VMFlags have a defined strength
    const vm::VMFlags all_flags[] = {
        vm::VMFlags::Cpu_Hypervisor_bit,
        vm::VMFlags::Cpu_Hypervisor_signature,
        vm::VMFlags::SMBIOS_SuspiciousManufacturer,
        vm::VMFlags::SMBIOS_SuspiciousUUID,
        vm::VMFlags::SMBIOS_UUIDTotallyZeroed,
        vm::VMFlags::Storage_SuspiciousSerial,
        vm::VMFlags::Storage_BusTypeIsVirtual,
        vm::VMFlags::Storage_AllDrivesBusesVirtual,
        vm::VMFlags::Storage_BusTypeUncommon,
        vm::VMFlags::Storage_ProductIdKnownVM,
        vm::VMFlags::Storage_AllDrivesVendorProductKnownVM,
        vm::VMFlags::Platform_WindowsRegistry,
        vm::VMFlags::Platform_LinuxDevices,
        vm::VMFlags::Platform_VirtualNetworkAdaptersPresent,
        vm::VMFlags::Platform_OnlyVirtualNetworkAdapters,
        vm::VMFlags::Platform_AccessToNetworkDevicesDenied,
        vm::VMFlags::Platform_HyperVIsolation,
    };

    for (auto flag : all_flags) {
        auto strength = vm::DefaultWeightPolicy::get_strength(flag);
        // Should be one of the valid strengths
        EXPECT_GE(static_cast<int>(strength),
                  static_cast<int>(vm::detail::FlagStrength::Weak));
        EXPECT_LE(static_cast<int>(strength),
                  static_cast<int>(vm::detail::FlagStrength::Critical));
    }
}

TEST(DefaultWeightPolicyTest, Calculate_NoFlags_ReturnsUnlikely)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 0, 0, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Unlikely);
}

TEST(DefaultWeightPolicyTest, Calculate_Critical_ReturnsDefinitelyVM)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 0, 0, true);
    EXPECT_EQ(confidence, vm::VMConfidence::DefinitelyVM);
}

TEST(DefaultWeightPolicyTest, Calculate_TwoStrong_ReturnsDefinitelyVM)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 0, 2, false);
    EXPECT_EQ(confidence, vm::VMConfidence::DefinitelyVM);
}

TEST(DefaultWeightPolicyTest, Calculate_OneStrong_ReturnsProbable)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 0, 1, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Probable);
}

TEST(DefaultWeightPolicyTest, Calculate_ThreeMedium_ReturnsProbable)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 3, 0, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Probable);
}

TEST(DefaultWeightPolicyTest, Calculate_OneMedium_ReturnsPossible)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(0, 1, 0, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Possible);
}

TEST(DefaultWeightPolicyTest, Calculate_TwoWeak_ReturnsPossible)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(2, 0, 0, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Possible);
}

TEST(DefaultWeightPolicyTest, Calculate_OneWeak_ReturnsUnlikely)
{
    auto confidence = vm::DefaultWeightPolicy::calculate(1, 0, 0, false);
    EXPECT_EQ(confidence, vm::VMConfidence::Unlikely);
}

// ============================================================================
// Custom Weight Policy Tests
// ============================================================================

struct TestWeightPolicy
{
    static constexpr vm::detail::FlagStrength get_strength(vm::VMFlags) noexcept
    {
        return vm::detail::FlagStrength::Weak;
    }

    static constexpr vm::VMConfidence calculate(int, int, int, bool) noexcept
    {
        return vm::VMConfidence::Unlikely;
    }
};

// Verify the concept is satisfied
static_assert(vm::WeightPolicy<TestWeightPolicy>,
    "TestWeightPolicy should satisfy WeightPolicy concept");
static_assert(vm::WeightPolicy<vm::DefaultWeightPolicy>,
    "DefaultWeightPolicy should satisfy WeightPolicy concept");

TEST(CustomWeightPolicyTest, CustomPolicy_ConceptSatisfied)
{
    // Test that custom policies satisfy the WeightPolicy concept
    // This is a compile-time check - if it compiles, the test passes
    static_assert(vm::WeightPolicy<TestWeightPolicy>,
        "TestWeightPolicy should satisfy WeightPolicy concept");

    // Verify the policy functions work correctly
    auto strength = TestWeightPolicy::get_strength(vm::VMFlags::Cpu_Hypervisor_bit);
    EXPECT_EQ(strength, vm::detail::FlagStrength::Weak);

    auto confidence = TestWeightPolicy::calculate(10, 10, 10, true);
    EXPECT_EQ(confidence, vm::VMConfidence::Unlikely);
}

// ============================================================================
// Heuristic Concept Tests
// ============================================================================

static_assert(vm::Heuristic<vm::DefaultHeuristic<>>,
    "DefaultHeuristic should satisfy Heuristic concept");
static_assert(vm::HeuristicEx<vm::DefaultHeuristicEx<>>,
    "DefaultHeuristicEx should satisfy HeuristicEx concept");

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(VMDetectionTest, AssumeVirtual_Deterministic)
{
    bool result1 = identy::vm::assume_virtual(mb_);
    bool result2 = identy::vm::assume_virtual(mb_);

    EXPECT_EQ(result1, result2) << "assume_virtual should be deterministic";
}

TEST_F(VMDetectionTest, AnalyzeFull_Deterministic)
{
    auto verdict1 = identy::vm::analyze_full(mb_);
    auto verdict2 = identy::vm::analyze_full(mb_);

    EXPECT_EQ(verdict1.confidence, verdict2.confidence)
        << "analyze_full confidence should be deterministic";
    EXPECT_EQ(verdict1.detections.size(), verdict2.detections.size())
        << "analyze_full detections count should be deterministic";
}

// ============================================================================
// Extended vs Basic Comparison
// ============================================================================

TEST_F(VMDetectionTest, ExtendedVsBasic_ConsistentDirection)
{
    auto verdict_basic = identy::vm::analyze_full(mb_);
    auto verdict_ex = identy::vm::analyze_full(mb_ex_);

    // Extended analysis may have more detections but should generally agree
    // on whether it's a VM or not
    EXPECT_EQ(verdict_basic.is_virtual(), verdict_ex.is_virtual())
        << "Basic and extended analysis should agree on VM detection";
}

} // namespace identy::test
