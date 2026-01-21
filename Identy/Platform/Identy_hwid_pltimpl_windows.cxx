#ifdef IDENTY_WIN32

#include "../Identy_pch.hxx"

#include "../Identy_strings.hxx"
#include "../Identy_types.hxx"

#include "../Identy_nvme_support.hxx"

#include "Identy_platform_hwid.hxx"

namespace identy
{
// StorageAdapterProtocolSpecificProperty = 49 (not defined in older Windows SDKs)
constexpr STORAGE_PROPERTY_ID StorageAdapterProtocolSpecificProperty = static_cast<STORAGE_PROPERTY_ID>(49);
} // namespace identy

namespace
{

// Offsets within Windows RSMB firmware table header
constexpr std::size_t RSMB_used_20_offset = 0;
constexpr std::size_t RSMB_major_version_offset = 1;
constexpr std::size_t RSMB_minor_version_offset = 2;
constexpr std::size_t RSMB_dmi_revision_offset = 3;
constexpr std::size_t RSMB_length_offset = 4;
constexpr std::size_t RSMB_table_data_offset = 8;

identy::SMBIOS_RawData get_smbios_win32()
{
    identy::dword size = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
    if(size == 0) {
        return {};
    }

    std::vector<identy::byte> buffer(size);
    GetSystemFirmwareTable('RSMB', 0, buffer.data(), size);

    identy::SMBIOS_RawData result;

    if(buffer.size() >= RSMB_table_data_offset) {
        result.used_20_calling_method = buffer[RSMB_used_20_offset];
        result.major_version = buffer[RSMB_major_version_offset];
        result.minor_version = buffer[RSMB_minor_version_offset];
        result.dmi_revision = buffer[RSMB_dmi_revision_offset];

        identy::dword table_length = 0;
        std::memcpy(&table_length, buffer.data() + RSMB_length_offset, sizeof(table_length));

        if(RSMB_table_data_offset + table_length <= buffer.size()) {
            result.table_data.assign(
                buffer.begin() + RSMB_table_data_offset, buffer.begin() + RSMB_table_data_offset + table_length);
        }
    }

    return result;
}

std::string get_nvme_serial(HANDLE h_device)
{
    // Build query structure on stack (small, safe)
    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = identy::StorageAdapterProtocolSpecificProperty;
    query.QueryType = PropertyStandardQuery;

    identy::nvme::StorageProtocolSpecificData protocol_data = {};
    protocol_data.ProtocolType = ProtocolTypeNvme;
    protocol_data.DataType = identy::nvme::NVMeDataTypeIdentify;
    protocol_data.ProtocolDataRequestValue = identy::nvme::CNS_CONTROLLER;
    protocol_data.ProtocolDataRequestSubValue = 0;
    protocol_data.ProtocolDataOffset = sizeof(identy::nvme::StorageProtocolSpecificData);
    protocol_data.ProtocolDataLength = sizeof(identy::nvme::NvmeIdentifyControllerData);

    // Build input buffer: query + protocol_data
    constexpr auto input_size = sizeof(STORAGE_PROPERTY_QUERY) + sizeof(identy::nvme::StorageProtocolSpecificData);
    std::vector<identy::byte> input_buffer(input_size);
    std::memcpy(input_buffer.data(), &query, sizeof(query));
    std::memcpy(input_buffer.data() + offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters), &protocol_data, sizeof(protocol_data));

    // Output buffer for descriptor + NVMe data (4KB+ on heap)
    constexpr auto output_size = sizeof(identy::nvme::StorageProtocolDataDescriptor) + sizeof(identy::nvme::NvmeIdentifyControllerData);
    std::vector<identy::byte> output_buffer(output_size);

    identy::dword bytes_returned = 0;
    auto result = DeviceIoControl(h_device, IOCTL_STORAGE_QUERY_PROPERTY, input_buffer.data(),
        static_cast<identy::dword>(input_buffer.size()), output_buffer.data(), static_cast<identy::dword>(output_buffer.size()),
        &bytes_returned, nullptr);

