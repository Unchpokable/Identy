#include "Identy_pch.hxx"

#include "Identy_hwid.hxx"
#include "Platform/Identy_platform_hwid.hxx"

namespace
{
constexpr std::uint32_t SMBIOS_type_system_information = 1;
constexpr std::uint32_t SMBIOS_system_information_header_length = 24;
constexpr std::ptrdiff_t SMBIOS_uuid_offset = 8;
} // namespace

namespace
{
constexpr identy::register_32 cpuleaf_vendorID = 0x00000000;
constexpr identy::register_32 cpuleaf_family = 0x00000001;
constexpr identy::register_32 cpuleaf_ext_instructions = 0x00000007;
constexpr identy::register_32 cpuleaf_ext_brand = 0x80000002;
constexpr identy::register_32 cpuleaf_hypervisor = 0x40000000;
constexpr identy::register_32 cpuleaf_extended_topology = 0x0000001F;
constexpr identy::register_32 cpuleaf_extended_topology_legacy = 0x0000000B;
constexpr identy::register_32 cpuleaf_ext_brand_test = 0x80000000;
} // namespace

namespace
{
constexpr identy::dword EAX = 0;
constexpr identy::dword EBX = 1;
constexpr identy::dword ECX = 2;
constexpr identy::dword EDX = 3;
} // namespace

namespace
{
void intrin_cpuid(int registers[4], int leaf)
{
#ifdef IDENTY_MSVC
    __cpuid(registers, leaf);
#elif defined(IDENTY_GNUC) || defined(IDENTY_CLANG)
    unsigned int eax, ebx, ecx, edx;
    __cpuid(leaf, eax, ebx, ecx, edx);
    registers[0] = static_cast<int>(eax);
    registers[1] = static_cast<int>(ebx);
    registers[2] = static_cast<int>(ecx);
    registers[3] = static_cast<int>(edx);
#endif
}

void intrin_cpuidex(int registers[4], int leaf, int subleaf)
{
#ifdef IDENTY_MSVC
    __cpuidex(registers, leaf, subleaf);
#elif defined(IDENTY_GNUC) || defined(IDENTY_CLANG)
    unsigned int eax, ebx, ecx, edx;
    __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
    registers[0] = static_cast<int>(eax);
    registers[1] = static_cast<int>(ebx);
    registers[2] = static_cast<int>(ecx);
    registers[3] = static_cast<int>(edx);
#endif
}
} // namespace

namespace
{
void copy_byte(const identy::register_32* from, identy::byte* to, std::ptrdiff_t index)
{
    const identy::byte* byte_ptr = reinterpret_cast<const identy::byte*>(from);
    *to = byte_ptr[index];
}
} // namespace

