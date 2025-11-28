# Identy

![AI generated banner](./mics/Banner_aigen.png)

> A modern C++20 hardware fingerprinting library for robust device identification

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)](https://github.com/yourusername/identy)

## Overview

**Identy** is a lightweight, cross-platform C++ library designed for creating stable hardware fingerprints without direct system API interaction complexity. It provides easy access to CPU information, SMBIOS firmware data, and physical storage device identifiers — everything needed to generate unique, persistent machine identifiers.

### Key Features

- **CPU Identification** — Extract vendor, model, instruction sets (SSE, AVX, etc.), and processor metadata via CPUID
- **SMBIOS Access** — Read motherboard firmware tables including system UUID and hardware identifiers
- **Storage Enumeration** — List physical drives with serial numbers and bus types (SATA, NVMe, USB)
- **Cryptographic Hashing** — Built-in SHA-256 fingerprint generation with customizable hash algorithms
- **VM Detection** — Detect virtualized environments (VMware, VirtualBox, Hyper-V, etc.)
- **Cross-Platform** — Windows and Linux support with platform-specific optimizations
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
- **Windows 10+** or **Linux** (kernel 3.0+)

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/identy.git
cd identy

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Integration

#### Option 1: CMake Subdirectory

```cmake
add_subdirectory(identy)
target_link_libraries(your_target PRIVATE Identy)
```

#### Option 2: Header-Only Usage

Simply include the main header:

```cpp
#include <Identy/Identy.h>
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
#include <algorithm>

int main() {
    // Capture extended information including physical drives
    auto mb = identy::snap_motherboard_ex();

    // Sort drives for consistent hashing
    std::sort(mb.drives.begin(), mb.drives.end(),
              [](auto& a, auto& b) { return a.serial < b.serial; });

    // Generate comprehensive fingerprint
    auto fingerprint = identy::hs::hash(mb);

    // Print drive information
    for (const auto& drive : mb.drives) {
        std::cout << "Drive: " << drive.device_name
                  << " | Serial: " << drive.serial << std::endl;
    }

    return 0;
}
```

### CPU Information Access

```cpp
auto mb = identy::snap_motherboard();

std::cout << "CPU Vendor: " << mb.cpu.vendor << std::endl;
std::cout << "Brand: " << mb.cpu.extended_brand_string << std::endl;
std::cout << "Logical Cores: " << (int)mb.cpu.logical_processors_count << std::endl;

// Check for specific instruction set support
bool has_avx = mb.cpu.instruction_set.modern & (1 << 28);  // AVX bit
std::cout << "AVX Support: " << (has_avx ? "Yes" : "No") << std::endl;
```

### Virtual Machine Detection

```cpp
auto mb = identy::snap_motherboard();

if (identy::assume_virtual(mb)) {
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
Enumerates all physical storage devices without capturing CPU/SMBIOS.

**Returns:** `std::vector<PhysicalDriveInfo>`

### Hashing Functions

#### `identy::hs::hash(const Motherboard& mb)`
Generates SHA-256 fingerprint from basic motherboard data.

**Returns:** `Hash256` (32-byte array)

#### `identy::hs::hash(const MotherboardEx& mb)`
Generates SHA-256 fingerprint including storage device information.

**Returns:** `Hash256` (32-byte array)

**Warning:** Drives must be sorted before hashing for consistent results.

#### `identy::hs::compare(Hash lhs, Hash rhs)`
Compares two hash values using constant-time comparison.

**Returns:** `0` if equal, non-zero otherwise

### I/O Functions

#### `identy::io::write_text(std::ostream& stream, const Motherboard& mb)`
Writes human-readable hardware information to output stream.

#### `identy::io::write_binary(std::ostream& stream, const Motherboard& mb)`
Writes compact binary representation of hardware data.

#### `identy::io::write_hash(std::ostream& stream, Hash&& hash)`
Writes raw hash bytes to output stream.

### VM Detection

#### `identy::assume_virtual(const Motherboard& mb)`
Detects if the system is running in a virtual machine environment.

**Returns:** `bool` — `true` if VM detected

## Data Structures

### `identy::Cpu`
Contains comprehensive CPU information:
- `vendor` — CPU manufacturer string
- `extended_brand_string` — Full processor model name
- `version` — Processor version info
- `logical_processors_count` — Number of logical cores
- `instruction_set` — Feature flags (SSE, AVX, AES-NI, etc.)

### `identy::SMBIOS`
SMBIOS firmware data:
- `uuid[16]` — System UUID from SMBIOS Type 1
- `major_version`, `minor_version` — SMBIOS spec version
- `raw_tables_data` — Complete SMBIOS tables

### `identy::PhysicalDriveInfo`
Physical storage device information:
- `device_name` — System device path (`\\.\PhysicalDrive0`, `/dev/sda`)
- `serial` — Drive serial number
- `bus_type` — Connection type (`SATA`, `NMVe`, `USB`)

### `identy::Motherboard`
Basic hardware snapshot:
- `cpu` — CPU information
- `smbios` — Firmware data

### `identy::MotherboardEx`
Extended hardware snapshot:
- `cpu` — CPU information
- `smbios` — Firmware data
- `drives` — Physical storage devices

## Hash Types

| Type | Size | Use Case |
|------|------|----------|
| `Hash128` | 16 bytes | Compact identifiers, MD5 compatibility |
| `Hash256` | 32 bytes | Default, SHA-256 compatible (recommended) |
| `Hash512` | 64 bytes | Maximum collision resistance, SHA-512 |

## Platform Specifics

### Windows
- Uses `GetSystemFirmwareTable` API for SMBIOS access
- Requires `advapi32.lib` linkage
- Drive enumeration may need admin rights

### Linux
- Reads SMBIOS from `/sys/firmware/dmi/tables/DMI`
- Drive information from `/dev/` and `sysfs`
- May require root for certain operations

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

- **snap_motherboard()**: ~1-5ms (no disk I/O)
- **snap_motherboard_ex()**: ~10-50ms (includes drive enumeration)
- **hash()**: ~0.1ms (SHA-256 computation)
- **Zero heap allocations** for basic operations (except `std::string` fields)

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
