#include "Identy_pch.hxx"

#include "Identy_sha256.hxx"

identy::hs::Hash256 identy::hs::detail::Sha256::hash(std::span<const byte> data) noexcept
{
    Sha256 ctx;
    ctx.update(data);
    return ctx.finalize();
}

identy::hs::detail::Sha256::Sha256() noexcept
{
    reset();
}

void identy::hs::detail::Sha256::reset() noexcept
{
    std::memcpy(m_state, k_initial_hash, sizeof(m_state));
    m_block_len = 0;
    m_total_len = 0;
    m_finalized = false;
}

void identy::hs::detail::Sha256::update(std::span<const byte> data) noexcept
{
    update(data.data(), data.size());
}

void identy::hs::detail::Sha256::update(const byte* data, std::size_t len) noexcept
{
    assert(!m_finalized && "Cannot update after finalize()");

    if(data == nullptr || len == 0) {
        return;
    }

    m_total_len += len;

    // If we have leftover data in block, try to fill it first
    if(m_block_len > 0) {
        std::size_t space_in_block = block_size - m_block_len;
        std::size_t to_copy = (len < space_in_block) ? len : space_in_block;

        std::memcpy(m_block + m_block_len, data, to_copy);
        m_block_len += to_copy;
        data += to_copy;
        len -= to_copy;

        if(m_block_len == block_size) {
            transform(m_block);
            m_block_len = 0;
        }
    }

    // Process full blocks directly from input
    while(len >= block_size) {
        transform(data);
        data += block_size;
        len -= block_size;
    }

    // Store remaining bytes
    if(len > 0) {
        std::memcpy(m_block, data, len);
        m_block_len = len;
    }
}

identy::hs::Hash256 identy::hs::detail::Sha256::finalize() noexcept
{
    assert(!m_finalized && "finalize() called twice");
    m_finalized = true;

    Hash256 result;

    // Calculate bit length before padding
    std::uint64_t bit_len = m_total_len * 8;

    // Append padding bit (0x80)
    m_block[m_block_len++] = 0x80;

    // If not enough space for length (need 8 bytes), pad current block and process
    if(m_block_len > 56) {
        std::memset(m_block + m_block_len, 0, block_size - m_block_len);
        transform(m_block);
        m_block_len = 0;
    }

    // Pad with zeros until 56 bytes
    std::memset(m_block + m_block_len, 0, 56 - m_block_len);

    // Append length in big-endian (last 8 bytes)
    m_block[56] = static_cast<byte>(bit_len >> 56);
    m_block[57] = static_cast<byte>(bit_len >> 48);
    m_block[58] = static_cast<byte>(bit_len >> 40);
    m_block[59] = static_cast<byte>(bit_len >> 32);
    m_block[60] = static_cast<byte>(bit_len >> 24);
    m_block[61] = static_cast<byte>(bit_len >> 16);
    m_block[62] = static_cast<byte>(bit_len >> 8);
    m_block[63] = static_cast<byte>(bit_len);

    transform(m_block);

    // Convert state to big-endian output
    for(std::size_t i = 0; i < 8; ++i) {
        std::uint32_t val = m_state[i];
        result.buffer[i * 4 + 0] = static_cast<byte>(val >> 24);
        result.buffer[i * 4 + 1] = static_cast<byte>(val >> 16);
        result.buffer[i * 4 + 2] = static_cast<byte>(val >> 8);
        result.buffer[i * 4 + 3] = static_cast<byte>(val);
    }

    return result;
}

void identy::hs::detail::Sha256::transform(const byte* block) noexcept
{
    std::uint32_t w[64];

    // Prepare message schedule (first 16 words from block, big-endian)
    for(std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16)
            | (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) | (static_cast<std::uint32_t>(block[i * 4 + 3]));
    }

    // Extend message schedule
    for(std::size_t i = 16; i < 64; ++i) {
        w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
    }

    // Initialize working variables
    std::uint32_t a = m_state[0];
    std::uint32_t b = m_state[1];
    std::uint32_t c = m_state[2];
    std::uint32_t d = m_state[3];
    std::uint32_t e = m_state[4];
    std::uint32_t f = m_state[5];
    std::uint32_t g = m_state[6];
    std::uint32_t h = m_state[7];

    // Main compression loop
    for(std::size_t i = 0; i < 64; ++i) {
        std::uint32_t t1 = h + sigma1(e) + choose(e, f, g) + k_round_constants[i] + w[i];
        std::uint32_t t2 = sigma0(a) + majority(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}
