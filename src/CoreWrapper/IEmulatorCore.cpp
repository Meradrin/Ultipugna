#include "IEmulatorCore.h"

#include "Util/Config.h"

IEmulatorCore* IEmulatorCore::CurrentCore = nullptr;

IEmulatorCore* IEmulatorCore::Current()
{
    return CurrentCore;
}

void IEmulatorCore::SetCurrent(IEmulatorCore* Core)
{
    CurrentCore = Core;
}

const std::string& IEmulatorCore::GetSettingValue(const std::string& SettingName) const
{
    std::string ConfigKey = "EmulatorCore.";
    ConfigKey += Name();
    ConfigKey += ".";
    ConfigKey += SettingName;
    return Config::Instance()[ConfigKey];
}

void IEmulatorCore::SetSettingValue(const std::string& SettingName, const std::string& Value) const
{
    std::string ConfigKey = "EmulatorCore.";
    ConfigKey += Name();
    ConfigKey += ".";
    ConfigKey += SettingName;
    Config::Instance()[ConfigKey] = Value;
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

const std::vector<std::array<std::uint32_t, 256>>& IEmulatorCore::GetTilePreviewPalettes() const
{
    static std::vector<std::array<std::uint32_t, 256>> EmptyPalettes;
    return EmptyPalettes;
}
