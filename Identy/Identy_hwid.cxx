#include "Identy_pch.hxx"

#include "Identy_hwid.hxx"

#include "Identy_platform.hxx"

#ifdef IDENTY_WIN32
#include <Windows.h>
#endif

namespace
{
constexpr std::uint32_t SMBIOS_type_system_information = 1;
constexpr std::uint32_t SMBIOS_system_information_header_length = 24;
constexpr std::ptrdiff_t SMBIOS_uuid_offset = 8;
constexpr std::ptrdiff_t SMBIOS_uuid_length = 16;
} // namespace

namespace
{
constexpr identy::register_32 cpuleaf_vendorID = 0x00000000;
constexpr identy::register_32 cpuleaf_family = 0x00000001;
constexpr identy::register_32 cpuleaf_ext_instructions = 0x00000007;
constexpr identy::register_32 cpuleaf_ext_brand = 0x80000002;
} // namespace

namespace
{
#ifdef IDENTY_WIN32
identy::SMBIOS::Ptr get_smbios_win32()
{
    identy::dword size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if(size == 0) {
        return identy::SMBIOS::Ptr();
    }

    identy::SMBIOS::Ptr smbios_ptr;
    smbios_ptr.allocate(size);

    GetSystemFirmwareTable('RSMB', 0, smbios_ptr, smbios_ptr.bytes_length());
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

std::vector<std::uint8_t> get_smbios_uuid_impl(identy::SMBIOS::Ptr& smbios)
{
    identy::byte* smbios_begin = smbios->SMBIOS_table_data;
    identy::byte* smbios_end = smbios_begin + smbios->length;

    std::vector<std::uint8_t> buffer;
    buffer.resize(static_cast<std::ptrdiff_t>(smbios_end - smbios_begin));

    while(smbios_begin < smbios_end) {
        identy::SMBIOS_Header* header = (identy::SMBIOS_Header*)smbios_begin;

        if(header->type == SMBIOS_type_system_information && header->length >= SMBIOS_system_information_header_length) {
            identy::byte* uuid = smbios_begin + SMBIOS_uuid_offset;

            for(std::size_t i { 0 }; i < SMBIOS_uuid_length; ++i) {
                buffer.emplace_back(uuid[i]);
            }

            return buffer;
        }

        identy::byte* next = smbios_begin + header->length;
        while(next < smbios_end && (*next != 0 || *(next + 1) != 0)) {
            next++;
        }

        smbios_begin = next + 2;
    }

    return {};
}

} // namespace

identy::SMBIOS::Ptr identy::get_smbios()
{
    return get_smbios_impl();
}

std::vector<std::uint8_t> identy::get_smbios_uuid()
{
    auto smbios = get_smbios_impl();
    if(smbios.data() == nullptr) {
        return {};
    }

    return get_smbios_uuid_impl(smbios);
}
