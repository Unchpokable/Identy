#pragma once

#ifndef UNC_IDENTY_HWID_H
#define UNC_IDENTY_HWID_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Identy_global.h"
#include "Identy_platform.hxx"
#include "Identy_types.hxx"

namespace identy
{
/**
 * @brief The length of SMBIOS UUID in bytes
 *
 * SMBIOS specification defines UUID as a 16-byte (128-bit) identifier
 * used to uniquely identify the system.
 */
constexpr std::size_t SMBIOS_uuid_length = 16;
} // namespace identy

namespace identy
{
/** @brief Type alias for 32-bit CPU register values used in CPUID operations */
using register_32 = std::int32_t;
} // namespace identy

namespace identy
{
/**
 * @brief CPU information structure containing identification and capability data
 *
 * This structure encapsulates comprehensive CPU information obtained through
 * the CPUID instruction, including vendor identification, processor version,
 * cache line parameters, and supported instruction sets.
 */
struct Cpu
{
    /** @brief CPU vendor identification string (e.g., "GenuineIntel", "AuthenticAMD") */
    std::string vendor;

    /** @brief Processor version information from CPUID EAX register (leaf 0x01) */
    register_32 version;

    /** @brief Hypervisor bit */
    bool hypervisor_bit;

    /** @brief Brand index value indicating the processor brand string table index */
    std::uint8_t brand_index;

    /** @brief CLFLUSH instruction cache line size in 8-byte increments */
    std::uint8_t clflush_line_size;

    /** @brief Number of logical processors per physical package */
    register_32 logical_processors_count;

    /** @brief Advanced Programmable Interrupt Controller (APIC) ID */
    std::uint8_t apic_id;

    /** @brief Extended processor brand string (human-readable model name) */
    std::string extended_brand_string;

    /** @brief Signature of current hypervisor (if present, empty string otherwise) */
    std::string hypervisor_signature;

    /**
     * @brief Nested structure containing CPU instruction set capability flags
     *
     * Contains feature flags from various CPUID leaves indicating supported
     * instruction sets and CPU capabilities.
     */
    struct _instruction_set
    {
        /** @brief Basic instruction set features from CPUID leaf 0x01 (EDX register) */
        register_32 basic;

        /** @brief Modern instruction set features from CPUID leaf 0x01 (ECX register) */
        register_32 modern;

        /** @brief Extended modern instruction set features from CPUID leaf 0x07 (EBX, ECX, EDX registers) */
        register_32 extended_modern[3];
    } instruction_set;

    /**
     * @brief Flag indicates that processor is TOO OLD and some fields can be invalid
     */
    bool too_old { false };
};

#pragma pack(push, 1)
/**
 * @brief Standard SMBIOS structure header
 *
 * Represents the common header present at the beginning of every SMBIOS
 * structure table entry. This header contains essential metadata for
 * identifying and parsing SMBIOS structures.
 *
 * @note The complete SMBIOS structure extends beyond this header with
 *       type-specific data fields and string data. This header contains
 *       only the most essential fields required for structure navigation.
 *
 * @see SMBIOS specification for complete structure definitions
 */
struct SMBIOS_Header
{
    /** @brief SMBIOS structure type identifier (e.g., 0=BIOS, 1=System, 2=Baseboard) */
    byte type;

    /** @brief Length of the formatted area of the structure (excluding strings) */
    byte length;

    /** @brief Unique handle number for this structure instance */
    word handle;
};
#pragma pack(pop)

/**
 * @brief Managed SMBIOS data structure using modern C++ containers
 *
 * High-level SMBIOS representation that avoids raw C-style memory management
 * by using std::vector for dynamic data storage. This structure is safe to
 * use directly without requiring smart pointer handles.
 *
 * @note This structure provides RAII-compliant memory management and is
 *       the recommended interface for accessing SMBIOS data in application code.
 */
struct SMBIOS
{
    /** @brief Indicates whether SMBIOS 2.0 calling convention was used */
    bool is_20_calling_used;

    /** @brief SMBIOS specification major version number */
    byte major_version;

    /** @brief SMBIOS specification minor version number */
    byte minor_version;

    /** @brief Desktop Management Interface (DMI) version number */
    byte dmi_version;

