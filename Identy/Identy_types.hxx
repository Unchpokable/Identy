#pragma once

#ifndef UNC_IDENTY_TYPES_H
#define UNC_IDENTY_TYPES_H

#include <cstdint>

#ifdef IDENTY_WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace identy
{
#ifdef IDENTY_WIN32
/** @brief Platform-specific byte type alias (Windows BYTE) */
using byte = BYTE;
/** @brief Platform-specific double word (32-bit) type alias (Windows DWORD) */
using dword = DWORD;
/** @brief Platform-specific word (16-bit) type alias (Windows WORD) */
using word = WORD;
#else
/** @brief Platform-specific byte type alias (unsigned char) */
using byte = unsigned char;
/** @brief Platform-specific double word (32-bit) type alias */
using dword = std::uint32_t;
/** @brief Platform-specific word (16-bit) type alias */
using word = std::uint16_t;
#endif
} // namespace identy

#endif