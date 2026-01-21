#pragma once

#ifndef UNC_IDENTY_HASH_BASE_H
#define UNC_IDENTY_HASH_BASE_H

#include <cstddef>

#include "Identy_types.hxx"

namespace identy::hs
{
/**
 * @brief Generic hash buffer template structure
 *
 * Fixed-size buffer for storing cryptographic hash values of arbitrary length.
 * This template provides a type-safe wrapper around raw byte arrays for hash
 * storage, ensuring compile-time size guarantees.
 *
 * @tparam BuffSize Size of the hash buffer in bytes
 *
 * @note This is a POD (Plain Old Data) type that can be safely copied and
 *       moved using memcpy operations
 *
 * @see Hash128, Hash256, Hash512
 */
template<std::size_t BuffSize>
struct Hash
{
    static_assert(BuffSize % 2 == 0, "Buffer size should be a power of 2!");

    /** @brief Fixed-size byte array containing the hash value */
    byte buffer[BuffSize];
};

/**
 * @brief 128-bit (16-byte) hash type alias
 *
 * Commonly used for MD5 hashes or truncated SHA variants.
 * Provides 2^128 possible unique values.
 */
using Hash128 = Hash<16>;

/**
 * @brief 256-bit (32-byte) hash type alias
 *
 * Standard size for SHA-256 cryptographic hashes and similar algorithms.
 * Provides 2^256 possible unique values, offering strong collision resistance.
 * This is the default hash type used by the library.
 */
using Hash256 = Hash<32>;

/**
 * @brief 512-bit (64-byte) hash type alias
 *
 * Used for SHA-512 and other extended-length cryptographic hashes.
 * Provides 2^512 possible unique values for maximum collision resistance.
 */
using Hash512 = Hash<64>;

/**
 * @brief Template concept requiring `buffer` field
 */
template<typename Hash>
concept IdentyHashCompatible = requires(Hash hash) { hash.buffer; };
} // namespace identy::hs

#endif