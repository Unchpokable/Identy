#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <vector>
#include <future>
#include <cstring>

#include <Identy.h>
#include "test_config.hxx"

namespace identy::test
{

// ============================================================================
// Full Pipeline Tests
// ============================================================================

TEST(IntegrationTest, FullPipeline_SnapHashCompare)
{
    // Test the typical use case: snap -> hash -> store/compare
    auto mb1 = snap_motherboard();
    auto hash1 = hs::hash(mb1);

    // Simulate storing hash (to string)
    std::ostringstream oss(std::ios::binary);
    io::write_hash(oss, hash1);
    std::string stored_hash = oss.str();

    // Later: snap again and compare
    auto mb2 = snap_motherboard();
    auto hash2 = hs::hash(mb2);

    // Compare with stored
    EXPECT_EQ(stored_hash.size(), sizeof(hash2.buffer));
    EXPECT_EQ(std::memcmp(stored_hash.data(), hash2.buffer, sizeof(hash2.buffer)), 0)
        << "Stored hash should match re-computed hash";

    // Also test direct compare
    EXPECT_EQ(hs::compare(hash1, hash2), 0)
        << "Directly compared hashes should be equal";
}

TEST(IntegrationTest, FullPipeline_SnapAnalyzeReport)
{
    // Test: snap -> analyze_full -> write_text (debugging/logging workflow)
    auto mb = snap_motherboard_ex();
    auto verdict = vm::analyze_full(mb);

    // Write report
    std::ostringstream report;
    io::write_text(report, mb);

    // Report should be non-empty
    EXPECT_FALSE(report.str().empty());

    // Log verdict details
    std::ostringstream verdict_log;
    verdict_log << "VM Detection Result:\n";
    verdict_log << "  Confidence: " << static_cast<int>(verdict.confidence) << "\n";
    verdict_log << "  Is Virtual: " << (verdict.is_virtual() ? "Yes" : "No") << "\n";
    verdict_log << "  Detection Flags: " << verdict.detections.size() << "\n";

    // This is informational - no strict assertions
    GTEST_LOG_(INFO) << verdict_log.str();
}

TEST(IntegrationTest, FullPipeline_ExtendedWorkflow)
{
    // Extended workflow: snap_ex -> hash -> analyze -> output
    auto mb = snap_motherboard_ex();

    // Compute fingerprint
    auto fingerprint = hs::hash(mb);

    // Analyze for VM
    auto verdict = vm::analyze_full(mb);

    // Write all outputs
    std::ostringstream text_out, binary_out, hash_out;
    io::write_text(text_out, mb);
    io::write_binary(binary_out, mb);
    io::write_hash(hash_out, mb);

    // All outputs should be non-empty
    EXPECT_FALSE(text_out.str().empty());
    EXPECT_FALSE(binary_out.str().empty());
    EXPECT_EQ(hash_out.str().size(), sizeof(fingerprint.buffer));

    // Fingerprint should match hash output
    EXPECT_EQ(std::memcmp(hash_out.str().data(), fingerprint.buffer,
                          sizeof(fingerprint.buffer)), 0);
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST(IntegrationTest, ConsistencyAcrossCalls_BasicMotherboard)
{
    constexpr int kIterations = 10;

    auto reference_mb = snap_motherboard();
    auto reference_hash = hs::hash(reference_mb);

    for (int i = 0; i < kIterations; ++i) {
        auto mb = snap_motherboard();
        auto hash = hs::hash(mb);

        EXPECT_EQ(mb.cpu.vendor, reference_mb.cpu.vendor)
            << "Iteration " << i << ": CPU vendor changed";
        EXPECT_EQ(mb.cpu.version, reference_mb.cpu.version)
            << "Iteration " << i << ": CPU version changed";
        EXPECT_EQ(hs::compare(hash, reference_hash), 0)
            << "Iteration " << i << ": Hash changed";
    }
}

TEST(IntegrationTest, ConsistencyAcrossCalls_ExtendedMotherboard)
{
    constexpr int kIterations = 5;

    auto reference_mb = snap_motherboard_ex();
    auto reference_hash = hs::hash(reference_mb);

    for (int i = 0; i < kIterations; ++i) {
        auto mb = snap_motherboard_ex();
        auto hash = hs::hash(mb);

        EXPECT_EQ(mb.cpu.vendor, reference_mb.cpu.vendor);
        EXPECT_EQ(mb.drives.size(), reference_mb.drives.size());
        EXPECT_EQ(hs::compare(hash, reference_hash), 0)
            << "Iteration " << i << ": Extended hash changed";
    }
}

TEST(IntegrationTest, ConsistencyAcrossCalls_VMDetection)
{
    constexpr int kIterations = 5;

    auto reference_mb = snap_motherboard();
    auto reference_verdict = vm::analyze_full(reference_mb);

    for (int i = 0; i < kIterations; ++i) {
        auto mb = snap_motherboard();
        auto verdict = vm::analyze_full(mb);

        EXPECT_EQ(verdict.is_virtual(), reference_verdict.is_virtual())
            << "Iteration " << i << ": VM detection result changed";
        EXPECT_EQ(verdict.confidence, reference_verdict.confidence)
            << "Iteration " << i << ": VM confidence changed";
    }
}

// ============================================================================
// Thread Safety Tests (Basic)
// ============================================================================

TEST(IntegrationTest, ThreadSafety_ConcurrentSnaps)
{
    constexpr int kThreads = 4;
    constexpr int kIterationsPerThread = 10;

    std::vector<std::future<bool>> futures;

    // Get reference values
    auto reference_mb = snap_motherboard();
    auto reference_hash = hs::hash(reference_mb);

    auto thread_func = [&reference_mb, &reference_hash]() {
        for (int i = 0; i < kIterationsPerThread; ++i) {
            auto mb = snap_motherboard();
            auto hash = hs::hash(mb);

            if (mb.cpu.vendor != reference_mb.cpu.vendor) return false;
            if (hs::compare(hash, reference_hash) != 0) return false;
        }
        return true;
    };

    // Launch threads
    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, thread_func));
    }

    // Wait for all and check results
    for (auto& f : futures) {
        EXPECT_TRUE(f.get()) << "Thread reported inconsistent results";
    }
}

