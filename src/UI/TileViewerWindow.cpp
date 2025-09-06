#include "TileViewerWindow.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <SDL_opengl.h>
#include "UI/ShortcutAndMenuUtils.h"
#include "Util/ImGuiMathUtil.h"

TileViewerWindow::TileViewerWindow()
{
    InitDefaultPalette();
}

std::uint64_t TileViewerWindow::TypeId()
{
    return TileViewerWindow::StaticTypeId();
}

void TileViewerWindow::OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath)
{
    const std::vector<MemoryRegion>& MemoryRegions = IEmulatorCore::Current()->GetMemoryRegions();

    SelectedMemoryRegion = 0;
    DisplayAddress = 0;
    MemoryRegionNames = "";
    PreviewIndex = 0;

    for (const MemoryRegion& MemoryRegion : MemoryRegions)
    {
        MemoryRegionNames += MemoryRegion.Name;
        MemoryRegionNames += '\0';
    }
}

void TileViewerWindow::OnEmulationCoreStop()
{
    MemRegion = nullptr;
}

const std::string& TileViewerWindow::Title()
{
    static std::string Title = "Tile Viewer";
    return Title;
}

void TileViewerWindow::Render()
{
    if (const IEmulatorCore* EmulatorCore = IEmulatorCore::Current())
    {
        const std::vector<MemoryRegion>& MemRegions = EmulatorCore->GetMemoryRegions();

        if (SelectedMemoryRegion < MemRegions.size() && MemRegion != &MemRegions[SelectedMemoryRegion])
        {
            MemRegion = &MemRegions[SelectedMemoryRegion];
        }
    }

    ImGui::Begin(Title().c_str());

    if (MemRegion != nullptr)
    {
        constexpr std::int32_t TileSizes[] = { 16, 32 };
        const std::int32_t TileSize = TileSizes[static_cast<std::int32_t>(Format)];

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Memory Region:");
        ImGui::SameLine();
        ImGuiUtil_ComboAutoWidth("##SelectedMemoryRegion", &SelectedMemoryRegion, MemoryRegionNames.c_str());
        ImGui::SameLine();
        ImGui::TextUnformatted("Format:");
        ImGui::SameLine();
        ImGuiUtil_ComboAutoWidth("##TileFormat", reinterpret_cast<int*>(&Format), "GameBoy 2BPP\0""Genesis 4BPP\0");

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Address:");
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::CalcTextSize("00000000").x + ImGui::GetStyle().FramePadding.x * 3 + ImGui::GetFrameHeight() * 2);
        ImGui::InputInt("##Address", &DisplayAddress, TileSize, TileSize * 0x10, ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::PopItemWidth();

        if (DisplayAddress < static_cast<std::int32_t>(MemRegion->StartAddress))
        {
            DisplayAddress = static_cast<std::int32_t>(MemRegion->StartAddress);
        }
        else if (DisplayAddress > static_cast<std::int32_t>(MemRegion->EndAddress))
        {
            DisplayAddress = static_cast<std::int32_t>(MemRegion->EndAddress);
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("Palette:");
        ImGui::SameLine();
        PaletteColors.resize(1);
        const auto& ColorPalettes = IEmulatorCore::Current()->GetTilePreviewPalettes();
        PaletteColors.insert(PaletteColors.end(), ColorPalettes.begin(), ColorPalettes.end());
        std::string PaletteNames = "Default Generic";
        PaletteNames += '\0';
        for (uint32_t PaletteIndex = 1; PaletteIndex < PaletteColors.size(); ++PaletteIndex)
        {
            PaletteNames += "Palette ";
            PaletteNames += std::to_string(PaletteIndex);
            PaletteNames += '\0';
        }
        ImGuiUtil_ComboAutoWidth("##PreviewIndex", reinterpret_cast<int*>(&PreviewIndex), PaletteNames.c_str());

        ImGui::Separator();

        constexpr  float ScrollBarWidth = 18.0f;
        const ImVec2 Available = ImGui::GetContentRegionAvail();
        const std::int32_t ImageTargetX = 1 << std::clamp(static_cast<std::int32_t>(std::round(std::log2(static_cast<double>(Available.x / 4)))), 7, 10);
        const float ZoomTarget = Available.x / static_cast<float>(ImageTargetX);
        const std::int32_t ImageTargetY = static_cast<std::int32_t>(Available.y / ZoomTarget);

        if (ImageTargetX != ImageWidth || ImageTargetY != ImageHeight)
        {
            ImageWidth = ImageTargetX;
            ImageHeight = ImageTargetY;
            Image.resize(ImageWidth * ImageHeight);
            if (ImageTexture)
                DestroyTexture(ImageTexture);
            ImageTexture = CreateTextureSize(ImageWidth, ImageHeight);
        }
        const std::int32_t TileByRowCount = ImageWidth / 8;
        const std::int32_t RowCount = ImageHeight / 8;

        for (std::int32_t Row = 0, TileAddress = DisplayAddress; Row < RowCount; ++Row)
        {
            for (std::int32_t Column = 0; Column < TileByRowCount; ++Column)
            {
                TileToImage(TileAddress, Column * 8, Row * 8);
                TileAddress += TileSize;
            }
        }

        UpdateTexture(ImageTexture, Image, ImageWidth, ImageHeight);

        ImGui::Image(ImageTexture, ImVec2(Available.x - ScrollBarWidth, Available.y), ImVec2(0, 0), ImVec2(1, 1));

        if (ImGui::IsItemHovered())
        {
            constexpr ImVec2 TilePixelSize = ImVec2(8, 8);
            const ImVec2 LocalPosition = ImGui::GetIO().MousePos - ImGui::GetItemRectMin();
            const ImVec2 LocalSize = ImGui::GetItemRectSize();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));

            if (ImGui::BeginItemTooltip())
            {
                const ImVec2 TilePosition = (LocalPosition / LocalSize) * ImVec2(static_cast<float>(ImageWidth), static_cast<float>(ImageHeight)) / TilePixelSize;
                const ImVec2 TileStartUV = ImVec2(std::floor(TilePosition.x) * TilePixelSize.x / static_cast<float>(ImageWidth), std::floor(TilePosition.y) * TilePixelSize.y / static_cast<float>(ImageHeight));
                const ImVec2 TileEndUV = TileStartUV + TilePixelSize / ImVec2 {static_cast<float>(ImageWidth), static_cast<float>(ImageHeight)};
                const int TileIndex = static_cast<int>(std::floor(TilePosition.x)) + static_cast<int>(std::floor(TilePosition.y)) * TileByRowCount;
                const std::int32_t TileAddress = DisplayAddress + TileIndex * TileSize;

                ImGui::Image(ImageTexture, ImVec2(64, 64), TileStartUV, TileEndUV);
                ImGui::SameLine();
                ImGui::Text("Tile Address: 0x%04x", TileAddress);
                ImGui::EndTooltip();
            }

            ImGui::PopStyleVar();

            ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

            if (const float Wheel = ImGui::GetIO().MouseWheel; Wheel != 0.0f)
                DisplayAddress = std::clamp(DisplayAddress + TileByRowCount * TileSize * static_cast<std::int32_t>(-Wheel), static_cast<std::int32_t>(MemRegion->StartAddress), static_cast<std::int32_t>(MemRegion->EndAddress));
        }

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().FramePadding.x - 1);

        const int MaxScroll = static_cast<int>(MemRegion->EndAddress - MemRegion->StartAddress) / TileSize;
        int SliderAddress = MaxScroll - ((DisplayAddress - static_cast<int>(MemRegion->StartAddress)) / TileSize);
        ImGui::VSliderInt("##Scrolling", ImVec2(ScrollBarWidth, Available.y), &SliderAddress, 0, MaxScroll, "", ImGuiSliderFlags_ClampOnInput);
        DisplayAddress = (MaxScroll - SliderAddress) * TileSize + static_cast<int>(MemRegion->StartAddress);
    }

    ImGui::End();
}

