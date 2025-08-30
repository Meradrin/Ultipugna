#pragma once

#include <memory>
#include <vector>
#include "imgui.h"
#include "IWindow.h"
#include "Util/HashUtil.h"

class RenderWindow final : public IWindow
{
public:
    static consteval std::uint64_t StaticTypeId() { return SourceLocationUniqueId64(); }

    RenderWindow(std::uint32_t MediaSource);
    virtual ~RenderWindow() override;

    virtual std::uint64_t TypeId() override;

    virtual void OnEmulationCoreStart(IEmulatorCore* Emulator) override;
    virtual void OnEmulationCoreStop() override;

    virtual const std::string& Title() override;
    virtual void Render() override;

private:
    void DestroyTexture();
    void CreateTexture(std::uint32_t Width, std::uint32_t Height);
    void UpdateTexture(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels);

    ImTextureID RenderTexture = ImTextureID_Invalid;
    float RenderWidth = 0;
    float RenderHeight = 0;
    std::uint32_t Source = 0;
    std::string TitleName;

    static void RenderCallback(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels);
    static std::vector<RenderWindow*> CurrentInstances;
};
