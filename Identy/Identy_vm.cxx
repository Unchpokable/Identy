#include "Identy_pch.hxx"

#include "Identy_vm.hxx"

#include "Platform/Identy_platform_vm.hxx"

namespace
{
constexpr const char* microsoft_hyperv_sig = "Microsoft Hv";

static constexpr std::array known_hypervisor_signatures {
    "KVM",
    "KVMKVMKVM",
    "VMwareVMware",
    "VBoxVBoxVBox",
    "TCGTCGTCG",
    "ACRNACRN",
    "bhyve bhyve",
    "Xen",
    microsoft_hyperv_sig,
};

static constexpr std::array known_vm_manufacturers {
    "innotek GmbH",
    "Oracle",
    "VMware, Inc.",
    "QEMU",
    "Xen",
    "Microsoft Corporation",
    "Parallels",
};

static constexpr std::array known_vm_network_adapters {
    "vmware",
    "vmxnet",
    "vmnet", // VMware
    "virtualbox",
    "vbox", // VirtualBox
    "hyper-v",
    "microsoft hyper-v", // Hyper-V
    "virtio",
    "red hat virtio", // KVM/QEMU
    "xennet",
    "xen",       // Xen
    "parallels", // Parallels
};

static constexpr std::array known_vm_drives_products {
    "VBOX",
    "VMWARE",
    "QEMU",
    "VIRTUAL",
    "XEN",
    "KVM",
    "RED HAT",
    "VIRTIO",
    "MSFT",
    "MICROSOFT VIRTUAL",
};

static constexpr std::array suspiciuos_buses {
    identy::PhysicalDriveInfo::SAS,
    identy::PhysicalDriveInfo::Scsi,
    identy::PhysicalDriveInfo::ATA,
};
} // namespace

namespace
{
constexpr std::uint32_t SMBIOS_type_system_manufacturer = 1;
constexpr std::ptrdiff_t SMBIOS_system_manufacturer_offset = 4;
} // namespace

namespace
{
constexpr char ctolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr bool char_equal_icase(char a, char b) noexcept
{
    return ctolower(static_cast<unsigned char>(a)) == ctolower(static_cast<unsigned char>(b));
}

constexpr bool contains_icase(std::string_view string, std::string_view substring) noexcept
{
    if(substring.empty()) {
        return true;
    }
    if(substring.size() > string.size()) {
        return false;
    }

    auto it = std::search(string.begin(), string.end(), substring.begin(), substring.end(), char_equal_icase);
    return it != string.end();
}
} // namespace

namespace
{
void check_network_adapters(identy::vm::HeuristicVerdict& verdict)
{
    bool access_denied = false;
    auto adapters = identy::platform::list_network_adapters(access_denied);

    if(access_denied) {
        verdict.detections.push_back(identy::vm::VMFlags::Platform_AccessToNetworkDevicesDenied);
        return;
    }

    int virtual_adapters_count = 0;
    int total_adapters_count = 0;

    for(const auto& adapter : adapters) {
        std::string_view desc { adapter.description };

        auto is_virtual = std::ranges::any_of(known_vm_network_adapters, [desc](std::string_view key) {
            return contains_icase(desc, key);
        });

        if(is_virtual) {
            virtual_adapters_count++;
            total_adapters_count++;
        }
        else {
            if(!adapter.is_loopback && !adapter.is_tunnel) {
                total_adapters_count++;
            }
        }
    }

    if(virtual_adapters_count > 0) {
        verdict.detections.push_back(identy::vm::VMFlags::Platform_VirtualNetworkAdaptersPresent);
    }

    if(virtual_adapters_count == total_adapters_count && total_adapters_count > 0) {
        verdict.detections.push_back(identy::vm::VMFlags::Platform_OnlyVirtualNetworkAdapters);
    }
}
} // namespace

