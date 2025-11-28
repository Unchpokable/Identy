#include "Identy_pch.hxx"

#include "Identy_io.hxx"

#include "Identy_hwid.hxx"

namespace
{
template<typename MB>
void write_text_common(std::ostream& stream, MB&& mb)
{
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

template<typename MB>
void write_binary_common(std::ostream& stream, MB&& mb)
{
    if(!stream.good()) {
        return;
    }

    std::uint32_t vendor_size = static_cast<std::uint32_t>(mb.cpu.vendor.size());
    stream.write(reinterpret_cast<const char*>(&vendor_size), sizeof(vendor_size));
    stream.write(mb.cpu.vendor.data(), vendor_size);

    stream.write(reinterpret_cast<const char*>(&mb.cpu.version), sizeof(mb.cpu.version));

    stream.write(reinterpret_cast<const char*>(&mb.cpu.hypervisor_bit), sizeof(mb.cpu.hypervisor_bit));

    stream.write(reinterpret_cast<const char*>(&mb.cpu.brand_index), sizeof(mb.cpu.brand_index));
    stream.write(reinterpret_cast<const char*>(&mb.cpu.clflush_line_size), sizeof(mb.cpu.clflush_line_size));
    stream.write(reinterpret_cast<const char*>(&mb.cpu.logical_processors_count), sizeof(mb.cpu.logical_processors_count));
    stream.write(reinterpret_cast<const char*>(&mb.cpu.apic_id), sizeof(mb.cpu.apic_id));

    std::uint32_t brand_size = static_cast<std::uint32_t>(mb.cpu.extended_brand_string.size());
    stream.write(reinterpret_cast<const char*>(&brand_size), sizeof(brand_size));
    stream.write(mb.cpu.extended_brand_string.data(), brand_size);

    std::uint32_t hyperv_size = static_cast<std::uint32_t>(mb.cpu.hypervisor_signature.size());
    stream.write(reinterpret_cast<const char*>(&hyperv_size), sizeof(hyperv_size));
    stream.write(mb.cpu.hypervisor_signature.data(), hyperv_size);

    stream.write(reinterpret_cast<const char*>(&mb.cpu.instruction_set.basic), sizeof(mb.cpu.instruction_set.basic));
    stream.write(reinterpret_cast<const char*>(&mb.cpu.instruction_set.modern), sizeof(mb.cpu.instruction_set.modern));
    stream.write(reinterpret_cast<const char*>(mb.cpu.instruction_set.extended_modern), sizeof(mb.cpu.instruction_set.extended_modern));

    stream.write(reinterpret_cast<const char*>(&mb.smbios.is_20_calling_used), sizeof(mb.smbios.is_20_calling_used));
    stream.write(reinterpret_cast<const char*>(&mb.smbios.major_version), sizeof(mb.smbios.major_version));
    stream.write(reinterpret_cast<const char*>(&mb.smbios.minor_version), sizeof(mb.smbios.minor_version));
    stream.write(reinterpret_cast<const char*>(&mb.smbios.dmi_version), sizeof(mb.smbios.dmi_version));

    stream.write(reinterpret_cast<const char*>(mb.smbios.uuid), sizeof(mb.smbios.uuid));

    std::uint32_t tables_size = static_cast<std::uint32_t>(mb.smbios.raw_tables_data.size());
    stream.write(reinterpret_cast<const char*>(&tables_size), sizeof(tables_size));
    stream.write(reinterpret_cast<const char*>(mb.smbios.raw_tables_data.data()), tables_size);
}
}; // namespace

void identy::io::write_text(std::ostream& stream, const identy::Motherboard& mb)
{
    if(!stream.good()) {
        return; // todo: throw error
    }

    write_text_common(stream, mb);
}

void identy::io::write_text(std::ostream& stream, const identy::MotherboardEx& mb)
{
    if(!stream.good()) {
        return;
    }

    write_text_common(stream, mb);

    stream << "Physical Drives:\n";
    if(mb.drives.empty()) {
        stream << " No drives detected or insufficient permissions\n";
        return;
    }

    for(std::size_t i = 0; i < mb.drives.size(); ++i) {
        const auto& drive = mb.drives[i];

        stream << std::format(" Drive {}\n", i + i);
        stream << "  Device: " << drive.device_name << "\n";
        stream << "  Serial: " << drive.serial << "\n";
        stream << "  Bus Type: ";

        switch(drive.bus_type) {
            case identy::PhysicalDriveInfo::SATA:
                stream << "SATA\n";
                break;
            case identy::PhysicalDriveInfo::NMVe:
                stream << "NVMe\n";
                break;
            case identy::PhysicalDriveInfo::USB:
                stream << "USB\n";
                break;
            default:
                stream << "Unknown\n";
                break;
        }
    }
}

void identy::io::write_binary(std::ostream& stream, const identy::Motherboard& mb)
{
    write_binary_common(stream, mb);
}

void identy::io::write_binary(std::ostream& stream, const identy::MotherboardEx& mb)
{
    if(!stream.good()) {
        return;
    }

    write_binary_common(stream, mb);

    std::uint32_t drives_count = static_cast<std::uint32_t>(mb.drives.size());
    stream.write(reinterpret_cast<const char*>(&drives_count), sizeof(drives_count));

    for(const auto& drive : mb.drives) {
        stream.write(reinterpret_cast<const char*>(&drive.bus_type), sizeof(drive.bus_type));

        std::uint32_t name_size = static_cast<std::uint32_t>(drive.device_name.size());
        stream.write(reinterpret_cast<const char*>(&name_size), sizeof(name_size));
        stream.write(drive.device_name.data(), name_size);

        std::uint32_t serial_size = static_cast<std::uint32_t>(drive.serial.size());
        stream.write(reinterpret_cast<const char*>(&serial_size), sizeof(serial_size));
        stream.write(drive.serial.data(), serial_size);
    }
}