TEST(IntegrationTest, ThreadSafety_ConcurrentAnalyze)
{
    constexpr int kThreads = 4;

    auto reference_mb = snap_motherboard();
    auto reference_verdict = vm::analyze_full(reference_mb);

    std::vector<std::future<bool>> futures;

    auto thread_func = [&reference_mb, &reference_verdict]() {
        for (int i = 0; i < 10; ++i) {
            auto verdict = vm::analyze_full(reference_mb);
            if (verdict.confidence != reference_verdict.confidence) return false;
            if (verdict.is_virtual() != reference_verdict.is_virtual()) return false;
        }
        return true;
    };

    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, thread_func));
    }

    for (auto& f : futures) {
        EXPECT_TRUE(f.get()) << "Thread reported inconsistent VM analysis";
    }
}

// ============================================================================
// API Completeness Tests
// ============================================================================

TEST(IntegrationTest, APICompleteness_AllFunctionsCallable)
{
    // Verify all main API functions are callable and don't crash

    // HWID functions
    EXPECT_NO_THROW({
        auto mb = snap_motherboard();
        (void)mb;
    });

    EXPECT_NO_THROW({
        auto mb_ex = snap_motherboard_ex();
        (void)mb_ex;
    });

    EXPECT_NO_THROW({
        auto drives = list_drives();
        (void)drives;
    });

    // Hash functions
    auto mb = snap_motherboard();
    auto mb_ex = snap_motherboard_ex();

    EXPECT_NO_THROW({
        auto h = hs::hash(mb);
        (void)h;
    });

    EXPECT_NO_THROW({
        auto h = hs::hash(mb_ex);
        (void)h;
    });

    // VM detection functions
    EXPECT_NO_THROW({
        bool v = vm::assume_virtual(mb);
        (void)v;
    });

    EXPECT_NO_THROW({
        bool v = vm::assume_virtual(mb_ex);
        (void)v;
    });

    EXPECT_NO_THROW({
        auto verdict = vm::analyze_full(mb);
        (void)verdict;
    });

    EXPECT_NO_THROW({
        auto verdict = vm::analyze_full(mb_ex);
        (void)verdict;
    });

    // I/O functions
    std::ostringstream oss;
    EXPECT_NO_THROW({ io::write_text(oss, mb); });
    EXPECT_NO_THROW({ io::write_text(oss, mb_ex); });
    EXPECT_NO_THROW({ io::write_binary(oss, mb); });
    EXPECT_NO_THROW({ io::write_binary(oss, mb_ex); });
    EXPECT_NO_THROW({ io::write_hash(oss, mb); });
    EXPECT_NO_THROW({ io::write_hash(oss, mb_ex); });
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(IntegrationTest, EdgeCase_RapidSuccessiveCalls)
{
    // Make many rapid calls to ensure no resource leaks or crashes
    constexpr int kCalls = 100;

    for (int i = 0; i < kCalls; ++i) {
        auto mb = snap_motherboard();
        auto hash = hs::hash(mb);
        auto verdict = vm::analyze_full(mb);
        (void)hash;
        (void)verdict;
    }

    // If we get here without crash or exception, test passes
    SUCCEED();
}

TEST(IntegrationTest, EdgeCase_MixedBasicAndExtended)
{
    // Interleave basic and extended calls
    for (int i = 0; i < 20; ++i) {
        if (i % 2 == 0) {
            auto mb = snap_motherboard();
            auto h = hs::hash(mb);
            auto v = vm::analyze_full(mb);
            (void)h;
            (void)v;
        } else {
            auto mb = snap_motherboard_ex();
            auto h = hs::hash(mb);
            auto v = vm::analyze_full(mb);
            (void)h;
            (void)v;
        }
    }

    SUCCEED();
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST(IntegrationTest, DataIntegrity_HashUniqueness)
{
    // Hash should differ if we modify the data
    auto mb = snap_motherboard();
    auto original_hash = hs::hash(mb);

    // Create a copy and modify it
    auto modified_mb = mb;
    modified_mb.cpu.vendor = "MODIFIED_VENDOR_FOR_TEST";

    auto modified_hash = hs::hash(modified_mb);

    // Hashes should differ
    EXPECT_NE(hs::compare(original_hash, modified_hash), 0)
        << "Hash should change when data is modified";
}

TEST(IntegrationTest, DataIntegrity_SmbiosUuidUsed)
{
    // Verify SMBIOS UUID is part of the hash
    auto mb = snap_motherboard();
    auto original_hash = hs::hash(mb);

    // Modify UUID
    auto modified_mb = mb;
    modified_mb.smbios.uuid[0] ^= 0xFF;

    auto modified_hash = hs::hash(modified_mb);

    EXPECT_NE(hs::compare(original_hash, modified_hash), 0)
        << "Hash should change when UUID is modified";
}

} // namespace identy::test
