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
    Platform_HyperVIsolation,               ///< Hardware Windows running with "Core Integrity" settings
};

enum class VMConfidence {
    Unlikely,
    Possible,
    Probable,
    DefinitelyVM,
};

namespace detail
{
enum class FlagStrength {
    Weak,
    Medium,
    Strong,
    Critical,
};

constexpr FlagStrength get_flag_strength(VMFlags flag);
VMConfidence calculate_confidence(const std::vector<VMFlags>& detections);
} // namespace detail

/// Result of Heuristic analysis of hardware info.
/// detections field contains all flags that heuristic marked as detected
struct HeuristicVerdict
{
    std::vector<VMFlags> detections;
    VMConfidence confidence;

    bool is_virtual() const
    {
        return confidence >= VMConfidence::Probable;
    }
};

struct DefaultHeuristic
{
    HeuristicVerdict operator()(const Motherboard& mb) const;
};

struct DefaultHeuristicEx
{
    HeuristicVerdict operator()(const MotherboardEx& mb) const;
};

template<typename T>
concept Heuristic = std::is_invocable_r_v<HeuristicVerdict, T, const Motherboard&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;

template<typename T>
concept HeuristicEx = std::is_invocable_r_v<HeuristicVerdict, T, const MotherboardEx&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;
} // namespace identy::vm

namespace identy::vm
{
/// Returns a flag is Identy thinks is given motherboard is virtual
template<Heuristic Heuristic = DefaultHeuristic>
bool assume_virtual(const Motherboard& mb);

/// Returns a flag is Identy thinks is given motherboard is virtual
template<HeuristicEx Heuristic = DefaultHeuristicEx>
bool assume_virtual(const MotherboardEx& mb);

template<Heuristic Heuristic = DefaultHeuristic>
HeuristicVerdict analyze_full(const Motherboard& mb);

template<HeuristicEx Heuristic = DefaultHeuristicEx>
HeuristicVerdict analyze_full(const MotherboardEx& mb);
} // namespace identy::vm

template<identy::vm::Heuristic Heuristic>
bool identy::vm::assume_virtual(const Motherboard& mb)
{
    return Heuristic {}(mb).is_virtual();
}

template<identy::vm::HeuristicEx Heuristic>
bool identy::vm::assume_virtual(const MotherboardEx& mb)
{
    return Heuristic {}(mb).is_virtual();
}

template<identy::vm::Heuristic Heuristic>
identy::vm::HeuristicVerdict identy::vm::analyze_full(const Motherboard& mb)
{
    return Heuristic {}(mb);
}

template<identy::vm::HeuristicEx Heuristic>
identy::vm::HeuristicVerdict identy::vm::analyze_full(const MotherboardEx& mb)
{
    return Heuristic {}(mb);
}

#endif
