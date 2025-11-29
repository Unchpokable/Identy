#pragma once

#ifndef UNC_IDENTY_SHA256_H
#define UNC_IDENTY_SHA256_H

#include "Identy_hash_base.hxx"
#include "Identy_types.hxx"

namespace identy::hs
{
class Sha256 final
{
public:
    static Hash256 hash(const std::vector<identy::byte>& bytes);

    Sha256();

    void update(const identy::byte* data, std::size_t len);

    Hash256 finalize();

private:
    std::uint32_t m_state[8];
    identy::byte m_data[64];
    std::uint32_t m_data_len;
    std::uint64_t m_bit_len;

    void init();
    void transform(const identy::byte* data);

    static constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n)
    {
        return std::rotr(x, n);
    }

    static constexpr std::uint32_t choose(std::uint32_t e, std::uint32_t f, std::uint32_t g)
    {
        return (e & f) ^ (~e & g);
    }

    static constexpr std::uint32_t majority(std::uint32_t a, std::uint32_t b, std::uint32_t c)
    {
        return (a & (b | c)) | (b & c);
    }

    static constexpr std::uint32_t sig0(std::uint32_t x)
    {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    static constexpr std::uint32_t sig1(std::uint32_t x)
    {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }
};
}

#endif