namespace
{
std::vector<std::uint8_t> get_smbios_uuid(identy::SMBIOS_Raw::Ptr& smbios)
{
    identy::byte* smbios_begin = smbios->SMBIOS_table_data;
    identy::byte* smbios_end = smbios_begin + smbios->length;

    std::vector<std::uint8_t> buffer;
    buffer.reserve(identy::SMBIOS_uuid_length);

    while(smbios_begin < smbios_end) {
        auto header = reinterpret_cast<identy::SMBIOS_Header*>(smbios_begin);

        if(header->type == SMBIOS_type_system_information && header->length >= SMBIOS_system_information_header_length) {
            if(smbios_begin + SMBIOS_uuid_offset + identy::SMBIOS_uuid_length > smbios_end) {
                break;
            }

            identy::byte* uuid = smbios_begin + SMBIOS_uuid_offset;

            for(std::size_t i { 0 }; i < identy::SMBIOS_uuid_length; ++i) {
                buffer.emplace_back(uuid[i]);
            }

            return buffer;
        }

        identy::byte* next = smbios_begin + header->length;

        while(next + 1 < smbios_end && (*next != 0 || *(next + 1) != 0)) {
            next++;
        }

        if(next + 2 > smbios_end) {
            break;
        }

        smbios_begin = next + 2;
    }

    return {};
}

identy::Cpu get_cpu_info()
{
    identy::Cpu cpu;

    identy::register_32 cpu_info[4] = { -1 };

    intrin_cpuid(cpu_info, cpuleaf_vendorID);

    char vendor[13] = { 0 };

    std::memcpy(vendor + 0, &cpu_info[EBX], sizeof(identy::register_32));
    std::memcpy(vendor + 4, &cpu_info[EDX], sizeof(identy::register_32));
    std::memcpy(vendor + 8, &cpu_info[ECX], sizeof(identy::register_32));

    cpu.vendor = std::string(vendor);

    identy::register_32 max_leaf = cpu_info[EAX];

    intrin_cpuid(cpu_info, cpuleaf_family);

    std::memcpy(&cpu.version, &cpu_info[EAX], sizeof(identy::register_32));

    cpu.hypervisor_bit = (cpu_info[ECX] >> 31) & 1;

    identy::register_32 ebx_val;
    std::memcpy(&ebx_val, &cpu_info[EBX], sizeof(identy::register_32));

    copy_byte(&ebx_val, &cpu.brand_index, 0);
    copy_byte(&ebx_val, &cpu.clflush_line_size, 1);
    copy_byte(&ebx_val, &cpu.apic_id, 3);

    std::memcpy(&cpu.instruction_set.basic, &cpu_info[EDX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.modern, &cpu_info[ECX], sizeof(identy::register_32));

    intrin_cpuidex(cpu_info, cpuleaf_ext_instructions, 0);

    std::memcpy(&cpu.instruction_set.extended_modern[0], &cpu_info[EBX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.extended_modern[1], &cpu_info[ECX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.extended_modern[2], &cpu_info[EDX], sizeof(identy::register_32));

    intrin_cpuid(cpu_info, cpuleaf_ext_brand_test);
    auto max_extended_leaf = static_cast<unsigned int>(cpu_info[EAX]);

    if(max_extended_leaf >= static_cast<unsigned int>(cpuleaf_ext_brand_test) + 4) {
        char brand[49] = { 0 };

        intrin_cpuid(reinterpret_cast<identy::register_32*>(brand + 0), cpuleaf_ext_brand + 0);
        intrin_cpuid(reinterpret_cast<identy::register_32*>(brand + 16), cpuleaf_ext_brand + 1);
        intrin_cpuid(reinterpret_cast<identy::register_32*>(brand + 32), cpuleaf_ext_brand + 2);

        cpu.extended_brand_string = std::string(brand);
    }
    else {
        cpu.extended_brand_string = "unavailable";
        cpu.too_old = true;
    }

    if(cpu.hypervisor_bit) {
        char hyperv_sig[13] = { 0 };
        intrin_cpuid(cpu_info, cpuleaf_hypervisor);

        identy::register_32 max_hypervisor_leaf = cpu_info[EAX];

        if(max_hypervisor_leaf >= cpuleaf_hypervisor) {
            std::memcpy(hyperv_sig + 0, &cpu_info[EBX], sizeof(identy::register_32));
            std::memcpy(hyperv_sig + 4, &cpu_info[ECX], sizeof(identy::register_32));
            std::memcpy(hyperv_sig + 8, &cpu_info[EDX], sizeof(identy::register_32));

            cpu.hypervisor_signature = std::string(hyperv_sig);
        }
    }

    cpu.logical_processors_count = 1;

    identy::register_32 leaf_to_use = 0;

    if(max_leaf >= cpuleaf_extended_topology) {
        leaf_to_use = cpuleaf_extended_topology;
    }
    else if(max_leaf >= cpuleaf_extended_topology_legacy) {
        leaf_to_use = cpuleaf_extended_topology_legacy;
    }

    if(leaf_to_use != 0) {
        identy::register_32 level = 0;

        while(true) {
            intrin_cpuidex(cpu_info, leaf_to_use, level);

            identy::byte level_type;
            copy_byte(&cpu_info[ECX], &level_type, 1);

            if(level_type == 0) {
                break;
            }

            if(level_type == 2) {
                identy::register_32 nb_proc_full;
                std::memcpy(&nb_proc_full, &cpu_info[EBX], sizeof(identy::register_32));
                cpu.logical_processors_count = nb_proc_full & 0xFFFF;
            }

            level++;
        }
    }
    else if(max_leaf >= cpuleaf_family) {
        intrin_cpuid(cpu_info, cpuleaf_family);
        identy::byte nb_proc;
        copy_byte(&cpu_info[EBX], &nb_proc, 2);
        cpu.logical_processors_count = static_cast<identy::register_32>(nb_proc);
    }
    else {
        cpu.too_old = true;
    }

    return cpu;
}

} // namespace

identy::Motherboard identy::snap_motherboard() noexcept
{
    Motherboard motherboard;
    motherboard.cpu = get_cpu_info();

    auto smbios_raw = platform::get_smbios();
    if(smbios_raw == nullptr) {
        return motherboard;
    }

    motherboard.smbios.major_version = smbios_raw->SMBIOS_major_version;
    motherboard.smbios.minor_version = smbios_raw->SMBIOS_minor_version;
    motherboard.smbios.is_20_calling_used = smbios_raw->used_20_calling_method == 1;
    motherboard.smbios.dmi_version = smbios_raw->dmi_revision;

    motherboard.smbios.raw_tables_data.resize(smbios_raw->length);
    std::memcpy(motherboard.smbios.raw_tables_data.data(), smbios_raw->SMBIOS_table_data, smbios_raw->length);

    auto uuid = get_smbios_uuid(smbios_raw);
    std::memcpy(motherboard.smbios.uuid, uuid.data(), std::min(uuid.size(), sizeof(motherboard.smbios.uuid)));

    return motherboard;
}

identy::MotherboardEx identy::snap_motherboard_ex() noexcept
{
    MotherboardEx motherboard;

    auto short_mb = snap_motherboard();

    motherboard.cpu = short_mb.cpu;
    motherboard.smbios = short_mb.smbios;

    motherboard.drives = list_drives();

    return motherboard;
}

std::vector<identy::PhysicalDriveInfo> identy::list_drives() noexcept
{
    return platform::list_drives();
}
