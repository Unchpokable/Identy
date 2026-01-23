#include <gtest/gtest.h>
#include <sstream>
#include <cstring>

#include <Identy.h>
#include "test_config.hxx"

namespace identy::test
{

class IOTest : public ::testing::Test
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
// write_text() Tests
// ============================================================================

TEST_F(IOTest, WriteText_ProducesOutput)
{
    std::ostringstream oss;
    io::write_text(oss, mb_);

    EXPECT_FALSE(oss.str().empty()) << "write_text should produce non-empty output";
}

TEST_F(IOTest, WriteText_ContainsCpuVendor)
{
    std::ostringstream oss;
    io::write_text(oss, mb_);

    std::string output = oss.str();

    // Output should contain the CPU vendor
    EXPECT_NE(output.find(mb_.cpu.vendor), std::string::npos)
        << "Text output should contain CPU vendor";
}

TEST_F(IOTest, WriteTextEx_ProducesOutput)
{
    std::ostringstream oss;
    io::write_text(oss, mb_ex_);

    EXPECT_FALSE(oss.str().empty()) << "write_text (extended) should produce non-empty output";
}

TEST_F(IOTest, WriteTextEx_ContainsCpuVendor)
{
    std::ostringstream oss;
    io::write_text(oss, mb_ex_);

    std::string output = oss.str();

    EXPECT_NE(output.find(mb_ex_.cpu.vendor), std::string::npos)
        << "Extended text output should contain CPU vendor";
}

TEST_F(IOTest, WriteTextEx_LongerThanBasic)
{
    std::ostringstream oss_basic, oss_ex;
    io::write_text(oss_basic, mb_);
    io::write_text(oss_ex, mb_ex_);

    // Extended output should be at least as long (likely longer with drives info)
    EXPECT_GE(oss_ex.str().size(), oss_basic.str().size())
        << "Extended text output should be at least as long as basic";
}

// ============================================================================
// write_binary() Tests
// ============================================================================

TEST_F(IOTest, WriteBinary_ProducesOutput)
{
    std::ostringstream oss(std::ios::binary);
    io::write_binary(oss, mb_);

    EXPECT_FALSE(oss.str().empty()) << "write_binary should produce non-empty output";
}

TEST_F(IOTest, WriteBinaryEx_ProducesOutput)
{
    std::ostringstream oss(std::ios::binary);
    io::write_binary(oss, mb_ex_);

    EXPECT_FALSE(oss.str().empty()) << "write_binary (extended) should produce non-empty output";
}

TEST_F(IOTest, WriteBinary_Deterministic)
{
    std::ostringstream oss1(std::ios::binary), oss2(std::ios::binary);
    io::write_binary(oss1, mb_);
    io::write_binary(oss2, mb_);

    EXPECT_EQ(oss1.str(), oss2.str())
        << "write_binary should be deterministic for same input";
}

TEST_F(IOTest, WriteBinaryEx_Deterministic)
{
    std::ostringstream oss1(std::ios::binary), oss2(std::ios::binary);
    io::write_binary(oss1, mb_ex_);
    io::write_binary(oss2, mb_ex_);

    EXPECT_EQ(oss1.str(), oss2.str())
        << "write_binary (extended) should be deterministic for same input";
}

// ============================================================================
// write_hash() Tests
// ============================================================================

TEST_F(IOTest, WriteHash_CorrectSize)
{
    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, mb_);

    // Default hash is Hash256 = 32 bytes
    EXPECT_EQ(oss.str().size(), 32u)
        << "write_hash should write exactly 32 bytes for Hash256";
}

TEST_F(IOTest, WriteHashEx_CorrectSize)
{
    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, mb_ex_);

    EXPECT_EQ(oss.str().size(), 32u)
        << "write_hash (extended) should write exactly 32 bytes for Hash256";
}

TEST_F(IOTest, WriteHash_MatchesDirectHash)
{
    // Compute hash directly
    auto direct_hash = hs::hash(mb_);

    // Write hash via stream
    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, mb_);

    std::string stream_data = oss.str();

    // Compare
    EXPECT_EQ(stream_data.size(), sizeof(direct_hash.buffer));
    EXPECT_EQ(std::memcmp(stream_data.data(), direct_hash.buffer, sizeof(direct_hash.buffer)), 0)
        << "Stream-written hash should match directly computed hash";
}

