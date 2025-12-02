/**
 * @file Identy_vm.hxx
 * @brief Virtual machine detection and heuristic analysis
 *
 * Provides functionality for detecting virtualized environments by analyzing
 * hardware characteristics, SMBIOS data, and platform-specific indicators.
 * The detection system uses a multi-factor heuristic approach combining
 * various evidence sources with weighted confidence scoring.
 *
 * ## Detection Methods
 *
 * - **CPU Features** - Hypervisor CPUID bit and vendor signatures
 * - **SMBIOS Analysis** - Suspicious manufacturer strings and UUID patterns
 * - **Storage Devices** - Virtual drive serial number patterns
 * - **Platform Specifics** - Registry keys (Windows), device files (Linux)
 * - **Network Adapters** - Virtual network adapter detection
 * - **Security Features** - HVCI/Core Isolation detection
 *
 * ## Confidence Levels
 *
 * Detection results include confidence scoring from "Unlikely" to "DefinitelyVM"
 * based on the strength and quantity of detected indicators.
 *
 * @note VM detection is heuristic-based and may produce false positives/negatives.
 *       Use as one factor in security decisions, not as sole authentication method.
 */

#pragma once

#ifndef UNC_IDENTY_VM_H
#define UNC_IDENTY_VM_H

#include "Identy_hwid.hxx"

namespace identy::vm
{
/**
 * @brief Virtual machine detection flags enumeration
 *
 * Defines individual detection signals that may indicate a virtualized
 * environment. Each flag represents a specific detection method or indicator
 * discovered during hardware analysis.
 */
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

/**
 * @brief Confidence level enumeration for VM detection results
 *
 * Represents the overall confidence that the system is running in a
 * virtualized environment, based on accumulated detection flag strengths.
 */
enum class VMConfidence {
    Unlikely,     ///< Very low probability of VM (no or only weak indicators)
    Possible,     ///< Some indicators present but inconclusive
    Probable,     ///< Strong indicators present, likely virtualized
    DefinitelyVM, ///< Multiple critical indicators, almost certainly a VM
};

namespace detail
{
/**
 * @brief Internal strength classification for individual detection flags
 *
 * Each VMFlags indicator has an associated strength level that affects
 * how much it contributes to the overall confidence calculation.
 */
enum class FlagStrength {
    Weak,     ///< Minor indicator (e.g., suspicious UUID pattern)
    Medium,   ///< Moderate indicator (e.g., VM manufacturer string)
    Strong,   ///< Strong indicator (e.g., hypervisor CPUID bit)
    Critical, ///< Definitive indicator (e.g., confirmed VM signature)
};

/**
 * @brief Maps a detection flag to its strength level
 *
 * @param flag Detection flag to query
 * @return Strength classification for the flag
 */
constexpr FlagStrength get_flag_strength(VMFlags flag);

/**
 * @brief Calculates overall VM confidence from detected flags
 *
 * Aggregates individual flag strengths to produce an overall confidence
 * level indicating the likelihood of virtualization.
 *
 * @param detections Vector of detected VMFlags
 * @return Overall confidence level (Unlikely to DefinitelyVM)
 */
VMConfidence calculate_confidence(const std::vector<VMFlags>& detections);
} // namespace detail

/**
 * @brief Result structure from heuristic VM analysis
 *
 * Contains all detected VM indicators and an overall confidence assessment
 * of whether the system is virtualized.
 */
struct HeuristicVerdict
{
    /** @brief List of all VM indicators that were detected */
    std::vector<VMFlags> detections;

    /** @brief Overall confidence level of VM presence */
    VMConfidence confidence { VMConfidence::Unlikely };

