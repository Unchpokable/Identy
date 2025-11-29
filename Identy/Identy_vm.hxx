#pragma once

#ifndef UNC_IDENTY_VM_H
#define UNC_IDENTY_VM_H

#include "Identy_hwid.hxx"

namespace identy::vm
{
enum class VMFlags
{
    Cpu_Hypervisor, ///< Detected Hypervisor flag at CPUID or\and Hypervisor signature present
    SMBIOS_SuspiciousManufacturer, ///< Detected known VM software manufacturer in SMBIOS
    SMBIOS_SuspiciousUUID, ///< UUID of motherboard SMBIOS is suspicious and can be spoofed or virtual
    Storage_SuspiciousSerial, ///< Serial number of any drive looks like virtual device
    Platform_WindowsRegistry, ///< for Windows platforms - detected registry records pointing to VM
    Platform_LinuxDevices, ///< for Linux platforms - some devices looks lite it's a virtual devices
    Platform_VirtualNetworkAdaptersPresent, ///< Detected virtual network adapter
};

/// Result of Heuristic analysis of hardware info.
/// detections field contains all flags that heuristic marked as detected
/// confidence field contains a value in range [0..1] where 0 is "absolutely not virtual machine" and 1 is "absolutely virtual machine"
/// confidence less than 1 can be presented in cases when detected Microsoft Hyper-V because Windows Core Isolation mechanism can set up "hypervisor bit" and write hypervisor signature into CPUID
struct HeuristicVerdict
{
    std::vector<VMFlags> detections;
    float confidence { 0.0 };
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
}

namespace identy::vm
{
/// Returns a flag is Identy thinks is given motherboard is virtual
template<Heuristic Heuristic = DefaultHeuristic, float Threshold = 0.5>
bool assume_virtual(const identy::Motherboard& mb);

/// Returns a flag is Identy thinks is given motherboard is virtual
template<HeuristicEx Heuristic = DefaultHeuristicEx, float Threshold = 0.5>
bool assume_virtual(const identy::MotherboardEx& mb);
} // namespace identy

template<identy::vm::Heuristic Heuristic, float Threshold>
bool identy::vm::assume_virtual(const identy::Motherboard& mb)
{
    auto verdict = Heuristic {}(mb);
    return verdict.confidence >= Threshold;
}

template<identy::vm::HeuristicEx Heuristic, float Threshold>
bool identy::vm::assume_virtual(const identy::MotherboardEx& mb)
{
    auto verdict = Heuristic {}(mb);
    return verdict.confidence >= Threshold;
}

#endif