namespace
{
std::string_view get_smbios_string(const identy::SMBIOS_Header* header, std::uint8_t index)
{
    if(index == 0) {
        return {};
    }

    auto ptr = reinterpret_cast<const char*>(header) + header->length;

    while(--index > 0 && *ptr) {
        ptr += std::string_view(ptr).size() + 1;
    }

    return { ptr };
}

std::string get_smbios_manufacturer(const identy::SMBIOS& smbios)
{
    auto raw_smbios = smbios.raw_tables_data.data();

    auto raw_smbios_end = raw_smbios + smbios.raw_tables_data.size();

    std::string_view manufacturer;

    while(raw_smbios < raw_smbios_end) {
        auto header = reinterpret_cast<const identy::SMBIOS_Header*>(raw_smbios);

        if(header->type == SMBIOS_type_system_manufacturer) {
            auto manufacturer_idx = raw_smbios + SMBIOS_system_manufacturer_offset;

            if(*manufacturer_idx != 0) {
                manufacturer = get_smbios_string(header, *manufacturer_idx);
            }
        }

        // skip strings block
        auto next = raw_smbios + header->length;

        while(next + 1 < raw_smbios_end && (*next != 0 || *(next + 1) != 0)) {
            next++;
        }

        if(next + 2 > raw_smbios_end) {
            break;
        }

        raw_smbios = next + 2;
    }

    return std::string(manufacturer);
}

bool is_hvci(const identy::Cpu& cpu, const identy::SMBIOS& smbios)
{
    if(!cpu.hypervisor_bit) {
        return false;
    }

    if(cpu.hypervisor_signature != microsoft_hyperv_sig) {
        return false;
    }

    auto manufacturer = get_smbios_manufacturer(smbios);

    auto is_known_manufacturer = std::ranges::any_of(known_vm_manufacturers, [manufacturer](const std::string& man) {
        return manufacturer.find(man) != std::string_view::npos;
    });

    if(is_known_manufacturer) {
        return false;
    }

    // todo: check UUID

    return true;
}

void check_smbios(const identy::SMBIOS& smbios, identy::vm::HeuristicVerdict& verdict)
{
    auto manufacturer = get_smbios_manufacturer(smbios);
    auto is_known_manufacturer = std::ranges::any_of(known_vm_manufacturers, [manufacturer](const std::string& man) {
        return manufacturer.find(man) != std::string_view::npos;
    });

    if(is_known_manufacturer) {
        verdict.detections.push_back(identy::vm::VMFlags::SMBIOS_SuspiciousManufacturer);
    }

    identy::byte zeroes[sizeof(smbios.uuid)] {};
    std::memset(zeroes, 0, sizeof(zeroes));

    if(std::memcmp(smbios.uuid, zeroes, sizeof(zeroes)) == 0) {
        // whole UUID is zeroed - VM
        verdict.detections.push_back(identy::vm::VMFlags::SMBIOS_SuspiciousUUID);
        verdict.detections.push_back(identy::vm::VMFlags::SMBIOS_UUIDTotallyZeroed);
    }
}

void check_drive(const identy::PhysicalDriveInfo& drive, identy::vm::HeuristicVerdict& verdict, int& product_id_known_vm_count)
{
    auto full_model_name = drive.vendor_id + " " + drive.product_id;

    if(std::ranges::any_of(known_vm_drives_products, [&full_model_name](std::string_view product) {
           return contains_icase(full_model_name, product);
       })) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_ProductIdKnownVM);
        ++product_id_known_vm_count;
    }

    if(drive.bus_type == identy::PhysicalDriveInfo::Virtual) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_BusTypeIsVirtual);
    }

    if(drive.serial.empty() || drive.serial.find_first_not_of(drive.serial[0]) == std::string_view::npos) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_SuspiciousSerial);
    }

    if(std::ranges::any_of(suspiciuos_buses, [&drive](const identy::PhysicalDriveInfo::BusType& bus) {
           return drive.bus_type == bus;
       })) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_BusTypeUncommon);
    }
}
} // namespace

