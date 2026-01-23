#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <set>

#include <Identy.h>
#include "test_config.hxx"

namespace identy::test
{

// ============================================================================
// Hash Type Tests
// ============================================================================

TEST(HashTypeTest, Hash128_SizeCorrect)
{
    EXPECT_EQ(sizeof(hs::Hash128::buffer), 16u)
        << "Hash128 buffer should be 16 bytes";
}

TEST(HashTypeTest, Hash256_SizeCorrect)
{
    EXPECT_EQ(sizeof(hs::Hash256::buffer), 32u)
        << "Hash256 buffer should be 32 bytes";
}

TEST(HashTypeTest, Hash512_SizeCorrect)
{
    EXPECT_EQ(sizeof(hs::Hash512::buffer), 64u)
        << "Hash512 buffer should be 64 bytes";
}

TEST(HashTypeTest, HashTemplate_EvenSizeRequired)
{
    // This is a compile-time check via static_assert in the library
    // We just verify our types work
    EXPECT_EQ(sizeof(hs::Hash<16>::buffer), 16u);
    EXPECT_EQ(sizeof(hs::Hash<32>::buffer), 32u);
    EXPECT_EQ(sizeof(hs::Hash<64>::buffer), 64u);
}

// ============================================================================
// Concept Tests
// ============================================================================

static_assert(hs::IdentyHashCompatible<hs::Hash128>,
    "Hash128 should satisfy IdentyHashCompatible concept");
static_assert(hs::IdentyHashCompatible<hs::Hash256>,
    "Hash256 should satisfy IdentyHashCompatible concept");
static_assert(hs::IdentyHashCompatible<hs::Hash512>,
    "Hash512 should satisfy IdentyHashCompatible concept");

static_assert(hs::IdentyHashFn<hs::detail::DefaultHash>,
    "DefaultHash should satisfy IdentyHashFn concept");
static_assert(hs::IdentyHashExFn<hs::detail::DefaultHashEx>,
    "DefaultHashEx should satisfy IdentyHashExFn concept");

// ============================================================================
// Hash Computation Tests
// ============================================================================

class HashComputationTest : public ::testing::Test
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

TEST_F(HashComputationTest, HashMotherboard_NotAllZero)
{
    auto hash = hs::hash(mb_);

    bool all_zero = std::all_of(
        std::begin(hash.buffer),
        std::end(hash.buffer),
        [](byte b) { return b == 0; }
    );

    EXPECT_FALSE(all_zero) << "Hash should not be all zeros";
}

TEST_F(HashComputationTest, HashMotherboardEx_NotAllZero)
{
    auto hash = hs::hash(mb_ex_);

    bool all_zero = std::all_of(
        std::begin(hash.buffer),
        std::end(hash.buffer),
        [](byte b) { return b == 0; }
    );

    EXPECT_FALSE(all_zero) << "Extended hash should not be all zeros";
}

TEST_F(HashComputationTest, HashMotherboard_Deterministic)
{
    auto hash1 = hs::hash(mb_);
    auto hash2 = hs::hash(mb_);

    EXPECT_EQ(std::memcmp(hash1.buffer, hash2.buffer, sizeof(hash1.buffer)), 0)
        << "Hash should be deterministic for same input";
}

TEST_F(HashComputationTest, HashMotherboardEx_Deterministic)
{
    auto hash1 = hs::hash(mb_ex_);
    auto hash2 = hs::hash(mb_ex_);

    EXPECT_EQ(std::memcmp(hash1.buffer, hash2.buffer, sizeof(hash1.buffer)), 0)
        << "Extended hash should be deterministic for same input";
}

TEST_F(HashComputationTest, HashMotherboard_ConsistentAcrossSnaps)
{
    auto mb1 = snap_motherboard();
    auto mb2 = snap_motherboard();

    auto hash1 = hs::hash(mb1);
    auto hash2 = hs::hash(mb2);

    EXPECT_EQ(std::memcmp(hash1.buffer, hash2.buffer, sizeof(hash1.buffer)), 0)
        << "Hash should be consistent across multiple snap_motherboard() calls";
}

TEST_F(HashComputationTest, HashMotherboardEx_ConsistentAcrossSnaps)
{
    auto mb1 = snap_motherboard_ex();
    auto mb2 = snap_motherboard_ex();

    auto hash1 = hs::hash(mb1);
    auto hash2 = hs::hash(mb2);

    EXPECT_EQ(std::memcmp(hash1.buffer, hash2.buffer, sizeof(hash1.buffer)), 0)
        << "Extended hash should be consistent across multiple snap_motherboard_ex() calls";
}

// ============================================================================
// Hash Compare Tests
// ============================================================================

TEST_F(HashComputationTest, Compare_EqualHashes)
{
    auto hash = hs::hash(mb_);

    EXPECT_EQ(hs::compare(hash, hash), 0)
        << "Comparing hash with itself should return 0";
}

TEST_F(HashComputationTest, Compare_CopiedHashes)
{
    auto hash1 = hs::hash(mb_);
    auto hash2 = hash1;  // Copy

    EXPECT_EQ(hs::compare(hash1, hash2), 0)
        << "Comparing copied hashes should return 0";
}

TEST_F(HashComputationTest, Compare_DifferentHashes)
{
    auto hash_basic = hs::hash(mb_);

    // Create a modified hash
    auto hash_modified = hash_basic;
    hash_modified.buffer[0] ^= 0xFF;  // Flip bits

    EXPECT_NE(hs::compare(hash_basic, hash_modified), 0)
        << "Comparing different hashes should return non-zero";
}

// ============================================================================
// DefaultHash Functor Tests
// ============================================================================

TEST_F(HashComputationTest, DefaultHash_ProducesHash256)
{
    hs::detail::DefaultHash hasher;
    auto hash = hasher(mb_);

    // Type check
    static_assert(std::is_same_v<decltype(hash), hs::Hash256>,
        "DefaultHash should produce Hash256");

    EXPECT_EQ(sizeof(hash.buffer), 32u);
}

TEST_F(HashComputationTest, DefaultHashEx_ProducesHash256)
{
    hs::detail::DefaultHashEx hasher;
    auto hash = hasher(mb_ex_);

    // Type check
    static_assert(std::is_same_v<decltype(hash), hs::Hash256>,
        "DefaultHashEx should produce Hash256");

    EXPECT_EQ(sizeof(hash.buffer), 32u);
}

// ============================================================================
// Hash Entropy Tests
// ============================================================================

TEST_F(HashComputationTest, Hash_HasReasonableEntropy)
{
    auto hash = hs::hash(mb_);

    // Count unique bytes
    std::set<byte> unique_bytes(std::begin(hash.buffer), std::end(hash.buffer));

    // A good hash should have many unique byte values
    // (with 32 bytes, we'd expect quite a few unique values)
    EXPECT_GT(unique_bytes.size(), 10u)
        << "Hash should have reasonable entropy (many unique bytes)";
}

// ============================================================================
// Basic vs Extended Hash Relationship
// ============================================================================

TEST_F(HashComputationTest, BasicVsExtended_MayDiffer)
{
    auto hash_basic = hs::hash(mb_);
    auto hash_ex = hs::hash(mb_ex_);

    // They may or may not be equal depending on whether drives affect the hash
    // This test just verifies both can be computed
    bool are_equal = (std::memcmp(hash_basic.buffer, hash_ex.buffer,
                                   sizeof(hash_basic.buffer)) == 0);

    if (mb_ex_.drives.empty()) {
        // If no drives, hashes might be equal
        GTEST_LOG_(INFO) << "No drives - basic and extended hashes "
                         << (are_equal ? "are equal" : "differ");
    } else {
        // With drives, extended hash should likely be different
        // (but this depends on implementation - not a strict requirement)
        GTEST_LOG_(INFO) << "With " << mb_ex_.drives.size() << " drives - "
                         << "basic and extended hashes "
                         << (are_equal ? "are equal" : "differ");
    }
}

} // namespace identy::test
