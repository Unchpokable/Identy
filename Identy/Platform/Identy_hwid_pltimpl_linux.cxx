#ifdef IDENTY_LINUX

#include "../Identy_pch.hxx"

#include "../Identy_strings.hxx"

#include "Identy_platform_hwid.hxx"

namespace
{
std::optional<std::array<identy::byte, 16>> get_smbios_uuid_sysfs(std::string_view uuid_str)
{
    if(uuid_str.size() < 32) {
        return std::nullopt;
    }

    std::array<identy::byte, 16> uuid;

    std::size_t byte_index { 0 };

    for(std::size_t i { 0 }; i < uuid_str.size() && byte_index < 16; ++i) {
        if(uuid_str[i] == '-') {
            continue;
        }

        if(i + 1 >= uuid_str.size()) {
            break;
        }

        char byte_hex[3] = { uuid_str[i], uuid_str[i + 1], 0 };

        identy::byte value { 0 };

        auto [ptr, ec] = std::from_chars(byte_hex, byte_hex + 2, value, 16);

        if(ec == std::errc()) {
            uuid[byte_index++] = value;
            i++;
        }
        {
            return std::nullopt;
        }
    }

    if(byte_index != 16) {
        return std::nullopt;
    }

    return uuid;
}
} // namespace

namespace
{
void try_read_sysfs_uuid(identy::SMBIOS_RawData& result)
{
    const std::filesystem::path dmi_id_path = "/sys/class/dmi/id";

    if(!std::filesystem::exists(dmi_id_path)) {
        return;
    }

    std::ifstream uuid_file(dmi_id_path / "product_uuid");

    std::string uuid_string;

    if(uuid_file >> uuid_string) {
        auto trimmed = identy::strings::trim_whitespace(uuid_string);
        result.fallback_uid = get_smbios_uuid_sysfs(trimmed);
    }

    std::ifstream version_file(dmi_id_path / "smbios_version");

    std::string version_string;

    if(version_file >> version_string) {
        size_t dot_pos = version_string.find('.');
        if(dot_pos != std::string::npos) {
            std::from_chars(version_string.data(), version_string.data() + dot_pos, result.major_version);
            std::from_chars(version_string.data() + dot_pos + 1, version_string.data() + version_string.size(), result.minor_version);
        }
    }
}
} // namespace

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
    identy::SMBIOS_RawData result;

    if(file.is_open()) {
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

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
    }
    else {
        try_read_sysfs_uuid(result);
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
