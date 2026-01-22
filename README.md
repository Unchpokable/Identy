# Identy

![AI generated banner](./misc/Banner_aigen.png)

> A modern C++20 hardware fingerprinting library for robust device identification

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)](https://github.com/Unchpokable/Identy)

## Overview

**Identy** is a lightweight C++ library designed for creating stable hardware fingerprints without direct system API interaction complexity. It provides easy access to CPU information, SMBIOS firmware data, and physical storage device identifiers — everything needed to generate unique, persistent machine identifiers.

### Key Features

- **CPU Identification** — Extract vendor, model, instruction sets (SSE, AVX, etc.), hypervisor presence, and processor metadata via CPUID
- **SMBIOS Access** — Read motherboard firmware tables including system UUID, version info, and raw firmware data
- **Storage Enumeration** — List physical drives with serial numbers, vendor/product IDs, and bus types (SATA, NVMe, USB, Virtual, SCSI, SAS, ATA)
- **Cryptographic Hashing** — Built-in SHA-256 fingerprint generation with template-based customizable hash algorithms
- **VM Detection** — Multi-factor heuristic detection of virtualized environments (VMware, VirtualBox, Hyper-V, QEMU, KVM, Xen, etc.) with confidence scoring
- **Network Adapter Analysis** — Detect virtual network adapters as part of VM detection heuristics
- **Cross-Platform** — Windows 10+ (full support), Linux (partial support)
- **Modern C++** — C++20 concepts, RAII-compliant memory management, zero-cost abstractions

## Use Cases

- **Software Licensing** — Bind licenses to specific hardware configurations
- **Device Authentication** — Create persistent machine identifiers for security systems
- **Anti-Fraud Systems** — Detect device fingerprint changes and cloned environments
- **Hardware Inventory** — Collect detailed system information for asset management
- **Telemetry & Analytics** — Track unique installations without user identification

## Quick Start

### Requirements

- **C++20** compatible compiler (MSVC 19.28+, GCC 10+, Clang 11+)
- **CMake** 3.10 or higher
- **Windows 10+** (full support) or **Linux** (partial support)

### Building

```bash
# Clone the repository
git clone https://github.com/Unchpokable/Identy.git
cd identy

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Integration

#### CMake Subdirectory

```cmake
add_subdirectory(identy)
target_link_libraries(your_target PRIVATE Identy)
```

## Usage Examples

### Basic Hardware Fingerprint

```cpp
#include <Identy/Identy.h>
#include <iostream>

int main() {
    // Capture motherboard information (CPU + SMBIOS)
    auto mb = identy::snap_motherboard();

    // Generate SHA-256 hardware fingerprint
    auto fingerprint = identy::hs::hash(mb);

    // Output as hex string
    std::cout << "Hardware ID: ";
    for (auto byte : fingerprint.buffer) {
        std::cout << std::hex << (int)byte;
    }
    std::cout << std::endl;

    return 0;
}
```

### Extended Fingerprint with Storage Devices

```cpp
#include <Identy/Identy.h>

int main() {
    // Capture extended information including physical drives
    // Drives are automatically sorted by serial number
    auto mb = identy::snap_motherboard_ex();

    // Generate comprehensive fingerprint
    auto fingerprint = identy::hs::hash(mb);

    // Print drive information
    for (const auto& drive : mb.drives) {
        std::cout << "Drive: " << drive.device_name
                  << " | Serial: " << drive.serial
                  << " | Vendor: " << drive.vendor_id << std::endl;
    }

    return 0;
}
```

### CPU Information Access

```cpp
auto mb = identy::snap_motherboard();

std::cout << "CPU Vendor: " << mb.cpu.vendor << std::endl;
std::cout << "Brand: " << mb.cpu.extended_brand_string << std::endl;
std::cout << "Logical Cores: " << mb.cpu.logical_processors_count << std::endl;

// Check for hypervisor presence
if (mb.cpu.hypervisor_bit) {
    std::cout << "Hypervisor: " << mb.cpu.hypervisor_signature << std::endl;
}

// Check for specific instruction set support
bool has_avx = mb.cpu.instruction_set.modern & (1 << 28);  // AVX bit
std::cout << "AVX Support: " << (has_avx ? "Yes" : "No") << std::endl;
```

### Virtual Machine Detection

```cpp
auto mb = identy::snap_motherboard();

if (identy::vm::assume_virtual(mb)) {
    std::cout << "Running in virtual environment" << std::endl;
} else {
    std::cout << "Running on physical hardware" << std::endl;
}
```

### Text Output for Debugging

```cpp
#include <fstream>

auto mb = identy::snap_motherboard_ex();

// Write human-readable output
std::ofstream file("hardware_info.txt");
identy::io::write_text(file, mb);

// Write binary format
std::ofstream binfile("hardware_info.bin", std::ios::binary);
identy::io::write_binary(binfile, mb);
```

### Comparing Fingerprints

```cpp
auto mb1 = identy::snap_motherboard();
auto mb2 = identy::snap_motherboard();

auto hash1 = identy::hs::hash(mb1);
auto hash2 = identy::hs::hash(mb2);

if (identy::hs::compare(hash1, hash2) == 0) {
    std::cout << "Identical hardware" << std::endl;
}
```

## API Reference

### Core Functions

#### `identy::snap_motherboard()`
Captures basic motherboard information (CPU + SMBIOS). Does not require elevated privileges.

**Returns:** `Motherboard` structure with CPU and SMBIOS data

#### `identy::snap_motherboard_ex()`
Captures extended information including physical storage drives.

**Returns:** `MotherboardEx` structure with CPU, SMBIOS, and drive data

**Note:** May require administrator privileges on Windows to enumerate drives.

#### `identy::list_drives()`
Enumerates all physical storage devices without capturing CPU/SMBIOS data.

**Returns:** `std::vector<PhysicalDriveInfo>` — Vector of physical drive information structures

**Note:** May require administrator privileges on Windows to access drive information.

### Hashing Functions

#### `identy::hs::hash<Hash>(const Motherboard& mb)`
Generates cryptographic fingerprint from basic motherboard data.

**Template Parameter:** `Hash` — Hash function type satisfying `IdentyHashFn` concept (default: `DefaultHash` producing SHA-256)

**Returns:** `Hash::Type` (default: `Hash256`, 32-byte array)

#### `identy::hs::hash<Hash>(const MotherboardEx& mb)`
Generates cryptographic fingerprint including storage device information.

**Template Parameter:** `Hash` — Hash function type satisfying `IdentyHashExFn` concept (default: `DefaultHashEx` producing SHA-256)

**Returns:** `Hash::Type` (default: `Hash256`, 32-byte array)

**Note:** Drives with bus type `USB` or `Other` are excluded from hash computation for stability. The `snap_motherboard_ex()` function automatically sorts drives by serial number.

#### `identy::hs::compare<Hash>(Hash&& lhs, Hash&& rhs)`
Compares two hash values.

**Returns:** `0` if equal, non-zero otherwise

### I/O Functions

#### `identy::io::write_text(std::ostream& stream, const Motherboard& mb)`
#### `identy::io::write_text(std::ostream& stream, const MotherboardEx& mb)`
Writes human-readable hardware information to output stream.

#### `identy::io::write_binary(std::ostream& stream, const Motherboard& mb)`
#### `identy::io::write_binary(std::ostream& stream, const MotherboardEx& mb)`
Writes compact binary representation of hardware data.

**Note:** Stream must be opened in binary mode (`std::ios::binary`).

#### `identy::io::write_hash<Hash>(std::ostream& stream, const Motherboard& mb)`
#### `identy::io::write_hash<Hash>(std::ostream& stream, const MotherboardEx& mb)`
Computes hash and writes raw bytes to output stream.

#### `identy::io::write_hash<Hash>(std::ostream& stream, Hash&& hash)`
Writes pre-computed raw hash bytes to output stream.

### VM Detection

#### `identy::vm::assume_virtual<Heuristic>(const Motherboard& mb)`
#### `identy::vm::assume_virtual<Heuristic>(const MotherboardEx& mb)`
Detects if the system is running in a virtual machine environment using heuristic analysis.

**Template Parameter:** `Heuristic` — Custom heuristic functor (default: `DefaultHeuristic` or `DefaultHeuristicEx`)

**Returns:** `bool` — `true` if VM detected with "Probable" or higher confidence

**Note:** This is a heuristic method that combines multiple detection signals. For detailed analysis including individual detection flags and confidence levels, use `identy::vm::analyze_full()`.

#### `identy::vm::analyze_full<Heuristic>(const Motherboard& mb)`
#### `identy::vm::analyze_full<Heuristic>(const MotherboardEx& mb)`
Performs comprehensive VM detection and returns detailed analysis results.

**Template Parameter:** `Heuristic` — Custom heuristic functor (default: `DefaultHeuristic` or `DefaultHeuristicEx`)

**Returns:** `HeuristicVerdict` — Structure containing detected VM indicators and confidence level

#### `identy::vm::HeuristicVerdict`
Result structure from VM analysis:
- `detections` — Vector of `VMFlags` that were detected
- `confidence` — Overall `VMConfidence` level
- `is_virtual()` — Returns `true` if confidence is `Probable` or `DefinitelyVM`

#### `identy::vm::VMConfidence`
Confidence level enumeration:
| Level | Description |
|-------|-------------|
| `Unlikely` | No or only weak indicators |
| `Possible` | Some indicators present but inconclusive |
| `Probable` | Strong indicators present, likely virtualized |
| `DefinitelyVM` | Multiple critical indicators, almost certainly a VM |

#### `identy::vm::VMFlags`
Detection indicators enumeration:

| Flag | Description |
|------|-------------|
| `Cpu_Hypervisor_bit` | Hypervisor flag at CPUID |
| `Cpu_Hypervisor_signature` | Known hypervisor signature detected |
| `SMBIOS_SuspiciousManufacturer` | Known VM manufacturer in SMBIOS |
| `SMBIOS_SuspiciousUUID` | Suspicious UUID pattern |
| `SMBIOS_UUIDTotallyZeroed` | UUID is all zeros |
| `Storage_SuspiciousSerial` | Drive serial looks virtual |
| `Storage_BusTypeIsVirtual` | Drive bus type is Virtual |
| `Storage_AllDrivesBusesVirtual` | All drives have virtual bus |
| `Storage_BusTypeUncommon` | Uncommon bus type (SAS, SCSI, ATA) |
| `Storage_ProductIdKnownVM` | Known VM product ID |
| `Storage_AllDrivesVendorProductKnownVM` | All drives are known VM |
| `Platform_WindowsRegistry` | Windows registry VM keys |
| `Platform_LinuxDevices` | Linux virtual device files |
| `Platform_VirtualNetworkAdaptersPresent` | Virtual network adapter detected |
| `Platform_OnlyVirtualNetworkAdapters` | All adapters are virtual |
| `Platform_AccessToNetworkDevicesDenied` | OS denied network access |
| `Platform_HyperVIsolation` | Windows Hyper-V with Core Isolation |

**Example:**
```cpp
auto mb = identy::snap_motherboard();
auto verdict = identy::vm::analyze_full(mb);

if (verdict.is_virtual()) {
    std::cout << "VM detected with confidence: "
              << static_cast<int>(verdict.confidence) << std::endl;
    std::cout << "Detected indicators: " << verdict.detections.size() << std::endl;
}
```

## Data Structures

### `identy::Cpu`
Contains comprehensive CPU information:
- `vendor` — CPU manufacturer string (e.g., "GenuineIntel", "AuthenticAMD")
- `extended_brand_string` — Full processor model name (48 chars from CPUID)
- `version` — Processor version info from CPUID leaf 0x01
- `logical_processors_count` — Number of logical cores
- `hypervisor_bit` — Hypervisor presence flag (CPUID bit 31 ECX)
- `hypervisor_signature` — Hypervisor vendor signature if present (e.g., "VMwareVMware", "KVMKVMKVM")
- `brand_index` — Brand index value
- `clflush_line_size` — Cache line flush size
- `apic_id` — Advanced PIC ID
- `instruction_set` — Feature flags structure:
  - `basic` — Basic features from CPUID leaf 0x01 EDX
  - `modern` — Modern features from CPUID leaf 0x01 ECX (SSE, AVX, AES-NI, etc.)
  - `extended_modern[3]` — Extended features from CPUID leaf 0x07 (EBX, ECX, EDX)
- `too_old` — Flag indicating very old CPU with limited CPUID support

### `identy::SMBIOS`
SMBIOS firmware data:
- `uuid[16]` — System UUID from SMBIOS Type 1
- `major_version`, `minor_version` — SMBIOS spec version
- `dmi_version` — DMI version number
- `is_20_calling_used` — Whether SMBIOS 2.0 calling convention was used
- `raw_tables_data` — Complete raw SMBIOS tables

### `identy::PhysicalDriveInfo`
Physical storage device information:
- `device_name` — System device path (e.g., `\\.\PhysicalDrive0` on Windows, `/dev/sdX` on Linux)
- `serial` — Drive serial number (manufacturer-assigned unique identifier)
- `model_id` — Human-readable device model ID
- `vendor_id` — Human-readable device vendor ID
- `product_id` — Human-readable device product ID
- `bus_type` — Connection type enum:

| BusType | Description |
|---------|-------------|
| `SATA` | Serial ATA interface |
| `NMVe` | NVM Express interface |
| `USB` | Universal Serial Bus |
| `Virtual` | Virtual storage bus |
| `Scsi` | Old parallel SCSI bus |
| `ATA` | PATA (IDE) interface |
| `SAS` | Serial Attached SCSI (enterprise) |
| `Other` | Unknown or unrecognized bus type |

**Note:** Serial numbers may be empty if not provided by the drive firmware or if access is denied.

### `identy::Motherboard`
Basic hardware snapshot:
- `cpu` — CPU information
- `smbios` — Firmware data

### `identy::MotherboardEx`
Extended hardware snapshot:
- `cpu` — CPU information
- `smbios` — Firmware data
- `drives` — Physical storage devices (sorted by serial number)

## Hash Types

| Type | Size | Use Case |
|------|------|----------|
| `Hash128` | 16 bytes | Compact identifiers, MD5 compatibility |
| `Hash256` | 32 bytes | Default, SHA-256 compatible (recommended) |
| `Hash512` | 64 bytes | Maximum collision resistance, SHA-512 |

## Platform Specifics

### Windows
- Uses `GetSystemFirmwareTable` API for SMBIOS access
- Requires `advapi32.lib` and `iphlpapi.lib` linkage
- Drive enumeration may need admin rights
- Full CPUID support for CPU information extraction
- Registry-based VM detection for Hyper-V and other hypervisors
- Network adapter detection via `GetAdaptersInfo` API

### Linux
- Partial support implemented
- SMBIOS access via `/sys/firmware/dmi/` or `/dev/mem`
- Drive and network adapter enumeration under development

## Security Considerations

Hardware fingerprinting should be used responsibly:

- **User Consent** — Always inform users when collecting hardware identifiers
- **Privacy** — Hardware IDs are persistent and can track devices across reinstalls
- **Compliance** — Ensure compliance with GDPR, CCPA, and local privacy regulations
- **Storage** — Hash hardware data before storage/transmission
- **Purpose** — Use only for legitimate security/anti-fraud purposes

**Do not use** hardware fingerprinting for:
- Invasive user tracking
- Creating "super cookies" without consent
- Bypassing user privacy controls

## Performance

Typical execution times on modern hardware:

- **snap_motherboard()**: ~1-5ms (CPU CPUID + SMBIOS firmware table read)
- **snap_motherboard_ex()**: ~10-50ms (includes physical drive enumeration via WMI/DeviceIoControl)
- **hash()**: ~0.1ms (pure SHA-256 computation)
- **vm::assume_virtual()**: ~0.5-2ms (heuristic analysis with minimal overhead)
- **Zero heap allocations** for basic operations (except `std::string` fields and `std::vector` containers)

**Note:** Drive enumeration time varies based on the number of installed storage devices and may require administrative privileges on Windows.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License — see the [LICENSE.txt](LICENSE.txt) file for details.

## Acknowledgments

- SMBIOS specifications by DMTF
- Intel and AMD CPUID documentation
- Windows Firmware Table API documentation

---

**Made with C++20** | **Built for reliability** | **Designed for privacy-conscious developers**
