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

identy::SMBIOS_Raw::Ptr get_smbios_win32()
{
    identy::dword size = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
    if(size == 0) {
        return {};
    }

    identy::SMBIOS_Raw::Ptr smbios_ptr;
    smbios_ptr.allocate(size);

    GetSystemFirmwareTable('RSMB', 0, smbios_ptr, static_cast<identy::dword>(smbios_ptr.bytes_length()));

    return smbios_ptr;
}

std::string get_nvme_serial(HANDLE h_device)
{
    auto buffer_size = sizeof(STORAGE_PROPERTY_QUERY) + sizeof(identy::nvme::StorageProtocolDataDescriptor)
        + sizeof(identy::nvme::NvmeIdentifyControllerData);

    std::vector<identy::byte> buffer;
    buffer.resize(buffer_size);

    auto query = reinterpret_cast<STORAGE_PROPERTY_QUERY*>(buffer.data());
    auto protocol_data = reinterpret_cast<identy::nvme::StorageProtocolSpecificData*>(query->AdditionalParameters);

    query->PropertyId = identy::StorageAdapterProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocol_data->ProtocolType = ProtocolTypeNvme;
    protocol_data->DataType = identy::nvme::NVMeDataTypeIdentify;
    protocol_data->ProtocolDataRequestValue = identy::nvme::CNS_CONTROLLER;
    protocol_data->ProtocolDataRequestSubValue = 0;
    protocol_data->ProtocolDataOffset = sizeof(identy::nvme::StorageProtocolSpecificData);
    protocol_data->ProtocolDataLength = sizeof(identy::nvme::NvmeIdentifyControllerData);

    identy::dword bytes_returned = 0;
    auto result = DeviceIoControl(h_device, IOCTL_STORAGE_QUERY_PROPERTY, buffer.data(), static_cast<identy::dword>(buffer.size()),
        buffer.data(), static_cast<identy::dword>(buffer.size()), &bytes_returned, nullptr);

    if(!result) {
        return {};
    }

    auto descriptor = reinterpret_cast<identy::nvme::StorageProtocolDataDescriptor*>(buffer.data());

    if(descriptor->ProtocolSpecificData.ProtocolDataOffset + sizeof(identy::nvme::NvmeIdentifyControllerData) > buffer.size()) {
        return {};
    }

    auto nvme_data = reinterpret_cast<identy::nvme::NvmeIdentifyControllerData*>(
        reinterpret_cast<identy::byte*>(&descriptor->ProtocolSpecificData) + descriptor->ProtocolSpecificData.ProtocolDataOffset);

    auto serial = std::string(reinterpret_cast<const char*>(nvme_data->SN), sizeof(nvme_data->SN));

    return serial;
}

std::optional<identy::PhysicalDriveInfo> get_drive_info(std::string_view drive_name)
{
    auto path = std::format(R"(\\.\{})", drive_name);

    HANDLE h_device = CreateFileA(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
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

        default:
            info.bus_type = identy::PhysicalDriveInfo::Other;
            break;
    }

    if(info.bus_type == identy::PhysicalDriveInfo::NMVe) {
        info.serial = get_nvme_serial(h_device);
    }
    else if(desc->SerialNumberOffset != 0 && desc->SerialNumberOffset < sizeof(buffer)) {
        info.serial = std::string(reinterpret_cast<const char*>(buffer) + desc->SerialNumberOffset);
    }

    info.serial = std::string(identy::strings::trim_whitespace(info.serial));

    info.device_name = drive_name;

    CloseHandle(h_device);

    return info;
}

std::vector<identy::PhysicalDriveInfo> list_drives_win32()
{
    constexpr identy::dword buffer_size = 65536;
    identy::CStdHandle<char> buffer;
    buffer.allocate(buffer_size);

    identy::dword count = QueryDosDeviceA(nullptr, buffer.data(), buffer_size);
    if(count == 0) {
        return {};
    }

    std::vector<std::string> drives;

    char* current_info = buffer.data();
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

SMBIOS_Raw::Ptr get_smbios()
{
    return get_smbios_win32();
}

std::vector<PhysicalDriveInfo> list_drives()
{
    return list_drives_win32();
}

} // namespace identy::platform

#endif // IDENTY_WIN32
