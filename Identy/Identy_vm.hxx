#pragma once

#ifndef UNC_IDENTY_VM_H
#define UNC_IDENTY_VM_H

#include "Identy_hwid.hxx"

namespace identy
{
/// Returns a flag is Identy thinks is given motherboard is virtual
bool assume_virtual(identy::Motherboard& mb);
/// Returns a flag is Identy thinks is given motherboard is virtual
bool assume_virtual(identy::MotherboardEx& mb);
} // namespace identy

#endif