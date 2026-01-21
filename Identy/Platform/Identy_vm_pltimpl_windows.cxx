#ifdef IDENTY_WIN32

#include "../Identy_pch.hxx"

#define NOMINMAX
#include <Windows.h>
#include <iphlpapi.h>

#include "Identy_platform_vm.hxx"

namespace
{

std::vector<identy::platform::NetworkAdapterInfo> list_network_adapters_win32(bool& access_denied)
{
    access_denied = false;

    ULONG buffer_size = 0;
    GetAdaptersInfo(nullptr, &buffer_size);

    std::vector<std::uint8_t> buffer;
    buffer.resize(buffer_size);

    auto adapter_info = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

    if(GetAdaptersInfo(adapter_info, &buffer_size) != NO_ERROR) {
        access_denied = true;
        return {};
    }

    std::vector<identy::platform::NetworkAdapterInfo> adapters;

    for(auto adapter = adapter_info; adapter != nullptr; adapter = adapter->Next) {
        identy::platform::NetworkAdapterInfo info;
        info.description = adapter->Description;
        info.is_loopback = (adapter->Type == MIB_IF_TYPE_LOOPBACK);
        info.is_tunnel = (adapter->Type == IF_TYPE_TUNNEL);

        adapters.push_back(std::move(info));
    }

    return adapters;
}

} // namespace

namespace identy::platform
{

std::vector<NetworkAdapterInfo> list_network_adapters(bool& access_denied)
{
    return list_network_adapters_win32(access_denied);
}

} // namespace identy::platform

#endif // IDENTY_WIN32
