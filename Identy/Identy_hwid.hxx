#pragma once

#ifndef UNC_IDENTY_HWID_H
#define UNC_IDENTY_HWID_H

#include "global.h"

namespace identy
{
#ifdef IDENTY_WIN32
using identy_byte = BYTE;
using identy_dword = DWORD;
using identy_word = WORD;
#else
using identy_byte = unsigned char;
using identy_dword = std::uint32_t;
using identy_word = std::uint16_t;
#endif
} // namespace identy

namespace identy
{
using register_32 = std::uint32_t;
} // namespace identy

namespace identy
{
struct IDENTY_EXPORT Cpu
{
    std::string vendor;
    std::uint8_t version;
    std::uint8_t brand_index;
    std::uint8_t clflush_line_size;
    std::uint8_t logical_processors_count;
    std::uint8_t apic_id;
    std::string extended_brand_string;

    struct _instruction_set
    {
        register_32 basic;
        register_32 modern;
        register_32 extended_modern;
    } instruction_set;
};

#ifdef IDENTY_WIN32
struct SMBIOS_Win32
{
    identy_byte used_20_calling_method;
    identy_byte SMBIOS_major_version;
    identy_byte SMBIOS_minor_version;
    identy_byte dmi_revision;
    identy_dword length;
    identy_byte SMBIOS_table_data[1];
};

using SMBIOS = SMBIOS_Win32;
#else
struct SMBIOS_Linux
{
    identy_dword length;
    identy_byte SMBIOS_table_data[1];
};

using SMBIOS = SMBIOS_Linux;
#endif

struct SMBIOS_Header
{
    identy_byte type;
    identy_byte length;
    identy_word handle;
};

struct Motherboard
{
    Cpu cpu;
};
} // namespace identy

namespace identy
{
IDENTY_EXPORT Cpu snap_cpu() noexcept;
} // namespace identy

#endif