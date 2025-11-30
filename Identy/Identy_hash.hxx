#pragma once

#ifndef UNC_IDENTY_HASH_H
#define UNC_IDENTY_HASH_H

#include "Identy_hash_base.hxx"
#include "Identy_hwid.hxx"

namespace identy::hs
{

} // namespace identy::hs

namespace identy::hs::detail
{
/**
 * @brief Computes default SHA-256 hash for basic motherboard information
 *
 * Internal implementation function that generates a cryptographic hash from
 * motherboard CPU and SMBIOS data. This hash provides a stable hardware
 * fingerprint for device identification purposes.
 *
 * @param board Motherboard structure containing CPU and SMBIOS information
 * @return Hash256 containing the computed 256-bit hash value
 *
 * @note This is an internal implementation detail. Use identy::hs::hash()
 *       template function for public API access
 *
 * @see default_hash_ex()
 * @see identy::hs::hash()
 */
Hash256 default_hash(const Motherboard& board);

/**
 * @brief Computes default SHA-256 hash for extended motherboard information
 *
 * Internal implementation function that generates a cryptographic hash from
 * extended motherboard data including CPU, SMBIOS, and physical drive information.
 * This provides a more comprehensive hardware fingerprint than default_hash().
 *
 * @param board MotherboardEx structure containing CPU, SMBIOS, and drive information
 * @return Hash256 containing the computed 256-bit hash value
 *
 * @warning The drives vector in MotherboardEx must be sorted by serial numbers
 *          or maintain stable ordering to ensure consistent hash generation
 *          across multiple invocations
 *
 * @note This is an internal implementation detail. Use identy::hs::hash()
 *       template function for public API access
 *
 * @see default_hash()
 * @see identy::hs::hash()
 */
Hash256 default_hash_ex(const MotherboardEx& board);
} // namespace identy::hs::detail

namespace identy::hs::detail
{
/**
 * @brief Hash function interface template
 *
 * Base interface template that defines the hash result type for hash function
 * implementations. All hash function types must inherit from this interface
 * and define their output hash type.
 *
 * @tparam HashObj The hash type (Hash128, Hash256, or Hash512) this function produces
 *
 * @note This is a policy-based design pattern allowing compile-time selection
 *       of hash algorithms and output sizes
 *
 * @see DefaultHash, DefaultHashEx
 */
template<typename HashObj>
struct IHash
{
    /** @brief Type alias for the hash result type */
    using Type = HashObj;
};

/**
 * @brief Default hash function for basic motherboard information
 *
 * Functor implementation that computes SHA-256 hashes from Motherboard structures.
 * This is the default hash algorithm used when calling identy::hs::hash() with
 * basic motherboard data.
 *
 * @note This is a stateless functor with trivial construction/destruction,
 *       optimized for compile-time instantiation
 *
 * @see DefaultHashEx
 * @see identy::hs::hash()
 */
struct DefaultHash final : public IHash<Hash256>
{
    /**
     * @brief Hash computation operator
     *
     * Computes the SHA-256 hash of motherboard hardware information.
     *
     * @param board Motherboard structure to hash
     * @return Hash256 containing the computed hash value
     */
    Type operator()(const Motherboard& board) const
    {
        return default_hash(board);
    }
};

/**
 * @brief Default hash function for extended motherboard information
 *
 * Functor implementation that computes SHA-256 hashes from MotherboardEx structures
 * including physical drive data. This is the default hash algorithm used when
 * calling identy::hs::hash() with extended motherboard data.
 *
 * @warning Input MotherboardEx structure must have drives sorted in a stable order
 *          (typically by serial number) to ensure consistent hash generation
 *
 * @note This is a stateless functor with trivial construction/destruction,
 *       optimized for compile-time instantiation
 *
 * @see DefaultHash
 * @see identy::hs::hash()
 */
struct DefaultHashEx final : public IHash<Hash256>
{
    /**
     * @brief Hash computation operator
     *
     * Computes the SHA-256 hash of extended motherboard hardware information
     * including storage drives.
     *
     * @param board MotherboardEx structure to hash (drives must be pre-sorted)
     * @return Hash256 containing the computed hash value
     */
    Type operator()(const MotherboardEx& board) const
    {
        return default_hash_ex(board);
    }
};
} // namespace identy::hs::detail

