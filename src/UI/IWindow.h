#pragma once

#include "imgui.h"
#include "CoreWrapper/IEmulatorCore.h"

class IWindow
{
public:
    virtual ~IWindow() = default;

    virtual std::uint64_t TypeId() = 0;

    virtual void OnEmulationCoreStart(IEmulatorCore* Emulator);
    virtual void OnEmulationCoreStop();

    virtual void OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath);
    virtual void OnEmulationMediaClose(std::uint32_t MediaSource);

    virtual ImGuiKeyChord GetDisplayShortcutKey() { return ImGuiKey_None; }
    virtual const std::string& Title() = 0;
    virtual void Render() = 0;

    bool IsOpen = false;
};
