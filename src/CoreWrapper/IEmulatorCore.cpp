#include "IEmulatorCore.h"

IEmulatorCore* IEmulatorCore::CurrentCore = nullptr;

IEmulatorCore* IEmulatorCore::Current()
{
    return CurrentCore;
}

void IEmulatorCore::SetCurrent(IEmulatorCore* Core)
{
    CurrentCore = Core;
}

const std::vector<MemoryRegion>& IEmulatorCore::GetMemoryRegions() const
{
    static constexpr std::vector<MemoryRegion> EmptyRegions;
    return EmptyRegions;
}

const std::vector<CPUDescription>& IEmulatorCore::GetCPUs() const
{
    static constexpr std::vector<CPUDescription> EmptyCPUs;
    return EmptyCPUs;
}