ImTextureID TileViewerWindow::CreateTextureSize(std::int32_t Width, std::int32_t Height)
{
    GLuint TextureId;
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    return TextureId;
}

void TileViewerWindow::DestroyTexture(ImTextureID& TextureID)
{
    if (TextureID != ImTextureID_Invalid)
    {
        const auto OpenGLTextureId = static_cast<GLuint>(static_cast<intptr_t>(TextureID));
        glDeleteTextures(1, &OpenGLTextureId);
        TextureID = ImTextureID_Invalid;
    }
}

void TileViewerWindow::UpdateTexture(ImTextureID TextureID, std::span<std::uint32_t> Pixels, std::int32_t Width, std::int32_t Height)
{
    const auto OpenGLTextureId = static_cast<GLuint>(static_cast<intptr_t>(TextureID));
    glBindTexture(GL_TEXTURE_2D, OpenGLTextureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height), GL_RGBA, GL_UNSIGNED_BYTE, Pixels.data());
}

void TileViewerWindow::TileToImage(std::uint64_t MemoryAddress, std::size_t OutputX, std::size_t OutputY)
{
    auto ReadData = [&](std::uint64_t Address) -> std::uint8_t
    {
        if (Address <= MemRegion->EndAddress && Address >= MemRegion->StartAddress)
            return static_cast<std::uint8_t>(MemRegion->Read(Address));

        return 0;
    };
    auto WritePixel = [&](std::size_t Index, std::size_t X, std::size_t Y)
    {
        if (X < ImageWidth && Y < ImageHeight)
            Image[X + Y * ImageWidth] = PaletteColors[PreviewIndex][Index];
    };

    switch (Format)
    {
    case TileFormat::GB_2BP:
        for (std::size_t TileY = 0; TileY < 8; ++TileY)
        {
            const std::uint8_t LowByte = ReadData(MemoryAddress++);
            const std::uint8_t HighByte = ReadData(MemoryAddress++);
            for (std::size_t TileX = 0; TileX < 8; ++TileX)
            {
                const std::size_t BitIndex = 7 - TileX;
                const std::uint8_t ColorIndex = ((HighByte >> BitIndex) & 1) << 1 | ((LowByte >> BitIndex) & 1);
                WritePixel(ColorIndex, OutputX + TileX, OutputY + TileY);
            }
        }
        break;
    case TileFormat::Genesis_4BPP:
        for (std::size_t TileY = 0; TileY < 8; ++TileY)
        {
            std::uint8_t Data = ReadData(MemoryAddress++);
            WritePixel(Data >> 4, OutputX, OutputY + TileY);
            WritePixel(Data & 0xf, OutputX + 1, OutputY + TileY);
            Data = ReadData(MemoryAddress++);
            WritePixel(Data >> 4, OutputX + 2, OutputY + TileY);
            WritePixel(Data & 0xf, OutputX + 3, OutputY + TileY);
            Data = ReadData(MemoryAddress++);
            WritePixel(Data >> 4, OutputX + 4, OutputY + TileY);
            WritePixel(Data & 0xf, OutputX + 5, OutputY + TileY);
            Data = ReadData(MemoryAddress++);
            WritePixel(Data >> 4, OutputX + 6, OutputY + TileY);
            WritePixel(Data & 0xf, OutputX + 7, OutputY + TileY);
        }
        break;
    }
}

void TileViewerWindow::InitDefaultPalette()
{
    PaletteColors.resize(1);
    PaletteColors[0] = {
        0xFF000000, // Black
        0xFFFFFFFF, // White
        0xFFFF0000, // Red
        0xFF00FF00, // Green
        0xFF0000FF, // Blue
        0xFFFFFF00, // Yellow
        0xFF00FFFF, // Cyan
        0xFFFF00FF, // Magenta
        0xFF800000, // Dark Red
        0xFF008000, // Dark Green
        0xFF000080, // Dark Blue
        0xFF808000, // Dark Yellow
        0xFF008080, // Dark Cyan
        0xFF800080, // Dark Magenta
        0xFF808080, // Gray
        0xFFC0C0C0 // Light Gray
    };
}
