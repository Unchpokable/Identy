#include "Identy_pch.hxx"

#include "Identy_hwid.hxx"

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
void copy_byte(identy::register_32* from, identy::byte* to, std::ptrdiff_t index)
{
    const identy::byte* byte_ptr = reinterpret_cast<const identy::byte*>(from);
    *to = byte_ptr[index];
}
} // namespace

namespace
{
#ifdef IDENTY_WIN32
identy::SMBIOS_Raw::Ptr get_smbios_win32()
{
    identy::dword size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if(size == 0) {
        return identy::SMBIOS_Raw::Ptr();
    }

    identy::SMBIOS_Raw::Ptr smbios_ptr;
    smbios_ptr.allocate(size);

    GetSystemFirmwareTable('RSMB', 0, smbios_ptr, static_cast<identy::dword>(smbios_ptr.bytes_length()));

    return smbios_ptr;
}

#define get_smbios_impl get_smbios_win32
#else
identy::SMBIOS get_smbios_linux()
{
    // todo: open /sys/firmware/dmi/tables/DMI and read it into SMBIOS.data field

    return identy::SMBIOS::Ptr();
}

#define get_smbios_impl get_smbios_linux
#endif

std::vector<std::uint8_t> get_smbios_uuid(identy::SMBIOS_Raw::Ptr& smbios)
{
    identy::byte* smbios_begin = smbios->SMBIOS_table_data;
    identy::byte* smbios_end = smbios_begin + smbios->length;

    std::vector<std::uint8_t> buffer;
    buffer.resize(smbios_end - smbios_begin);

    while(smbios_begin < smbios_end) {
        auto header = reinterpret_cast<identy::SMBIOS_Header*>(smbios_begin);

        if(header->type == SMBIOS_type_system_information && header->length >= SMBIOS_system_information_header_length) {
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

    __cpuid(cpu_info, cpuleaf_vendorID);

    char vendor[13] = { 0 };

    std::memcpy(vendor + 0, &cpu_info[EBX], sizeof(identy::register_32));
    std::memcpy(vendor + 4, &cpu_info[EDX], sizeof(identy::register_32));
    std::memcpy(vendor + 8, &cpu_info[ECX], sizeof(identy::register_32));

    cpu.vendor = std::string(vendor);

    identy::register_32 max_leaf = cpu_info[EAX];

    __cpuid(cpu_info, cpuleaf_family);

    std::memcpy(&cpu.version, &cpu_info[EAX], sizeof(identy::register_32));

    cpu.hypervisor_bit = (cpu_info[ECX] >> 31) & 1;

    identy::register_32 ebx_val;
    std::memcpy(&ebx_val, &cpu_info[EBX], sizeof(identy::register_32));

    copy_byte(&ebx_val, &cpu.brand_index, 0);
    copy_byte(&ebx_val, &cpu.clflush_line_size, 1);
    copy_byte(&ebx_val, &cpu.apic_id, 3);

    std::memcpy(&cpu.instruction_set.basic, &cpu_info[EDX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.modern, &cpu_info[ECX], sizeof(identy::register_32));

    __cpuidex(cpu_info, cpuleaf_ext_instructions, 0);

    std::memcpy(&cpu.instruction_set.extended_modern[0], &cpu_info[EBX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.extended_modern[1], &cpu_info[ECX], sizeof(identy::register_32));
    std::memcpy(&cpu.instruction_set.extended_modern[2], &cpu_info[EDX], sizeof(identy::register_32));

    char brand[49] = { 0 };

    __cpuid(reinterpret_cast<identy::register_32*>(brand + 0), cpuleaf_ext_brand + 0);
    __cpuid(reinterpret_cast<identy::register_32*>(brand + 16), cpuleaf_ext_brand + 1);
    __cpuid(reinterpret_cast<identy::register_32*>(brand + 32), cpuleaf_ext_brand + 2);

    cpu.extended_brand_string = std::string(brand);

    if(cpu.hypervisor_bit) {
        char hyperv_sig[13] = { 0 };
        __cpuid(cpu_info, cpuleaf_hypervisor);

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
            __cpuidex(cpu_info, leaf_to_use, level);

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
        __cpuid(cpu_info, cpuleaf_family);
        identy::byte nb_proc;
        copy_byte(&cpu_info[EBX], &nb_proc, 2);
        cpu.logical_processors_count = static_cast<identy::register_32>(nb_proc);
    }
    else {
        cpu.too_old = true;
    }

    return cpu;
}

#ifdef IDENTY_WIN32
std::optional<identy::PhysicalDriveInfo> get_drive_info(std::string_view drive_name)
{
    auto path = std::format(R"(\\.\{})", drive_name);

    HANDLE h_device = CreateFileA(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, NULL);
    if(h_device == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    identy::PhysicalDriveInfo info;

    STORAGE_PROPERTY_QUERY query;
    ZeroMemory(&query, sizeof(query));

    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    identy::byte buffer[1024];
    identy::dword bytes_returned = 0;

    if(!DeviceIoControl(h_device, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &bytes_returned, NULL)) {
        CloseHandle(h_device);
        return std::nullopt;
    }

    STORAGE_DEVICE_DESCRIPTOR* desc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(buffer);

    switch(desc->BusType) {
        case BusTypeNvme:
            info.bus_type = identy::PhysicalDriveInfo::NMVe;
            break;

        case BusTypeSata:
            info.bus_type = identy::PhysicalDriveInfo::SATA;
            break;

        case BusTypeUsb:
            info.bus_type = identy::PhysicalDriveInfo::USB;
            break;
    }

    if(desc->SerialNumberOffset != 0 && desc->SerialNumberOffset < sizeof(buffer)) {
        info.serial = std::string(reinterpret_cast<const char*>(buffer) + desc->SerialNumberOffset);
    }

    info.serial.erase(std::remove(info.serial.begin(), info.serial.end(), ' '), info.serial.end());

    info.device_name = drive_name;

    CloseHandle(h_device);

    return info;
}

std::vector<identy::PhysicalDriveInfo> list_drives_win32()
{
    constexpr identy::dword buffer_size = 65536;
    thread_local char buffer[buffer_size];

    identy::dword count = QueryDosDeviceA(NULL, buffer, buffer_size);
    if(count == 0) {
        return {};
    }

    std::vector<std::string> drives;

    char* current_info = buffer;
    while(*current_info) {
        std::string_view device_name(current_info);

        if(device_name.starts_with("PhysicalDrive")) {
            drives.emplace_back(device_name);
        }

        current_info += device_name.size() + 1;
    }

    std::vector<identy::PhysicalDriveInfo> drive_infos;
    for(auto& drive_name : drives) {
        auto result = get_drive_info(drive_name);
        if(result.has_value()) {
            drive_infos.push_back(std::move(result.value()));
        }
    }

    return drive_infos;
}

#define list_drives_impl list_drives_win32
#else
std::vector<identy::PhysicalDriveInfo> list_drives_linux()
{
    // todo: list all physical drives using linux\posix API

    std::vector<identy::PhysicalDriveInfo> drive_infos;

    return drive_infos;
}

#define list_drives_impl list_drives_linux
#endif

} // namespace

identy::Motherboard identy::snap_motherboard()
{
    Motherboard motherboard;
    motherboard.cpu = get_cpu_info();

    auto smbios_raw = get_smbios_impl();

#ifdef IDENTY_WIN32
    motherboard.smbios.is_20_calling_used = smbios_raw->used_20_calling_method == 1;
    motherboard.smbios.major_version = smbios_raw->SMBIOS_major_version;
    motherboard.smbios.minor_version = smbios_raw->SMBIOS_minor_version;
    motherboard.smbios.dmi_version = smbios_raw->dmi_revision;
#else
    // todo: get from Linux system APIs
    motherboard.smbios.is_20_calling_used = false;
    motherboard.smbios.major_version = 255;
    motherboard.smbios.minor_version = 255;
    motherboard.smbios.dmi_version = 255;
#endif

    motherboard.smbios.raw_tables_data.resize(smbios_raw->length);
    std::memcpy(motherboard.smbios.raw_tables_data.data(), smbios_raw->SMBIOS_table_data, smbios_raw->length);

    auto uuid = get_smbios_uuid(smbios_raw);
    std::memcpy(motherboard.smbios.uuid, uuid.data(), std::min(uuid.size(), sizeof(motherboard.smbios.uuid)));

    return motherboard;
}

identy::MotherboardEx identy::snap_motherboard_ex()
{
    MotherboardEx motherboard;

    auto short_mb = snap_motherboard();

    motherboard.cpu = short_mb.cpu;
    motherboard.smbios = short_mb.smbios;

    motherboard.drives = list_drives();

    return motherboard;
}

std::vector<identy::PhysicalDriveInfo> identy::list_drives()
{
    return list_drives_impl();
}
