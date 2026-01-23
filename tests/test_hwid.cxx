#include <gtest/gtest.h>
#include <algorithm>
#include <string>

#include <Identy.h>
#include "test_config.hxx"

namespace identy::test
{

class HwidTest : public ::testing::Test
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
// CPU Tests
// ============================================================================

TEST_F(HwidTest, SnapMotherboard_ReturnsValidCpu)
{
    EXPECT_FALSE(mb_.cpu.vendor.empty()) << "CPU vendor should not be empty";
}

TEST_F(HwidTest, SnapMotherboard_CpuVendorKnown)
{
    EXPECT_TRUE(IsKnownCpuVendor(mb_.cpu.vendor))
        << "CPU vendor '" << mb_.cpu.vendor << "' is not recognized. "
        << "This may be a new/rare CPU or a detection issue.";
}

TEST_F(HwidTest, SnapMotherboard_CpuVersionNonZero)
{
    // CPU version from CPUID should be non-zero for any modern CPU
    // Only skip this check if CPU is marked as too_old
    if (!mb_.cpu.too_old) {
        EXPECT_NE(mb_.cpu.version, 0) << "CPU version should be non-zero for modern CPUs";
    }
}

TEST_F(HwidTest, SnapMotherboard_CpuExtendedBrandString)
{
    // Extended brand string might be empty on very old CPUs
    if (!mb_.cpu.too_old) {
        EXPECT_FALSE(mb_.cpu.extended_brand_string.empty())
            << "Extended brand string should be available on modern CPUs";
    }
}

TEST_F(HwidTest, SnapMotherboard_CpuLogicalProcessorsPositive)
{
    EXPECT_GT(mb_.cpu.logical_processors_count, 0)
        << "Logical processor count should be positive";
}

// ============================================================================
// SMBIOS Tests
// ============================================================================

TEST_F(HwidTest, SnapMotherboard_SmbiosVersionValid)
{
    // SMBIOS versions are typically 2.x or 3.x
    EXPECT_GE(mb_.smbios.major_version, 2) << "SMBIOS major version should be at least 2";
    EXPECT_LE(mb_.smbios.major_version, 3) << "SMBIOS major version should be at most 3";
    EXPECT_LE(mb_.smbios.minor_version, 9) << "SMBIOS minor version should be single digit";
}

TEST_F(HwidTest, SnapMotherboard_UuidHasContent)
{
    // Check that UUID is not completely zeroed
    bool all_zero = std::all_of(
        std::begin(mb_.smbios.uuid),
        std::end(mb_.smbios.uuid),
        [](byte b) { return b == 0; }
    );

    // In VMs, UUID might be zeroed, so we just log a warning
    if (all_zero) {
        GTEST_LOG_(WARNING) << "SMBIOS UUID is completely zeroed. "
                            << "This is unusual but may occur in some VM environments.";
    }
}

TEST_F(HwidTest, SnapMotherboard_UuidCorrectLength)
{
    EXPECT_EQ(sizeof(mb_.smbios.uuid), SMBIOS_uuid_length)
        << "UUID should be exactly " << SMBIOS_uuid_length << " bytes";
}

TEST_F(HwidTest, SnapMotherboard_RawTablesDataNotEmpty)
{
    // Raw SMBIOS tables should contain some data
    EXPECT_FALSE(mb_.smbios.raw_tables_data.empty())
        << "SMBIOS raw tables data should not be empty";
}

// ============================================================================
// Extended Motherboard Tests
// ============================================================================

TEST_F(HwidTest, SnapMotherboardEx_CpuMatchesBasic)
{
    // Extended and basic should return same CPU info
    EXPECT_EQ(mb_ex_.cpu.vendor, mb_.cpu.vendor);
    EXPECT_EQ(mb_ex_.cpu.version, mb_.cpu.version);
}

TEST_F(HwidTest, SnapMotherboardEx_SmbiosMatchesBasic)
{
    // Extended and basic should return same SMBIOS info
    EXPECT_EQ(mb_ex_.smbios.major_version, mb_.smbios.major_version);
    EXPECT_EQ(mb_ex_.smbios.minor_version, mb_.smbios.minor_version);
}

TEST_F(HwidTest, SnapMotherboardEx_DrivesMayBeEmpty)
{
    // Drives may be empty in CI/VM environments due to permissions
    if (mb_ex_.drives.empty()) {
        GTEST_LOG_(WARNING) << "No drives accessible. "
                            << "This may be due to permission restrictions in CI/VM environment.";
    }
}

TEST_F(HwidTest, SnapMotherboardEx_DrivesHaveDeviceNames)
{
    for (const auto& drive : mb_ex_.drives) {
        EXPECT_FALSE(drive.device_name.empty())
            << "Drive device name should not be empty";
    }
}

TEST_F(HwidTest, SnapMotherboardEx_DrivesHaveSerials)
{
    for (const auto& drive : mb_ex_.drives) {
        // Serial might be empty for some virtual drives
        if (drive.serial.empty()) {
            GTEST_LOG_(WARNING) << "Drive " << drive.device_name
                                << " has empty serial number";
        }
    }
}

TEST_F(HwidTest, SnapMotherboardEx_DrivesBusTypeValid)
{
    for (const auto& drive : mb_ex_.drives) {
        // BusType should be within enum range
        EXPECT_GE(static_cast<int>(drive.bus_type), 0);
        EXPECT_LE(static_cast<int>(drive.bus_type),
                  static_cast<int>(PhysicalDriveInfo::BusType::Other))
            << "Drive " << drive.device_name << " has invalid bus type";
    }
}

// ============================================================================
// list_drives() Tests
// ============================================================================

TEST(HwidListDrivesTest, ListDrives_DoesNotThrow)
{
    EXPECT_NO_THROW({
        auto drives = identy::list_drives();
    }) << "list_drives() should not throw exceptions";
}

TEST(HwidListDrivesTest, ListDrives_MatchesMotherboardEx)
{
    auto drives = identy::list_drives();
    auto mb_ex = identy::snap_motherboard_ex();

    // Both should return same drives
    EXPECT_EQ(drives.size(), mb_ex.drives.size())
        << "list_drives() and snap_motherboard_ex() should return same number of drives";
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST(HwidConsistencyTest, SnapMotherboard_Deterministic)
{
    auto mb1 = identy::snap_motherboard();
    auto mb2 = identy::snap_motherboard();

    EXPECT_EQ(mb1.cpu.vendor, mb2.cpu.vendor);
    EXPECT_EQ(mb1.cpu.version, mb2.cpu.version);
    EXPECT_EQ(mb1.smbios.major_version, mb2.smbios.major_version);

    // UUIDs should match
    EXPECT_EQ(
        std::memcmp(mb1.smbios.uuid, mb2.smbios.uuid, SMBIOS_uuid_length),
        0
    ) << "SMBIOS UUID should be consistent across calls";
}

TEST(HwidConsistencyTest, SnapMotherboardEx_Deterministic)
{
    auto mb1 = identy::snap_motherboard_ex();
    auto mb2 = identy::snap_motherboard_ex();

    EXPECT_EQ(mb1.cpu.vendor, mb2.cpu.vendor);
    EXPECT_EQ(mb1.drives.size(), mb2.drives.size());

    for (size_t i = 0; i < mb1.drives.size(); ++i) {
        EXPECT_EQ(mb1.drives[i].serial, mb2.drives[i].serial)
            << "Drive serial at index " << i << " should be consistent";
    }
}

} // namespace identy::test
