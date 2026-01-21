#ifdef IDENTY_LINUX

#include "../Identy_pch.hxx"

#include "Identy_platform_vm.hxx"

namespace
{

std::string read_sysfs_file(const std::filesystem::path& path)
{
    if(!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream file(path);
    std::string value;
    std::getline(file, value);

    return value;
}

std::vector<identy::platform::NetworkAdapterInfo> list_network_adapters_linux(bool& access_denied)
{
    access_denied = false;

    const std::filesystem::path net_path = "/sys/class/net";

    if(!std::filesystem::exists(net_path)) {
        access_denied = true;
        return {};
    }

    std::vector<identy::platform::NetworkAdapterInfo> adapters;

    for(const auto& entry : std::filesystem::directory_iterator(net_path)) {
        auto iface_name = entry.path().filename().string();

        identy::platform::NetworkAdapterInfo info;

        // Read device type from uevent or use interface name heuristics
        auto uevent_path = entry.path() / "device" / "uevent";
        auto driver_path = entry.path() / "device" / "driver";

        // Check if it's a loopback interface
        info.is_loopback = (iface_name == "lo");

        // Check for tunnel interfaces
        auto type_path = entry.path() / "type";
        auto type_str = read_sysfs_file(type_path);
        if(!type_str.empty()) {
            int type = std::stoi(type_str);
            // ARPHRD_TUNNEL = 768, ARPHRD_TUNNEL6 = 769, ARPHRD_SIT = 776, ARPHRD_IPGRE = 778
            info.is_tunnel = (type == 768 || type == 769 || type == 776 || type == 778);
        }

        // Try to get description from driver or device info
        if(std::filesystem::exists(driver_path)) {
            auto driver_target = std::filesystem::read_symlink(driver_path);
            info.description = driver_target.filename().string();
        }

        // If no driver info, use interface name as description
        if(info.description.empty()) {
            info.description = iface_name;
        }

        adapters.push_back(std::move(info));
    }

    return adapters;
}

} // namespace

namespace identy::platform
{

std::vector<NetworkAdapterInfo> list_network_adapters(bool& access_denied)
{
    return list_network_adapters_linux(access_denied);
}

} // namespace identy::platform

#endif // IDENTY_LINUX
