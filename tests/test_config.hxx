#pragma once

#ifndef IDENTY_TEST_CONFIG_H
#define IDENTY_TEST_CONFIG_H

#include <string>

namespace identy::test
{

/**
 * @brief Determines expected environment for VM detection tests
 *
 * Priority:
 * 1. IDENTY_TEST_EXPECT_VM define (CMake option) - expects VM
 * 2. IDENTY_TEST_EXPECT_BAREMETAL define (CMake option) - expects bare metal
 * 3. CI/GITHUB_ACTIONS defines - auto-detect CI as VM
 * 4. Default: assume bare metal (local development)
 */
#if defined(IDENTY_TEST_EXPECT_VM)
inline constexpr bool kExpectVirtualMachine = true;
inline constexpr bool kEnvironmentExplicitlySet = true;
#elif defined(IDENTY_TEST_EXPECT_BAREMETAL)
inline constexpr bool kExpectVirtualMachine = false;
inline constexpr bool kEnvironmentExplicitlySet = true;
#elif defined(CI) || defined(GITHUB_ACTIONS)
inline constexpr bool kExpectVirtualMachine = true;
inline constexpr bool kEnvironmentExplicitlySet = false;
#else
inline constexpr bool kExpectVirtualMachine = false;
inline constexpr bool kEnvironmentExplicitlySet = false;
#endif

/**
 * @brief Returns human-readable description of expected environment
 */
inline const char* GetExpectedEnvironmentDescription()
{
    if constexpr (kExpectVirtualMachine) {
        return "Virtual Machine (CI/Cloud)";
    } else {
        return "Bare Metal (Physical Hardware)";
    }
}

/**
 * @brief Known CPU vendors for validation
 */
inline constexpr const char* kKnownCpuVendors[] = {
    "GenuineIntel",   // Intel
    "AuthenticAMD",   // AMD
    "AMDisbetter!",   // Early AMD
    "CentaurHauls",   // VIA/Centaur
    "CyrixInstead",   // Cyrix
    "TransmetaCPU",   // Transmeta
    "GenuineTMx86",   // Transmeta
    "Geode by NSC",   // National Semiconductor
    "NexGenDriven",   // NexGen
    "RiseRiseRise",   // Rise
    "SiS SiS SiS ",   // SiS
    "UMC UMC UMC ",   // UMC
    "VIA VIA VIA ",   // VIA
    "Vortex86 SoC",   // Vortex
    // Hypervisor vendors (also valid in VM environments)
    "KVMKVMKVM",      // KVM
    "Microsoft Hv",   // Microsoft Hyper-V
    "VMwareVMware",   // VMware
    "XenVMMXenVMM",   // Xen
    "prl hyperv  ",   // Parallels
    "VBoxVBoxVBox",   // VirtualBox
    " lrpepyh vr ",   // Parallels (alternate)
    "bhyve bhyve ",   // bhyve
    "QNXQVMBSQG",     // QNX
    "ACRNACRNACRN",   // ACRN
};

/**
 * @brief Check if a CPU vendor string is known/valid
 */
inline bool IsKnownCpuVendor(const std::string& vendor)
{
    for (const auto* known : kKnownCpuVendors) {
        if (vendor.find(known) != std::string::npos ||
            std::string(known).find(vendor) != std::string::npos) {
            return true;
        }
    }
    // Also accept if it contains common substrings
    return vendor.find("Intel") != std::string::npos ||
           vendor.find("AMD") != std::string::npos ||
           vendor.find("ARM") != std::string::npos;
}

} // namespace identy::test

#endif // IDENTY_TEST_CONFIG_H
