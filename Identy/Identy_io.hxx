/**
 * @file Identy_io.hxx
 * @brief I/O serialization utilities for hardware information and hash data
 *
 * Provides functions for writing motherboard hardware data and cryptographic
 * hashes to output streams in various formats (text, binary, raw hash bytes).
 * These utilities are useful for logging, debugging, persistence, and network
 * transmission of hardware fingerprints.
 *
 * ## Supported Output Formats
 *
 * - **Text Format** - Human-readable structured output with labeled fields
 * - **Binary Format** - Compact binary serialization for efficient storage
 * - **Raw Hash** - Direct byte-level hash output for transmission/comparison
 *
 * @note All functions operate on std::ostream, supporting files, stringstreams,
 *       network sockets, or any other stream-compatible output target.
 */

#pragma once

#ifndef UNC_IDENTY_IO_H
#define UNC_IDENTY_IO_H

#include <ostream>

#include "Identy_hash.hxx"

namespace identy
{
struct Motherboard;
struct MotherboardEx;
} // namespace identy

namespace identy::io
{
/**
 * @brief Writes basic motherboard information to stream in human-readable text format
 *
 * Serializes CPU and SMBIOS data to the output stream with labeled fields and
 * formatted values suitable for debugging or logging. Output includes CPU vendor,
 * model, instruction set features, SMBIOS version, and system UUID.
 *
 * @param stream Output stream to write to (must be in good state)
 * @param mb Motherboard structure containing hardware data
 *
 * @note Output is designed for human readability, not for parsing or deserialization
 * @see write_binary() for compact machine-readable format
 */
void write_text(std::ostream& stream, const Motherboard& mb);

/**
 * @brief Writes extended motherboard information to stream in human-readable text format
 *
 * Serializes CPU, SMBIOS, and physical drive data to the output stream with labeled
 * fields and formatted values. Includes all information from the basic Motherboard
 * variant plus detailed storage device enumeration.
 *
 * @param stream Output stream to write to (must be in good state)
 * @param mb MotherboardEx structure containing hardware and drive data
 *
 * @note Output is designed for human readability, not for parsing or deserialization
 * @see write_binary() for compact machine-readable format
 */
void write_text(std::ostream& stream, const MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
/**
 * @brief Writes basic motherboard information in compact binary format
 *
 * Serializes CPU and SMBIOS data to the output stream as a compact binary
 * representation suitable for efficient storage or network transmission.
 *
 * @param stream Output stream to write to (must be in good state and binary mode)
 * @param mb Motherboard structure containing hardware data
 *
 * @warning Stream must be opened in binary mode (std::ios::binary) to prevent
 *          line-ending translation corrupting the data
 *
 * @note No deserialization function is currently provided. Use for archival
 *       or transmission where you control both ends of the data flow.
 */
void write_binary(std::ostream& stream, const Motherboard& mb);

/**
 * @brief Writes extended motherboard information in compact binary format
 *
 * Serializes CPU, SMBIOS, and drive data to the output stream as a compact binary
 * representation suitable for efficient storage or network transmission.
 *
 * @param stream Output stream to write to (must be in good state and binary mode)
 * @param mb MotherboardEx structure containing hardware and drive data
 *
 * @warning Stream must be opened in binary mode (std::ios::binary) to prevent
 *          line-ending translation corrupting the data
 *
 * @note No deserialization function is currently provided. Use for archival
 *       or transmission where you control both ends of the data flow.
 */
void write_binary(std::ostream& stream, const MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
/**
 * @brief Computes and writes hash of basic motherboard information to stream
 *
 * Template function that computes a cryptographic hash of the motherboard data
 * using the specified hash algorithm, then writes the raw hash bytes to the
 * output stream. This is a convenience wrapper combining hashing and output.
 *
 * @tparam Hash Hash function type satisfying IdentyHashFn concept
 *              (default: DefaultHash producing SHA-256)
 *
 * @param stream Output stream to write raw hash bytes to
 * @param mb Motherboard structure to hash
 *
 * @note This function checks stream buffer space and may silently fail if
 *       insufficient space is available (current implementation does not throw)
 *
 * @see write_hash(std::ostream&, Hash&&) for writing pre-computed hashes
 */
template<hs::IdentyHashFn Hash = hs::detail::DefaultHash>
void write_hash(std::ostream& stream, const Motherboard& mb);

/**
 * @brief Computes and writes hash of extended motherboard information to stream
 *
 * Template function that computes a cryptographic hash of the extended motherboard
 * data (including drives) using the specified hash algorithm, then writes the raw
 * hash bytes to the output stream.
 *
 * @tparam Hash Hash function type satisfying IdentyHashExFn concept
 *              (default: DefaultHashEx producing SHA-256)
 *
 * @param stream Output stream to write raw hash bytes to
 * @param mb MotherboardEx structure to hash
 *
 * @warning The drives vector in mb must be sorted before calling to ensure
 *          consistent hash generation
 *
 * @note This function checks stream buffer space and may silently fail if
 *       insufficient space is available (current implementation does not throw)
 *
 * @see write_hash(std::ostream&, Hash&&) for writing pre-computed hashes
 */
template<hs::IdentyHashExFn Hash = hs::detail::DefaultHashEx>
void write_hash(std::ostream& stream, const MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
/**
 * @brief Writes raw hash bytes to output stream
 *
 * Writes the complete raw byte content of a hash structure directly to the
 * output stream without any encoding or formatting. Useful for transmitting
 * hashes over network connections or saving them to binary files.
 *
 * @tparam Hash Hash type satisfying IdentyHashCompatible concept (Hash128/256/512)
 *
 * @param stream Output stream to write to (must be in good state)
 * @param hash Hash structure containing raw bytes to write
 *
 * @note Checks stream buffer space before writing. May silently fail if
 *       insufficient space (current implementation does not throw)
 *
 * @note For binary protocols, ensure stream is in binary mode to prevent
 *       line-ending translation corrupting the hash data
 */
template<hs::IdentyHashCompatible Hash>
void write_hash(std::ostream& stream, Hash&& hash);
} // namespace identy::io

template<identy::hs::IdentyHashFn Hash>
void identy::io::write_hash(std::ostream& stream, const Motherboard& mb)
{
    auto hash = Hash {}(mb);
    write_hash(stream, hash);
}

template<identy::hs::IdentyHashExFn Hash>
void identy::io::write_hash(std::ostream& stream, const MotherboardEx& mb)
{
    auto hash = Hash {}(mb);
    write_hash(stream, hash);
}

template<identy::hs::IdentyHashCompatible Hash>
void identy::io::write_hash(std::ostream& stream, Hash&& hash)
{
    if(!stream.good()) {
        return; // todo: throw exception?
    }

    stream.write(reinterpret_cast<const char*>(hash.buffer), sizeof(hash.buffer));
}

#endif
