#pragma once

#ifndef UNC_IDENTY_IO_H
#define UNC_IDENTY_IO_H

#include "Identy_hash.hxx"

namespace identy
{
struct Motherboard;
struct MotherboardEx;
} // namespace identy

namespace identy::io
{
void write_text(std::ostream& stream, const identy::Motherboard& mb);
void write_text(std::ostream& stream, const identy::MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
void write_binary(std::ostream& stream, const identy::Motherboard& mb);
void write_binary(std::ostream& stream, const identy::MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
void write_hash(std::ostream& stream, const identy::Motherboard& mb);
void write_hash(std::ostream& stream, const identy::MotherboardEx& mb);
} // namespace identy::io

namespace identy::io
{
template<identy::hs::IdentyHashCompatible Hash>
void write_hash(std::ostream& stream, Hash&& hash);
} // namespace identy::io

template<identy::hs::IdentyHashCompatible Hash>
void identy::io::write_hash(std::ostream& stream, Hash&& hash)
{
    if(!stream.good()) {
        return; // todo: throw exception?
    }

    auto currp = stream.tellp();

    stream.seekp(std::ios::end);
    auto endp = stream.tellp();

    auto space = endp - currp;

    stream.seekp(currp);

    if(space < sizeof(hash.buffer)) {
        return; // todo: throw exception?
    }

    stream.write(reinterpret_cast<const char*>(hash.buffer), sizeof(hash.buffer));
}

#endif