namespace
{
template<typename MB>
identy::vm::HeuristicVerdict check_mb_common(const MB& mb)
{
    identy::vm::HeuristicVerdict verdict;

    if(is_hvci(mb.cpu, mb.smbios)) {
        verdict.detections.push_back(identy::vm::VMFlags::Platform_HyperVIsolation);
    }
    else {
        if(mb.cpu.hypervisor_bit) {
            verdict.detections.push_back(identy::vm::VMFlags::Cpu_Hypervisor_bit);
        }

        if(std::ranges::any_of(known_hypervisor_signatures, [&mb](const std::string& sig) {
               return mb.cpu.hypervisor_signature.find(sig) != std::string::npos;
           })) {
            verdict.detections.push_back(identy::vm::VMFlags::Cpu_Hypervisor_signature);
        }
    }

    check_smbios(mb.smbios, verdict);
    check_network_adapters(verdict);

    return verdict;
}
} // namespace

constexpr identy::vm::detail::FlagStrength identy::vm::detail::get_flag_strength(VMFlags flag)
{
    switch(flag) {
        case VMFlags::Platform_HyperVIsolation:
        case VMFlags::Platform_VirtualNetworkAdaptersPresent:
            return FlagStrength::Weak;

        case VMFlags::SMBIOS_SuspiciousUUID:
        case VMFlags::Platform_OnlyVirtualNetworkAdapters:
        case VMFlags::Storage_BusTypeUncommon:
            return FlagStrength::Medium;

        case VMFlags::Cpu_Hypervisor_bit:
        case VMFlags::Cpu_Hypervisor_signature:
        case VMFlags::Storage_BusTypeIsVirtual:
        case VMFlags::Storage_ProductIdKnownVM:
        case VMFlags::SMBIOS_SuspiciousManufacturer:
            return FlagStrength::Strong;

        case VMFlags::SMBIOS_UUIDTotallyZeroed:
        case VMFlags::Storage_AllDrivesBusesVirtual:
        case VMFlags::Storage_AllDrivesVendorProductKnownVM:
            return FlagStrength::Critical;

        case VMFlags::Storage_SuspiciousSerial:
        case VMFlags::Platform_WindowsRegistry:
        case VMFlags::Platform_LinuxDevices:
        case VMFlags::Platform_AccessToNetworkDevicesDenied:
            return FlagStrength::Medium;

        default:
            return FlagStrength::Weak;
    }
}

identy::vm::VMConfidence identy::vm::detail::calculate_confidence(const std::vector<VMFlags>& detections)
{
    int weak = 0, medium = 0, strong = 0;
    bool critical = false;

    for(auto flag : detections) {
        switch(get_flag_strength(flag)) {
            case FlagStrength::Weak:
                ++weak;
                break;
            case FlagStrength::Medium:
                ++medium;
                break;
            case FlagStrength::Strong:
                ++strong;
                break;
            case FlagStrength::Critical:
                critical = true;
                break;
        }
    }

    if(critical) {
        return VMConfidence::DefinitelyVM;
    }

    if(strong >= 2) {
        return VMConfidence::DefinitelyVM;
    }

    if(strong >= 1 || medium >= 3) {
        return VMConfidence::Probable;
    }

    if(medium >= 1 || weak >= 2) {
        return VMConfidence::Possible;
    }

    return VMConfidence::Unlikely;
}

identy::vm::HeuristicVerdict identy::vm::DefaultHeuristic::operator()(const Motherboard& mb) const
{
    auto verdict = check_mb_common(mb);
    verdict.confidence = detail::calculate_confidence(verdict.detections);

    return verdict;
}

identy::vm::HeuristicVerdict identy::vm::DefaultHeuristicEx::operator()(const MotherboardEx& mb) const
{
    auto verdict = check_mb_common(mb);

    int product_vm_count {};
    for(auto& disk : mb.drives) {
        check_drive(disk, verdict, product_vm_count);
    }

    auto virtual_buses = std::ranges::count_if(mb.drives, [](const identy::PhysicalDriveInfo& drive) {
        return drive.bus_type == identy::PhysicalDriveInfo::Virtual;
    });

    if(!mb.drives.empty() && virtual_buses == mb.drives.size()) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_AllDrivesBusesVirtual);
    }

    if(!mb.drives.empty() && product_vm_count == mb.drives.size()) {
        verdict.detections.push_back(identy::vm::VMFlags::Storage_AllDrivesVendorProductKnownVM);
    }

    verdict.confidence = detail::calculate_confidence(verdict.detections);

    return verdict;
}
