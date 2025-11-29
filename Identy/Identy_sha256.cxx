#include "Identy_pch.hxx"

#include "Identy_sha256.hxx"

identy::hs::Hash256 identy::hs::Sha256::hash(const std::vector<identy::byte>& bytes)
{
    Sha256 ctx;
    ctx.update(bytes.data(), bytes.size());
    return ctx.finalize();
}

identy::hs::Sha256::Sha256()
{
    init();
}

void identy::hs::Sha256::update(const identy::byte* data, std::size_t len)
{
    for(size_t i = 0; i < len; ++i) {
        m_data[m_data_len] = data[i];
        m_data_len++;
        if(m_data_len == 64) {
            transform(m_data);
            m_bit_len += 512;
            m_data_len = 0;
        }
    }
}

identy::hs::Hash256 identy::hs::Sha256::finalize()
{
    Hash256 result;

    std::uint32_t i = m_data_len;

    // Append 1 bit (0x80)
    m_data[i++] = 0x80;

    if(i > 56) {
        while(i < 64) {
            m_data[i++] = 0x00;
        }
        transform(m_data);
        i = 0;
    }

    while(i < 56) {
        m_data[i++] = 0x00;
    }

    m_bit_len += static_cast<std::uint64_t>(m_data_len) * 8;

    m_data[63] = m_bit_len;
    m_data[62] = m_bit_len >> 8;
    m_data[61] = m_bit_len >> 16;
    m_data[60] = m_bit_len >> 24;
    m_data[59] = m_bit_len >> 32;
    m_data[58] = m_bit_len >> 40;
    m_data[57] = m_bit_len >> 48;
    m_data[56] = m_bit_len >> 56;

    transform(m_data);

    for(std::uint8_t j = 0; j < 4; ++j) {
        std::uint32_t val = m_state[0];
        result.buffer[0 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[1];
        result.buffer[4 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[2];
        result.buffer[8 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[3];
        result.buffer[12 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[4];
        result.buffer[16 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[5];
        result.buffer[20 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[6];
        result.buffer[24 + j] = (val >> (24 - j * 8)) & 0x000000ff;

        val = m_state[7];
        result.buffer[28 + j] = (val >> (24 - j * 8)) & 0x000000ff;
    }

    return result;
}

void identy::hs::Sha256::init()
{
    m_state[0] = 0x6a09e667;
    m_state[1] = 0xbb67ae85;
    m_state[2] = 0x3c6ef372;
    m_state[3] = 0xa54ff53a;
    m_state[4] = 0x510e527f;
    m_state[5] = 0x9b05688c;
    m_state[6] = 0x1f83d9ab;
    m_state[7] = 0x5be0cd19;
    m_data_len = 0;
    m_bit_len = 0;
}

void identy::hs::Sha256::transform(const identy::byte* data)
{
    std::uint32_t m[64];
    std::uint32_t i, j;

    for(i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    }

    for(; i < 64; ++i) {
        m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
    }

    std::uint32_t a = m_state[0];
    std::uint32_t b = m_state[1];
    std::uint32_t c = m_state[2];
    std::uint32_t d = m_state[3];
    std::uint32_t e = m_state[4];
    std::uint32_t f = m_state[5];
    std::uint32_t g = m_state[6];
    std::uint32_t h = m_state[7];

    static const std::uint32_t k[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6,
        0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1,
        0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2 };

    for(i = 0; i < 64; ++i) {
        std::uint32_t t1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + choose(e, f, g) + k[i] + m[i];
        std::uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}
