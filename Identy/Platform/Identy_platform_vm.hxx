#pragma once

#ifndef UNC_IDENTY_PLATFORM_VM_H
#define UNC_IDENTY_PLATFORM_VM_H

#include <string>
#include <vector>

namespace identy::platform
{

/**
 * @brief Basic information about a network adapter
 */
struct NetworkAdapterInfo
{
    std::string description;
    bool is_loopback { false };
    bool is_tunnel { false };
};

/**
 * @brief Platform-specific network adapter enumeration
 * @return Vector of network adapter information, or empty vector on failure
 *         If the OS denied access to network devices, returns empty vector
 *         and sets the out parameter to true
 */
std::vector<NetworkAdapterInfo> list_network_adapters(bool& access_denied);

} // namespace identy::platform

#endif
