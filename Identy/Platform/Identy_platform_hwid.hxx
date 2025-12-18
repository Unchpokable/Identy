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

} // namespace identy

namespace identy::platform
{

/**
 * @brief Platform-specific SMBIOS retrieval
 * @return Smart pointer to raw SMBIOS data, or nullptr on failure
 */
SMBIOS_Raw::Ptr get_smbios();

/**
 * @brief Platform-specific drive enumeration
 * @return Vector of physical drive information
 */
std::vector<PhysicalDriveInfo> list_drives();

} // namespace identy::platform

#endif