    if(!result) {
        return {};
    }

    // Parse descriptor header via memcpy
    identy::nvme::StorageProtocolDataDescriptor descriptor = {};
    std::memcpy(&descriptor, output_buffer.data(), sizeof(descriptor));

    auto data_offset = sizeof(identy::nvme::StorageProtocolDataDescriptor) - sizeof(identy::nvme::StorageProtocolSpecificData)
        + descriptor.ProtocolSpecificData.ProtocolDataOffset;

    if(data_offset + sizeof(identy::nvme::NvmeIdentifyControllerData) > output_buffer.size()) {
        return {};
    }

    // Extract only the serial number field (20 bytes) - no need to copy entire 4KB structure
    constexpr std::size_t sn_offset_in_nvme_data = offsetof(identy::nvme::NvmeIdentifyControllerData, SN);
    constexpr std::size_t sn_size = sizeof(identy::nvme::NvmeIdentifyControllerData::SN);

    char serial_buffer[sn_size + 1] = {};
    std::memcpy(serial_buffer, output_buffer.data() + data_offset + sn_offset_in_nvme_data, sn_size);

    return std::string(serial_buffer, sn_size);
}

std::optional<identy::PhysicalDriveInfo> get_drive_info(std::string_view drive_name)
{
    auto path = std::format(R"(\\.\{})", drive_name);

    HANDLE h_device = CreateFileA(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if(h_device == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    identy::PhysicalDriveInfo info;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    std::vector<identy::byte> buffer(1024);
    identy::dword bytes_returned = 0;

    if(!DeviceIoControl(
           h_device, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer.data(), static_cast<identy::dword>(buffer.size()), &bytes_returned, nullptr)) {
        CloseHandle(h_device);
        return std::nullopt;
    }

    // Parse STORAGE_DEVICE_DESCRIPTOR header via memcpy to avoid strict aliasing
    STORAGE_DEVICE_DESCRIPTOR desc = {};
    std::memcpy(&desc, buffer.data(), std::min(sizeof(desc), buffer.size()));

    switch(desc.BusType) {
        case BusTypeNvme:
            info.bus_type = identy::PhysicalDriveInfo::NMVe;
            break;

        case BusTypeSata:
            info.bus_type = identy::PhysicalDriveInfo::SATA;
            break;

        case BusTypeUsb:
            info.bus_type = identy::PhysicalDriveInfo::USB;
            break;

        default:
            info.bus_type = identy::PhysicalDriveInfo::Other;
            break;
    }

    if(info.bus_type == identy::PhysicalDriveInfo::NMVe) {
        info.serial = get_nvme_serial(h_device);
    }
    else if(desc.SerialNumberOffset != 0 && desc.SerialNumberOffset < buffer.size()) {
        // Serial number is null-terminated string at offset
        const char* serial_ptr = reinterpret_cast<const char*>(buffer.data() + desc.SerialNumberOffset);
        info.serial = std::string(serial_ptr);
    }

    info.serial = std::string(identy::strings::trim_whitespace(info.serial));

    info.device_name = drive_name;

    CloseHandle(h_device);

    return info;
}

std::vector<identy::PhysicalDriveInfo> list_drives_win32()
{
    constexpr identy::dword buffer_size = 65536;
    std::vector<char> buffer(buffer_size);

    identy::dword count = QueryDosDeviceA(nullptr, buffer.data(), buffer_size);
    if(count == 0) {
        return {};
    }

    std::vector<std::string> drives;

    const char* current_info = buffer.data();
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

} // namespace

namespace identy::platform
{

SMBIOS_RawData get_smbios()
{
    return get_smbios_win32();
}

std::vector<PhysicalDriveInfo> list_drives()
{
    return list_drives_win32();
}

} // namespace identy::platform

#endif // IDENTY_WIN32