namespace identy::hs
{
/**
 * @brief C++20 concept defining valid hash functions for Motherboard structures
 *
 * Constrains template parameters to types that can be used as hash functions
 * for basic motherboard information. A valid hash function must:
 * - Define a nested Type alias for the hash result type
 * - Be invocable with a const Motherboard& parameter and return Type
 * - Have trivial construction and destruction semantics
 *
 * @tparam T Type to validate as a hash function
 *
 * @note The trivial construction/destruction requirements enable zero-overhead
 *       instantiation of hash functors
 *
 * @see IdentyHashExFn
 * @see DefaultHash
 */
template<typename T>
concept IdentyHashFn = requires { typename T::Type; } && std::is_invocable_r_v<typename T::Type, T, const Motherboard&>
    && std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>;

/**
 * @brief C++20 concept defining valid hash functions for MotherboardEx structures
 *
 * Constrains template parameters to types that can be used as hash functions
 * for extended motherboard information including drive data. A valid hash
 * function must:
 * - Define a nested Type alias for the hash result type
 * - Be invocable with a const MotherboardEx& parameter and return Type
 * - Have trivial construction and destruction semantics
 *
 * @tparam T Type to validate as a hash function
 *
 * @note The trivial construction/destruction requirements enable zero-overhead
 *       instantiation of hash functors
 *
 * @see IdentyHashFn
 * @see DefaultHashEx
 */
template<typename T>
concept IdentyHashExFn = requires { typename T::Type; } && std::is_invocable_r_v<typename T::Type, T, const MotherboardEx&>
    && std::is_trivially_constructible_v<T> && std::is_trivially_destructible_v<T>;
} // namespace identy::hs

namespace identy::hs
{
/**
 * @brief Computes cryptographic hash of basic motherboard information
 *
 * Template function that generates a hardware fingerprint from motherboard
 * CPU and SMBIOS data using a customizable hash algorithm. By default, uses
 * SHA-256 to produce a 256-bit hash suitable for device identification.
 *
 * This function provides a stable, deterministic identifier derived from
 * hardware characteristics. The hash remains constant as long as the physical
 * hardware components (CPU, motherboard firmware) remain unchanged.
 *
 * @tparam Hash Hash function type satisfying IdentyHashFn concept
 *              (default: DefaultHash producing Hash256/SHA-256)
 *
 * @param mb Motherboard structure containing CPU and SMBIOS information
 * @return Hash value of type Hash::Type (typically Hash256)
 *
 * @note This function does not include physical drive information in the hash.
 *       For extended hardware fingerprinting including storage devices, use
 *       the MotherboardEx overload
 *
 * @note The hash is deterministic and will produce identical output for
 *       identical hardware configurations
 *
 * @see hash(const identy::MotherboardEx&)
 * @see DefaultHash
 * @see Hash256
 *
 * Example usage:
 * @code
 * auto mb = identy::snap_motherboard();
 * auto fingerprint = identy::hs::hash(mb);
 * // fingerprint.buffer now contains 32-byte SHA-256 hash
 * @endcode
 */
template<IdentyHashFn Hash = detail::DefaultHash>
auto hash(const Motherboard& mb) -> Hash::Type;

/**
 * @brief Computes cryptographic hash of extended motherboard information
 *
 * Template function that generates a comprehensive hardware fingerprint from
 * motherboard CPU, SMBIOS, and physical drive data using a customizable hash
 * algorithm. By default, uses SHA-256 to produce a 256-bit hash suitable for
 * device identification.
 *
 * This function provides a more comprehensive hardware identifier than the
 * basic Motherboard variant by including storage device serial numbers in
 * the hash computation. This creates a stronger binding to the physical machine.
 *
 * @tparam Hash Hash function type satisfying IdentyHashExFn concept
 *              (default: DefaultHashEx producing Hash256/SHA-256)
 *
 * @param mb MotherboardEx structure containing CPU, SMBIOS, and drive information
 * @return Hash value of type Hash::Type (typically Hash256)
 *
 * @warning The drives vector in the input MotherboardEx structure MUST be sorted
 *          by serial numbers (or maintain any other stable ordering) before calling
 *          this function. Unsorted drive lists will produce inconsistent hashes
 *          across multiple invocations even on the same hardware.
 *
 * @note This hash includes physical drive information and will change if storage
 *       devices are added, removed, or replaced
 *
 * @note The hash is deterministic and will produce identical output for
 *       identical hardware configurations with stable drive ordering
 *
 * @see hash(const identy::Motherboard&)
 * @see DefaultHashEx
 * @see Hash256
 *
 * Example usage:
 * @code
 * auto mb = identy::snap_motherboard_ex();
 * // Sort drives to ensure stable hashing
 * std::sort(mb.drives.begin(), mb.drives.end(),
 *           [](auto& a, auto& b) { return a.serial < b.serial; });
 * auto fingerprint = identy::hs::hash(mb);
 * // fingerprint.buffer now contains 32-byte SHA-256 hash
 * @endcode
 */
template<IdentyHashExFn Hash = detail::DefaultHashEx>
auto hash(const MotherboardEx& mb) -> Hash::Type;
} // namespace identy::hs

namespace identy::hs
{
template<IdentyHashCompatible Hash>
int compare(Hash&& lhs, Hash&& rhs);
} // namespace identy::hs

template<identy::hs::IdentyHashFn Hash>
auto identy::hs::hash(const Motherboard& mb) -> Hash::Type
{
    return Hash {}(mb);
}

template<identy::hs::IdentyHashExFn Hash>
auto identy::hs::hash(const MotherboardEx& mb) -> Hash::Type
{
    return Hash {}(mb);
}

template<identy::hs::IdentyHashCompatible Hash>
int identy::hs::compare(Hash&& lhs, Hash&& rhs)
{
    return std::memcmp(lhs.buffer, rhs.buffer, sizeof(lhs.buffer));
}

#endif
