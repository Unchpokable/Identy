#include "Identy_pch.hxx"

#include "Identy_io.hxx"

#include "Identy_hwid.hxx"

void identy::io::write_text(std::ostream& stream, const identy::Motherboard& mb)
{
}

void identy::io::write_text(std::ostream& stream, const identy::MotherboardEx& mb)
{
}

void identy::io::write_binary(std::ostream& stream, const identy::Motherboard& mb)
{
}

void identy::io::write_binary(std::ostream& stream, const identy::MotherboardEx& mb)
{
}

void identy::io::write_hash(std::ostream& stream, const identy::Motherboard& mb)
{
    write_hash(stream, identy::hs::hash(mb));
}

void identy::io::write_hash(std::ostream& stream, const identy::MotherboardEx& mb)
{
    write_hash(stream, identy::hs::hash(mb));
}
