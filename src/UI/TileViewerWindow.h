#pragma once

#include "IWindow.h"
#include "Util/HashUtil.h"

enum class TileFormat
{
    GB_2BP,
    Genesis_4BPP
};

class TileViewerWindow : public IWindow
{
public:
    static consteval std::uint64_t StaticTypeId() { return SourceLocationUniqueId64(); }

    TileViewerWindow();

    virtual std::uint64_t TypeId() override;

    virtual void OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath) override;
    virtual void OnEmulationCoreStop() override;

    virtual const std::string& Title() override;
    virtual void Render() override;

private:
    static ImTextureID CreateTextureSize(std::int32_t Width, std::int32_t Height);
    static void DestroyTexture(ImTextureID& TextureID);
    static void UpdateTexture(ImTextureID TextureID, std::span<std::uint32_t> Pixels, std::int32_t Width, std::int32_t Height);

    void TileToImage(std::uint64_t MemoryAddress, std::size_t OutputX, std::size_t OutputY);
    void InitDefaultPalette();

    TileFormat Format = TileFormat::Genesis_4BPP;

    const MemoryRegion* MemRegion = nullptr;
    std::string MemoryRegionNames;
    std::int32_t SelectedMemoryRegion = 0;
    std::int32_t DisplayAddress = 0;

    ImTextureID ImageTexture = ImTextureID_Invalid;
    std::vector<std::uint32_t> Image;
    std::int32_t ImageWidth = 0;
    std::int32_t ImageHeight = 0;

    std::int32_t PreviewIndex = 0;
    std::vector<std::array<std::uint32_t, 256>> PaletteColors;
};
