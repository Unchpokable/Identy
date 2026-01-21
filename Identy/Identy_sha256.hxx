/**
 * @file Identy_sha256.hxx
 * @brief Internal SHA-256 cryptographic hash implementation
 *
 * Provides a self-contained implementation of the SHA-256 cryptographic hash
 * algorithm as defined in FIPS 180-4. This implementation is used internally
 * by Identy for generating hardware fingerprints.
 *
 * ## Implementation Details
 *
 * - **Algorithm:** SHA-256 (Secure Hash Algorithm 2, 256-bit variant)
 * - **Block Size:** 512 bits (64 bytes)
 * - **Digest Size:** 256 bits (32 bytes)
 * - **Compliance:** FIPS 180-4 specification
 * - **Performance:** Pure C++ implementation without hardware acceleration
 *
 * @note This is an internal implementation detail. Application code should use
 *       the high-level hashing functions from identy::hs namespace instead.
 *
 * @see identy::hs::hash()
 * @see Hash256
 */

#pragma once

#ifndef UNC_IDENTY_SHA256_H
#define UNC_IDENTY_SHA256_H

#include <cstddef>
#include <cstdint>
#include <span>

#include "Identy_hash_base.hxx"
#include "Identy_types.hxx"

namespace identy::hs::detail
{
/**
 * @brief Internal SHA-256 hash implementation class
 *
 * Implements the SHA-256 cryptographic hash algorithm with support for both
 * one-shot hashing (static hash() method) and incremental hashing (update()
 * method for streaming data).
 *
 * ## Usage Patterns
 *
 * **One-shot hashing:**
 * ```cpp
 * std::span<const byte> data = ...;
 * Hash256 result = Sha256::hash(data);
 * ```
 *
 * **Incremental hashing:**
 * ```cpp
 * Sha256 hasher;
 * hasher.update(chunk1);
 * hasher.update(chunk2);
 * Hash256 result = hasher.finalize();
 * ```
 *
 * @note This class is for internal library use. Public API should use
 *       identy::hs::hash() template functions instead.
 *
 * @warning After calling finalize(), the hasher object is in finalized state
 *          and must be reset() before reuse
 */
class Sha256 final
{
public:
    /** @brief SHA-256 block size in bytes (512 bits) */
    static constexpr std::size_t block_size = 64;

    /** @brief SHA-256 digest size in bytes (256 bits) */
    static constexpr std::size_t digest_size = 32;

    /**
     * @brief One-shot SHA-256 hash computation
     *
     * Computes the complete SHA-256 hash of the input data in a single
     * operation. Convenient for hashing data that fits entirely in memory.
     *
     * @param data Span of bytes to hash
     * @return Hash256 structure containing the computed 256-bit hash
     *
     * @note This is a convenience wrapper around the incremental API
     */
    static Hash256 hash(std::span<const byte> data) noexcept;

    /**
     * @brief Default constructor initializing hasher to initial state
     *
     * Prepares the hasher for accepting data via update() calls.
     */
    Sha256() noexcept;

    /**
     * @brief Updates hash state with additional data (span overload)
     *
     * Incrementally processes input data, updating the internal hash state.
     * Can be called multiple times to hash data in chunks.
     *
     * @param data Span of bytes to add to the hash
     *
     * @note Can be called multiple times before finalize()
     * @note Data is processed in 512-bit blocks internally
     */
    void update(std::span<const byte> data) noexcept;

    /**
     * @brief Updates hash state with additional data (pointer overload)
     *
     * Incrementally processes input data, updating the internal hash state.
     * Can be called multiple times to hash data in chunks.
     *
     * @param data Pointer to byte array to add to the hash
     * @param len Length of the data in bytes
     *
     * @note Can be called multiple times before finalize()
     * @note Data is processed in 512-bit blocks internally
     */
    void update(const byte* data, std::size_t len) noexcept;

    /**
     * @brief Finalizes hash computation and returns result
     *
     * Completes the SHA-256 computation by applying padding and final
     * transformations, then returns the computed hash value.
     *
     * @return Hash256 structure containing the computed 256-bit hash
     *
     * @warning After calling finalize(), the hasher enters finalized state
     *          and cannot accept more data until reset() is called
     *
     * @note The result is available immediately and the hasher can be reset
     *       for reuse with reset()
     */
    [[nodiscard]] Hash256 finalize() noexcept;

    /**
     * @brief Resets hasher to initial state for reuse
     *
     * Clears all internal state and returns the hasher to initial condition,
     * allowing it to be reused for computing a new hash.
     */
    void reset() noexcept;

private:
    static constexpr std::uint32_t k_round_constants[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1,
        0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e,
        0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa,
        0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

    static constexpr std::uint32_t k_initial_hash[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
        0x5be0cd19 };

    std::uint32_t m_state[8];
    byte m_block[block_size];
    std::size_t m_block_len;
    std::uint64_t m_total_len;
    bool m_finalized;

    void transform(const byte* block) noexcept;

    static constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) noexcept
    {
        return (x >> n) | (x << (32 - n));
    }

    static constexpr std::uint32_t choose(std::uint32_t e, std::uint32_t f, std::uint32_t g) noexcept
    {
        return (e & f) ^ (~e & g);
    }

    static constexpr std::uint32_t majority(std::uint32_t a, std::uint32_t b, std::uint32_t c) noexcept
    {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    static constexpr std::uint32_t sigma0(std::uint32_t x) noexcept
    {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    static constexpr std::uint32_t sigma1(std::uint32_t x) noexcept
    {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    static constexpr std::uint32_t gamma0(std::uint32_t x) noexcept
    {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    static constexpr std::uint32_t gamma1(std::uint32_t x) noexcept
    {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
};
} // namespace identy::hs::detail

#endif