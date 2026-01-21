#pragma once

#ifndef UNC_IDENTY_PLATFORM_HWID_H
#define UNC_IDENTY_PLATFORM_HWID_H

#include "../Identy_hwid.hxx"

namespace identy
{

#ifdef IDENTY_LINUX
#pragma pack(push, 1)
struct SMBIOS_EntryPoint32
{
    char anchor[4];
    byte checksum;
    byte length;
    byte major_version;
    byte minor_version;
    word max_structure_size;
    byte entry_point_revision;
    byte formatted_area[5];

    char intermediate_anchor[5];
    byte intermediate_checksum;
    word structure_table_length;
    dword structure_table_address;
    word number_of_structures;
    byte bcd_revision;
};

struct SMBIOS_EntryPoint64
{
    char anchor[5];
    byte checksum;
    byte length;
    byte major_version;
    byte minor_version;
    byte docrev;
    byte entry_point_revision;
    byte reserved;
    dword structure_table_max_size;
    qword structure_table_address;
};
#pragma pack(pop)

enum SMBIOS_Entry_Type {
    Unknown,
    Entry_32bit,
    Entry_64bit
};
#endif

/**
 * @brief Raw SMBIOS data returned from platform layer
 *
 * Contains version information and raw table data in a safe std::vector.
 * This replaces the old flexible-array-member based SMBIOS_Raw structure
 * for safer memory management.
 */
struct SMBIOS_RawData
{
    byte used_20_calling_method { 0 };
    byte major_version { 0 };
    byte minor_version { 0 };
    byte dmi_revision { 0 };
    std::vector<byte> table_data;

    [[nodiscard]] bool empty() const noexcept
    {
        return table_data.empty();
    }
};

} // namespace identy

namespace identy::platform
{

/**
 * @brief Platform-specific SMBIOS retrieval
 * @return SMBIOS data structure, empty if retrieval failed
 */
SMBIOS_RawData get_smbios();

/**
 * @brief Platform-specific drive enumeration
 * @return Vector of physical drive information
 */
std::vector<PhysicalDriveInfo> list_drives();

} // namespace identy::platform

#endif
