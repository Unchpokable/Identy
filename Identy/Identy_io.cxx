#include "Identy_pch.hxx"

#include "Identy_io.hxx"

#include "Identy_hwid.hxx"

void identy::io::write_text(std::ostream& stream, const identy::Motherboard& mb)
{
    if(!stream.good()) {
        return; // todo: throw error
    }

    stream << "CPU:\n";
    stream << mb.cpu.extended_brand_string;
    stream << " Vendor: " << mb.cpu.vendor << "\n";
    stream << " Cores: " << mb.cpu.logical_processors_count << "\n";
    stream << " Hypervisor present: " << std::format("{}", mb.cpu.hypervisor_bit) << "\n";
    stream << " Hypervisor signature (if presented)" << mb.cpu.hypervisor_signature << "\n";

    stream << "Motherboard:\n";
    stream << " SMBIOS UUID: ";
    stream.write(reinterpret_cast<const char*>(mb.smbios.uuid), sizeof(mb.smbios.uuid));
    stream << "\n";

    stream << " SMBIOS Ver: ";
    stream << std::format("{}.{}\n", mb.smbios.major_version, mb.smbios.minor_version);

    stream << " SMBIOS DMI Ver: ";
    stream << std::format("{}\n", mb.smbios.dmi_version);

    stream << " SMBIOS 2.0 calling conversion: " << std::format("{}\n", mb.smbios.is_20_calling_used);
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
