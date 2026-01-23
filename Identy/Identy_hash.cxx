#include "Identy_pch.hxx"

#include "Identy_hash.hxx"
#include "Identy_sha256.hxx"

namespace
{
/**
 * @brief Helper to update hash with raw bytes of a trivially copyable value
 */
template<typename T>
void hash_value(identy::hs::detail::Sha256& ctx, const T& value) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    static_assert(std::has_unique_object_representations_v<T>, "T MUST have unique object representations");

    ctx.update(reinterpret_cast<const identy::byte*>(&value), sizeof(T));
}

/**
 * @brief Helper to update hash with string data
 */
void hash_string(identy::hs::detail::Sha256& ctx, const std::string& str) noexcept
{
    ctx.update(reinterpret_cast<const identy::byte*>(str.data()), str.size());
}

/**
 * @brief Helper to update hash with raw byte array
 */
void hash_bytes(identy::hs::detail::Sha256& ctx, const identy::byte* data, std::size_t size) noexcept
{
    ctx.update(data, size);
}

/**
 * @brief Updates hash context with Motherboard data
 *
 * @param ctx The SHA256 context to update
 * @param board The motherboard information to hash
 */
void hash_motherboard(identy::hs::detail::Sha256& ctx, const identy::Motherboard& board) noexcept
{
    // Hash CPU vendor string
    hash_string(ctx, board.cpu.vendor);

    // Hash CPU version (4 bytes)
    hash_value(ctx, board.cpu.version);

    // Hash CPU characteristics
    hash_value(ctx, board.cpu.brand_index);
    hash_value(ctx, board.cpu.clflush_line_size);
    hash_value(ctx, board.cpu.logical_processors_count);

    // Hash extended brand string
    hash_string(ctx, board.cpu.extended_brand_string);

    // Hash instruction sets
    hash_value(ctx, board.cpu.instruction_set.basic);
    hash_value(ctx, board.cpu.instruction_set.modern);
    hash_value(ctx, board.cpu.instruction_set.extended_modern);

    // Hash SMBIOS data
    identy::byte is_20_flag = board.smbios.is_20_calling_used ? 1 : 0;
    hash_value(ctx, is_20_flag);
    hash_value(ctx, board.smbios.major_version);
    hash_value(ctx, board.smbios.minor_version);
    hash_value(ctx, board.smbios.dmi_version);

    // Hash UUID
    hash_bytes(ctx, board.smbios.uuid, identy::SMBIOS_uuid_length);
}

/**
 * @brief Updates hash context with MotherboardEx data
 *
 * @param ctx The SHA256 context to update
 * @param board The extended motherboard information to hash
 */
void hash_motherboard_ex(identy::hs::detail::Sha256& ctx, const identy::MotherboardEx& board) noexcept
{
    // Hash base motherboard data
    identy::Motherboard base_board;
    base_board.cpu = board.cpu;
    base_board.smbios = board.smbios;
    hash_motherboard(ctx, base_board);

    // Hash drive information
    for(const auto& drive : board.drives) {
        if(drive.bus_type == identy::PhysicalDriveInfo::USB || drive.bus_type == identy::PhysicalDriveInfo::Other) {
            continue;
        }

        // Hash bus type
        hash_value(ctx, drive.bus_type);

        // Hash device name
        hash_string(ctx, drive.device_name);

        // Hash serial number
        hash_string(ctx, drive.serial);
    }
}
} // anonymous namespace

identy::hs::Hash256 identy::hs::detail::default_hash(const Motherboard& board)
{
    Sha256 ctx;
    hash_motherboard(ctx, board);
    return ctx.finalize();
}

identy::hs::Hash256 identy::hs::detail::default_hash_ex(const MotherboardEx& board)
{
    Sha256 ctx;
    hash_motherboard_ex(ctx, board);
    return ctx.finalize();
}