    /**
     * @brief Convenience method to check if system is likely virtual
     *
     * @return true if confidence is Probable or DefinitelyVM, false otherwise
     */
    bool is_virtual() const
    {
        return confidence >= VMConfidence::Probable;
    }
};

/**
 * @brief Default heuristic functor for basic motherboard analysis
 *
 * Analyzes CPU and SMBIOS data to detect VM indicators without requiring
 * extended hardware enumeration.
 */
struct DefaultHeuristic
{
    /**
     * @brief Performs VM detection analysis on basic motherboard data
     *
     * @param mb Motherboard structure with CPU and SMBIOS information
     * @return HeuristicVerdict containing detected flags and confidence level
     */
    HeuristicVerdict operator()(const Motherboard& mb) const;
};

/**
 * @brief Default heuristic functor for extended motherboard analysis
 *
 * Analyzes CPU, SMBIOS, and storage device data to detect VM indicators
 * with additional drive serial number checking.
 */
struct DefaultHeuristicEx
{
    /**
     * @brief Performs VM detection analysis on extended motherboard data
     *
     * @param mb MotherboardEx structure with CPU, SMBIOS, and drive information
     * @return HeuristicVerdict containing detected flags and confidence level
     */
    HeuristicVerdict operator()(const MotherboardEx& mb) const;
};

/**
 * @brief C++20 concept defining valid heuristic functions for basic motherboard
 *
 * A heuristic must be invocable with a Motherboard reference and return a
 * HeuristicVerdict, with trivial construction/destruction for zero overhead.
 *
 * @tparam T Type to validate as a heuristic functor
 */
template<typename T>
concept Heuristic = std::is_invocable_r_v<HeuristicVerdict, T, const Motherboard&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;

/**
 * @brief C++20 concept defining valid heuristic functions for extended motherboard
 *
 * A heuristic must be invocable with a MotherboardEx reference and return a
 * HeuristicVerdict, with trivial construction/destruction for zero overhead.
 *
 * @tparam T Type to validate as a heuristic functor
 */
template<typename T>
concept HeuristicEx = std::is_invocable_r_v<HeuristicVerdict, T, const MotherboardEx&> && std::is_trivially_constructible_v<T>
    && std::is_trivially_destructible_v<T>;
} // namespace identy::vm

namespace identy::vm
{
/**
 * @brief Checks if basic motherboard data indicates a virtual machine
 *
 * Performs heuristic analysis of CPU and SMBIOS data to determine if the
 * system is running in a virtualized environment. Returns a simple boolean
 * verdict based on the confidence threshold (Probable or higher).
 *
 * @tparam Heuristic Heuristic functor type (default: DefaultHeuristic)
 *
 * @param mb Motherboard structure with CPU and SMBIOS information
 * @return true if confidence is Probable or DefinitelyVM, false otherwise
 *
 * @note This is a convenience wrapper around analyze_full() that returns
 *       only the boolean verdict. Use analyze_full() to access detailed
 *       detection flags and confidence levels.
 *
 * @see analyze_full()
 * @see HeuristicVerdict
 */
template<Heuristic Heuristic = DefaultHeuristic>
bool assume_virtual(const Motherboard& mb);

/**
 * @brief Checks if extended motherboard data indicates a virtual machine
 *
 * Performs comprehensive heuristic analysis of CPU, SMBIOS, and storage
 * device data to determine if the system is running in a virtualized
 * environment. Returns a simple boolean verdict based on confidence threshold.
 *
 * @tparam Heuristic Heuristic functor type (default: DefaultHeuristicEx)
 *
 * @param mb MotherboardEx structure with CPU, SMBIOS, and drive information
 * @return true if confidence is Probable or DefinitelyVM, false otherwise
 *
 * @note This is a convenience wrapper around analyze_full() that returns
 *       only the boolean verdict. Use analyze_full() to access detailed
 *       detection flags and confidence levels.
 *
 * @see analyze_full()
 * @see HeuristicVerdict
 */
template<HeuristicEx Heuristic = DefaultHeuristicEx>
bool assume_virtual(const MotherboardEx& mb);

/**
 * @brief Performs full VM detection analysis on basic motherboard data
 *
 * Executes comprehensive heuristic analysis and returns detailed results
 * including all detected indicators and confidence scoring. Use this function
 * when you need to understand which specific VM indicators were found.
 *
 * @tparam Heuristic Heuristic functor type (default: DefaultHeuristic)
 *
 * @param mb Motherboard structure with CPU and SMBIOS information
 * @return HeuristicVerdict with detected flags and confidence level
 *
 * @note The returned verdict includes the complete list of detected VM
 *       indicators, allowing for custom confidence thresholds or logging.
 *
 * @see assume_virtual()
 * @see HeuristicVerdict
 */
template<Heuristic Heuristic = DefaultHeuristic>
HeuristicVerdict analyze_full(const Motherboard& mb);

/**
 * @brief Performs full VM detection analysis on extended motherboard data
 *
 * Executes comprehensive heuristic analysis including storage device checks
 * and returns detailed results with all detected indicators and confidence
 * scoring. Use this function when you need to understand which specific VM
 * indicators were found.
 *
 * @tparam Heuristic Heuristic functor type (default: DefaultHeuristicEx)
 *
 * @param mb MotherboardEx structure with CPU, SMBIOS, and drive information
 * @return HeuristicVerdict with detected flags and confidence level
 *
 * @note The returned verdict includes the complete list of detected VM
 *       indicators, allowing for custom confidence thresholds or logging.
 *
 * @see assume_virtual()
 * @see HeuristicVerdict
 */
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
