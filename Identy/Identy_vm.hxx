#pragma once

#ifndef UNC_IDENTY_VM_H
#define UNC_IDENTY_VM_H

#include "Identy_hwid.hxx"

namespace identy::vm
{
enum class VMFlags {
    Cpu_Hypervisor_bit,                     ///< Detected Hypervisor flag at CPUID
    Cpu_Hypervisor_signature,               ///< Detected known Hypervisor signature
    SMBIOS_SuspiciousManufacturer,          ///< Detected known VM software manufacturer in SMBIOS
    SMBIOS_SuspiciousUUID,                  ///< UUID of motherboard SMBIOS is suspicious and can be spoofed or virtual
    SMBIOS_UUIDTotallyZeroed,               ///< UUID of motherboard is completely zeroed
    Storage_SuspiciousSerial,               ///< Serial number of any drive looks like virtual device
    Platform_WindowsRegistry,               ///< for Windows platforms - detected registry records pointing to VM
    Platform_LinuxDevices,                  ///< for Linux platforms - some devices looks lite it's a virtual devices
    Platform_VirtualNetworkAdaptersPresent, ///< Detected virtual network adapter
    Platform_OnlyVirtualNetworkAdapters,    ///< All network adapters is virtual
    Platform_AccessToNetworkDevicesDenied,  ///< Operating system denied access to network adapters
};

enum class VMConfidence {
    // todo: ???
};

/// Result of Heuristic analysis of hardware info.
/// detections field contains all flags that heuristic marked as detected
struct HeuristicVerdict
{
    std::vector<VMFlags> detections;
    VMConfidence confidence;
};

struct DefaultHeuristic
{
    HeuristicVerdict operator()(const identy::Motherboard& mb) const;
};

struct DefaultHeuristicEx
{
    HeuristicVerdict operator()(const identy::MotherboardEx& mb) const;
};

template<typename T>
concept Heuristic = std::is_invocable_r_v<HeuristicVerdict, T, const identy::Motherboard&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;

template<typename T>
concept HeuristicEx = std::is_invocable_r_v<HeuristicVerdict, T, const identy::MotherboardEx&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;
} // namespace identy::vm

namespace identy::vm
{
/// Returns a flag is Identy thinks is given motherboard is virtual
template<Heuristic Heuristic = DefaultHeuristic>
bool assume_virtual(const identy::Motherboard& mb);

/// Returns a flag is Identy thinks is given motherboard is virtual
template<HeuristicEx Heuristic = DefaultHeuristicEx>
bool assume_virtual(const identy::MotherboardEx& mb);
} // namespace identy::vm

template<identy::vm::Heuristic Heuristic>
bool identy::vm::assume_virtual(const identy::Motherboard& mb)
{
    auto verdict = Heuristic {}(mb);

    return false;
}

template<identy::vm::HeuristicEx Heuristic>
bool identy::vm::assume_virtual(const identy::MotherboardEx& mb)
{
    auto verdict = Heuristic {}(mb);

    return false;
}

#endif
