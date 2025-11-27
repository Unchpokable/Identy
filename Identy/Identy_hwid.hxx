#pragma once

#ifndef UNC_IDENTY_HWID_H
#define UNC_IDENTY_HWID_H

#include "Identy_global.h"
#include "Identy_platform.hxx"
#include "Identy_resource_handle.hxx"

namespace identy
{
#ifdef IDENTY_WIN32
using byte = BYTE;
using dword = DWORD;
using word = WORD;
#else
using byte = unsigned char;
using dword = std::uint32_t;
using word = std::uint16_t;
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
    using Ptr = CStdHandle<SMBIOS_Win32>;

    byte used_20_calling_method;
    byte SMBIOS_major_version;
    byte SMBIOS_minor_version;
    byte dmi_revision;
    dword length;
    byte SMBIOS_table_data[1];
};

using SMBIOS = SMBIOS_Win32;
#else
struct SMBIOS_Linux
{
    using Ptr = CStdHandle<SMBIOS_Linux>;

    dword length;
    byte SMBIOS_table_data[1];
};

using SMBIOS = SMBIOS_Linux;
#endif

struct SMBIOS_Header
{
    byte type;
    byte length;
    word handle;
};

struct Motherboard
{
    Cpu cpu;
};
} // namespace identy

namespace identy
{
IDENTY_EXPORT SMBIOS::Ptr get_smbios();
IDENTY_EXPORT std::vector<std::uint8_t> get_smbios_uuid();
} // namespace identy

namespace identy
{
IDENTY_EXPORT Cpu snap_cpu() noexcept;
} // namespace identy

#endif