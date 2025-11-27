#include "pch.hxx"

#include "platform.hxx"

#include "Identy_hwid.hxx"

#ifdef IDENTY_WIN32

#include <Windows.h>

namespace
{
constexpr identy::SMBIOS operator""_smbios(const char* str, std::size_t size)
{
    return { 0 };
}
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
identy::SMBIOS get_smbios_win32()
{
    identy::identy_dword size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
    if(size == 0) {
        return "ERROR"_smbios;
    }

    std::vector<identy::identy_byte> buffer;
    buffer.resize(size);

    GetSystemFirmwareTable('RSMB', 0, buffer.data(), size);
}

#define get_smbios get_smbios_win32
#else
identy::SMBIOS get_smbios_linux()
{
}

#define get_smbios get_smbios_linux
#endif
} // namespace

#endif
