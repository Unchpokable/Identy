#include "Identy_pch.hxx"

#include "Identy_hash.hxx"
#include "Identy_sha256.hxx"

namespace
{
/**
 * @brief Serializes Motherboard structure into a byte buffer for hashing
 *
 * @param board The motherboard information to serialize
 * @return std::vector<identy::byte> Serialized data ready for hashing
 */
std::vector<identy::byte> serialize_motherboard(const identy::Motherboard& board)
{
    std::vector<identy::byte> buffer;

    // Reserve approximate size to avoid reallocations
    buffer.reserve(1024 + board.smbios.raw_tables_data.size());

    // Serialize CPU vendor string
    buffer.insert(buffer.end(), board.cpu.vendor.begin(), board.cpu.vendor.end());

    // Serialize CPU version (4 bytes)
    const auto* version_ptr = reinterpret_cast<const identy::byte*>(&board.cpu.version);
    buffer.insert(buffer.end(), version_ptr, version_ptr + sizeof(board.cpu.version));

    // Serialize CPU characteristics
    buffer.push_back(board.cpu.brand_index);
    buffer.push_back(board.cpu.clflush_line_size);
    buffer.push_back(board.cpu.apic_id);

    const auto* logic_proc_count_ptr = reinterpret_cast<const identy::byte*>(&board.cpu.logical_processors_count);
    buffer.insert(buffer.end(), logic_proc_count_ptr, logic_proc_count_ptr + sizeof(board.cpu.logical_processors_count));

    // Serialize extended brand string
    buffer.insert(buffer.end(), board.cpu.extended_brand_string.begin(), board.cpu.extended_brand_string.end());

    // Serialize instruction sets
    const auto* basic_ptr = reinterpret_cast<const identy::byte*>(&board.cpu.instruction_set.basic);
    buffer.insert(buffer.end(), basic_ptr, basic_ptr + sizeof(board.cpu.instruction_set.basic));

    const auto* modern_ptr = reinterpret_cast<const identy::byte*>(&board.cpu.instruction_set.modern);
    buffer.insert(buffer.end(), modern_ptr, modern_ptr + sizeof(board.cpu.instruction_set.modern));

    const auto* extended_ptr = reinterpret_cast<const identy::byte*>(&board.cpu.instruction_set.extended_modern);
    buffer.insert(buffer.end(), extended_ptr, extended_ptr + sizeof(board.cpu.instruction_set.extended_modern));

    // Serialize SMBIOS data
    buffer.push_back(board.smbios.is_20_calling_used ? 1 : 0);
    buffer.push_back(board.smbios.major_version);
    buffer.push_back(board.smbios.minor_version);
    buffer.push_back(board.smbios.dmi_version);

    // Serialize UUID
    buffer.insert(buffer.end(), board.smbios.uuid, board.smbios.uuid + identy::SMBIOS_uuid_length);

    // Serialize raw SMBIOS tables
    buffer.insert(buffer.end(), board.smbios.raw_tables_data.begin(), board.smbios.raw_tables_data.end());

    return buffer;
}

/**
 * @brief Serializes MotherboardEx structure into a byte buffer for hashing
 *
 * @param board The extended motherboard information to serialize
 * @return std::vector<identy::byte> Serialized data ready for hashing
 */
std::vector<identy::byte> serialize_motherboard_ex(const identy::MotherboardEx& board)
{
    // Start with base motherboard serialization
    identy::Motherboard base_board;
    base_board.cpu = board.cpu;
    base_board.smbios = board.smbios;

    std::vector<identy::byte> buffer = serialize_motherboard(base_board);

    // Add drive information
    for(const auto& drive : board.drives) {
        // Serialize bus type
        const auto* bus_type_ptr = reinterpret_cast<const identy::byte*>(&drive.bus_type);
        buffer.insert(buffer.end(), bus_type_ptr, bus_type_ptr + sizeof(drive.bus_type));

        // Serialize device name
        buffer.insert(buffer.end(), drive.device_name.begin(), drive.device_name.end());

        // Serialize serial number
        buffer.insert(buffer.end(), drive.serial.begin(), drive.serial.end());
    }

    return buffer;
}
} // anonymous namespace

identy::hs::Hash256 identy::hs::detail::default_hash(const identy::Motherboard& board)
{
    auto serialized_data = serialize_motherboard(board);
    return Sha256::hash(serialized_data);
}

identy::hs::Hash256 identy::hs::detail::default_hash_ex(const identy::MotherboardEx& board)
{
    auto serialized_data = serialize_motherboard_ex(board);
    return Sha256::hash(serialized_data);
}
