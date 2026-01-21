#ifdef IDENTY_LINUX

#include "../Identy_pch.hxx"

#include "../Identy_strings.hxx"

#include "Identy_platform_hwid.hxx"

namespace
{

identy::SMBIOS_Entry_Type get_smbios_entry_type(const identy::byte* data, std::size_t size)
{
    if(size >= 4 && std::memcmp(data, "_SM_", 4) == 0) {
        return identy::Entry_32bit;
    }

    if(size >= 5 && std::memcmp(data, "_SM3_", 5) == 0) {
        return identy::SMBIOS_Entry_Type::Entry_64bit;
    }
    return identy::SMBIOS_Entry_Type::Unknown;
}

void read_smbios_versions(identy::SMBIOS_RawData& result, const std::vector<identy::byte>& entry_point_buffer)
{
    auto type = get_smbios_entry_type(entry_point_buffer.data(), entry_point_buffer.size());

    switch(type) {
        case identy::Entry_32bit: {
            if(entry_point_buffer.size() >= sizeof(identy::SMBIOS_EntryPoint32)) {
                identy::SMBIOS_EntryPoint32 entry;
                std::memcpy(&entry, entry_point_buffer.data(), sizeof(entry));
                result.major_version = entry.major_version;
                result.minor_version = entry.minor_version;
            }
            break;
        }

        case identy::Entry_64bit: {
            if(entry_point_buffer.size() >= sizeof(identy::SMBIOS_EntryPoint64)) {
                identy::SMBIOS_EntryPoint64 entry;
                std::memcpy(&entry, entry_point_buffer.data(), sizeof(entry));
                result.major_version = entry.major_version;
                result.minor_version = entry.minor_version;
            }
            break;
        }

        default:
            break;
    }
}

identy::SMBIOS_RawData get_smbios_linux()
{
    std::ifstream file("/sys/firmware/dmi/tables/DMI", std::ios::binary | std::ios::ate);

    if(!file.is_open()) {
        return {};
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    identy::SMBIOS_RawData result;
    result.table_data.resize(static_cast<std::size_t>(size));

    file.read(reinterpret_cast<char*>(result.table_data.data()), size);

    // Read entry point for version info
    std::ifstream entry_file("/sys/firmware/dmi/tables/smbios_entry_point", std::ios::binary | std::ios::ate);
    if(entry_file.is_open()) {
        auto entry_size = entry_file.tellg();
        entry_file.seekg(0);

        std::vector<identy::byte> entry_buffer(static_cast<std::size_t>(entry_size));
        entry_file.read(reinterpret_cast<char*>(entry_buffer.data()), entry_size);

        read_smbios_versions(result, entry_buffer);
    }

    return result;
}

std::string read_sysfs_value(const std::filesystem::path& path)
{
    namespace fs = std::filesystem;

    if(!fs::exists(path))
        return "";

    std::ifstream file(path);
    std::string value;
    std::getline(file, value);

    value = identy::strings::trim_whitespace(value);

    return value;
}

std::vector<identy::PhysicalDriveInfo> list_drives_linux()
{
    std::vector<identy::PhysicalDriveInfo> drive_infos;

    for(const auto& entry : std::filesystem::directory_iterator("/sys/block")) {
        auto device = entry.path().filename().string();

        if(device.starts_with("loop") || device.starts_with("ram") || device.starts_with("dm-")) {
            continue;
        }

        identy::PhysicalDriveInfo info;

        if(device.starts_with("nvme")) {
            info.bus_type = identy::PhysicalDriveInfo::NMVe;

            info.serial = read_sysfs_value(entry.path() / "serial");
        }
        else if(device.starts_with("sd")) {
            auto subsystem_path = entry.path() / "device" / "subsystem";

            if(std::filesystem::exists(subsystem_path)) {
                auto target = std::filesystem::read_symlink(subsystem_path);

                auto subsystem = target.filename();

                if(subsystem == "scsi" || subsystem == "ata") {
                    info.bus_type = identy::PhysicalDriveInfo::SATA;
                }
                else if(subsystem == "usb") {
                    info.bus_type = identy::PhysicalDriveInfo::USB;
                }
                else {
                    info.bus_type = identy::PhysicalDriveInfo::Other;
                }
            }
            else {
                info.bus_type = identy::PhysicalDriveInfo::Other;
            }

            info.serial = read_sysfs_value(entry.path() / "device" / "serial");

            if(info.serial.empty()) {
                info.serial = read_sysfs_value(entry.path() / "device" / "vpd_pg80");
            }
        }
        else {
            continue;
        }

        drive_infos.push_back(info);
    }

    return drive_infos;
}

} // namespace

namespace identy::platform
{

SMBIOS_RawData get_smbios()
{
    return get_smbios_linux();
}

std::vector<PhysicalDriveInfo> list_drives()
{
    return list_drives_linux();
}

} // namespace identy::platform

#endif // IDENTY_LINUX