TEST_F(IOTest, WriteHashEx_MatchesDirectHash)
{
    auto direct_hash = hs::hash(mb_ex_);

    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, mb_ex_);

    std::string stream_data = oss.str();

    EXPECT_EQ(stream_data.size(), sizeof(direct_hash.buffer));
    EXPECT_EQ(std::memcmp(stream_data.data(), direct_hash.buffer, sizeof(direct_hash.buffer)), 0)
        << "Stream-written extended hash should match directly computed hash";
}

TEST_F(IOTest, WriteHash_PrecomputedHash)
{
    auto hash = hs::hash(mb_);

    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, hash);

    std::string stream_data = oss.str();

    EXPECT_EQ(stream_data.size(), sizeof(hash.buffer));
    EXPECT_EQ(std::memcmp(stream_data.data(), hash.buffer, sizeof(hash.buffer)), 0)
        << "write_hash with precomputed hash should write exact bytes";
}

TEST_F(IOTest, WriteHash_Deterministic)
{
    std::ostringstream oss1(std::ios::binary), oss2(std::ios::binary);
    io::write_hash(oss1, mb_);
    io::write_hash(oss2, mb_);

    EXPECT_EQ(oss1.str(), oss2.str())
        << "write_hash should be deterministic";
}

// ============================================================================
// Bad Stream Handling Tests
// ============================================================================

TEST_F(IOTest, WriteHash_BadStream_NoThrow)
{
    std::ostringstream oss;
    oss.setstate(std::ios::badbit);  // Set bad state

    EXPECT_NO_THROW({
        io::write_hash(oss, mb_);
    }) << "write_hash should not throw on bad stream";
}

TEST_F(IOTest, WriteHash_BadStream_NoWrite)
{
    std::ostringstream oss;
    oss.setstate(std::ios::badbit);

    io::write_hash(oss, mb_);

    // Clear bad state to check size
    oss.clear();
    EXPECT_TRUE(oss.str().empty())
        << "write_hash should not write to bad stream";
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(IOTest, WriteText_ConsistentAcrossSnaps)
{
    auto mb1 = snap_motherboard();
    auto mb2 = snap_motherboard();

    std::ostringstream oss1, oss2;
    io::write_text(oss1, mb1);
    io::write_text(oss2, mb2);

    EXPECT_EQ(oss1.str(), oss2.str())
        << "Text output should be consistent across multiple snaps";
}

TEST_F(IOTest, WriteBinary_ConsistentAcrossSnaps)
{
    auto mb1 = snap_motherboard();
    auto mb2 = snap_motherboard();

    std::ostringstream oss1(std::ios::binary), oss2(std::ios::binary);
    io::write_binary(oss1, mb1);
    io::write_binary(oss2, mb2);

    EXPECT_EQ(oss1.str(), oss2.str())
        << "Binary output should be consistent across multiple snaps";
}

// ============================================================================
// Different Hash Sizes Tests
// ============================================================================

TEST_F(IOTest, WriteHash_DifferentSizes)
{
    // Test writing different hash sizes directly
    hs::Hash128 h128{};
    hs::Hash256 h256{};
    hs::Hash512 h512{};

    // Initialize with non-zero data
    std::memset(h128.buffer, 0xAA, sizeof(h128.buffer));
    std::memset(h256.buffer, 0xBB, sizeof(h256.buffer));
    std::memset(h512.buffer, 0xCC, sizeof(h512.buffer));

    std::ostringstream oss128(std::ios::binary);
    std::ostringstream oss256(std::ios::binary);
    std::ostringstream oss512(std::ios::binary);

    io::write_hash(oss128, h128);
    io::write_hash(oss256, h256);
    io::write_hash(oss512, h512);

    EXPECT_EQ(oss128.str().size(), 16u) << "Hash128 should write 16 bytes";
    EXPECT_EQ(oss256.str().size(), 32u) << "Hash256 should write 32 bytes";
    EXPECT_EQ(oss512.str().size(), 64u) << "Hash512 should write 64 bytes";
}

} // namespace identy::test
