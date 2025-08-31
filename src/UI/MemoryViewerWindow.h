#pragma once

#include "imgui_hex.h"
#include "IWindow.h"
#include "Util/HashUtil.h"

class MemoryViewerWindow final : public IWindow
{
public:
    static consteval std::uint64_t StaticTypeId() { return SourceLocationUniqueId64(); }

    virtual std::uint64_t TypeId() override;

    virtual void OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath) override;
    virtual void OnEmulationCoreStop() override;

    virtual const std::string& Title() override;
    virtual void Render() override;

private:
    void SelectMemoryRegion(std::size_t Index);

    ImGuiHexEditorState MemEditorState;
    const MemoryRegion* MemRegion = nullptr;
    int SelectedMemoryRegion = 0;
    std::uint64_t DisplayAddress = 0;
    std::string MemoryRegionNames;
};
