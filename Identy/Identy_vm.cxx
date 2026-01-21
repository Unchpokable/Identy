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
} // namespace

namespace
{
constexpr std::uint32_t SMBIOS_type_system_manufacturer = 1;
constexpr std::ptrdiff_t SMBIOS_system_manufacturer_offset = 4;
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
        auto char_equal = [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        };

        auto desc = std::string_view(adapter.description);

        auto is_virtual = std::ranges::any_of(known_vm_network_adapters, [&](std::string_view key) {
            auto it = std::search(desc.begin(), desc.end(), key.begin(), key.end(), char_equal);
            return it != desc.end();
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
        case VMFlags::SMBIOS_SuspiciousManufacturer:
            return FlagStrength::Weak;

        case VMFlags::SMBIOS_SuspiciousUUID:
        case VMFlags::Platform_OnlyVirtualNetworkAdapters:
            return FlagStrength::Medium;

        case VMFlags::Cpu_Hypervisor_bit:
        case VMFlags::Cpu_Hypervisor_signature:
            return FlagStrength::Strong;

        case VMFlags::SMBIOS_UUIDTotallyZeroed:
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

    // todo: analyze disks

    verdict.confidence = detail::calculate_confidence(verdict.detections);

    return verdict;
}