    /** @brief System UUID (128-bit universally unique identifier) as defined by SMBIOS Type 1 */
    byte uuid[SMBIOS_uuid_length];

    /** @brief Complete raw SMBIOS table data copied from firmware, managed by std::vector */
    std::vector<std::uint8_t> raw_tables_data;
};

/**
 * @brief Basic motherboard information structure
 *
 * Contains essential hardware identification data from the system motherboard,
 * including CPU characteristics and SMBIOS firmware data.
 *
 * @note This is the lightweight version without physical drive enumeration.
 *       For complete hardware information including storage devices, use
 *       MotherboardEx instead.
 *
 * @see MotherboardEx
 * @see snap_motherboard()
 */
struct Motherboard
{
    /** @brief Information about the installed CPU */
    Cpu cpu;

    /** @brief SMBIOS data from system firmware */
    SMBIOS smbios;
};

/**
 * @brief Physical storage drive information
 *
 * Contains identification and connection information for physical storage
 * devices attached to the system.
 */
struct PhysicalDriveInfo
{
    /**
     * @brief Storage device bus connection type enumeration
     */
    enum BusType {
        SATA, /**< Serial ATA interface */
        NMVe, /**< NVM Express (Non-Volatile Memory express) interface */
        USB,  /**< Universal Serial Bus external interface */
        Other
    } bus_type { SATA };

    /** @brief Drive device name for current session (.\\\\.\\PhysicalDriveN for Windows, /dev/ for Linux) */
    std::string device_name;

    /** @brief Drive serial number string for unique identification */
    std::string serial;
};

/**
 * @brief Extended motherboard information with storage device enumeration
 *
 * Comprehensive hardware identification structure that includes CPU, SMBIOS
 * firmware data, and a complete list of installed physical storage drives.
 *
 * @warning On Windows systems, enumerating physical drives may require
 *          administrative privileges. The function may return an empty
 *          drives vector if insufficient permissions are available.
 *
 * @see Motherboard
 * @see snap_motherboard_ex()
 */
struct MotherboardEx
{
    /** @brief Information about the installed CPU */
    Cpu cpu;

    /** @brief SMBIOS data from system firmware */
    SMBIOS smbios;

    /** @brief List of all detected physical storage drives in the system */
    std::vector<PhysicalDriveInfo> drives;
};

} // namespace identy

namespace identy
{
} // namespace identy

namespace identy
{
/**
 * @brief Captures basic motherboard and CPU identification information
 *
 * Retrieves hardware identification data from the system including CPU
 * characteristics (vendor, model, instruction sets) and SMBIOS firmware
 * information (system UUID, version data, raw tables).
 *
 * This function performs read-only operations and does not enumerate
 * physical storage drives. For drive information, use snap_motherboard_ex()
 * or enumerate drives separately.
 *
 * @return Motherboard structure containing CPU and SMBIOS data
 *
 * @throws noexcept This function does not throw exceptions
 *
 * @note This function typically does not require elevated privileges
 * @note Platform-specific: Uses CPUID instruction for CPU data and
 *       GetSystemFirmwareTable on Windows or /sys/firmware/dmi on Linux
 *       for SMBIOS data
 *
 * @see snap_motherboard_ex()
 * @see Motherboard
 */
IDENTY_EXPORT Motherboard snap_motherboard();

/**
 * @brief Captures complete motherboard information including storage devices
 *
 * Retrieves comprehensive hardware identification data from the system,
 * including CPU characteristics, SMBIOS firmware information, and a complete
 * enumeration of all installed physical storage drives with their serial
 * numbers and bus types.
 *
 * @return MotherboardEx structure containing CPU, SMBIOS, and drive information
 *
 * @throws noexcept This function does not throw exceptions
 *
 * @warning On Windows systems, enumerating physical drives may require
 *          administrative privileges. If insufficient permissions are available,
 *          the drives vector in the returned structure may be empty while
 *          CPU and SMBIOS data will still be populated.
 *
 * @note This function performs more extensive system queries than
 *       snap_motherboard() and may take longer to execute
 *
 * @see snap_motherboard()
 * @see MotherboardEx
 */
IDENTY_EXPORT MotherboardEx snap_motherboard_ex();
} // namespace identy

namespace identy
{
IDENTY_EXPORT std::vector<PhysicalDriveInfo> list_drives();
} // namespace identy

#endif
