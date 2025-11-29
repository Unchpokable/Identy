#include "Identy_pch.hxx"

#include "Identy_vm.hxx"

identy::vm::HeuristicVerdict identy::vm::DefaultHeuristic::operator()(const identy::Motherboard& mb) const
{
    return {};
}

identy::vm::HeuristicVerdict identy::vm::DefaultHeuristicEx::operator()(const identy::MotherboardEx& mb) const
{
    return {};
}
