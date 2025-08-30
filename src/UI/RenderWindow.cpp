#include "RenderWindow.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <SDL_opengl.h>
#include "imgui.h"
#include "GL/glcorearb.h"

extern "C"
{
    #include "types.h"
    #include "system.h"
}

std::vector<RenderWindow*> RenderWindow::CurrentInstances;

RenderWindow::RenderWindow(std::uint32_t MediaSource)
    : Source(MediaSource)
{
    CurrentInstances.push_back(this);
    IsOpen = true;

    if (MediaSource == 0)
    {
        TitleName = "Render Screen";
    }
    else
    {
        std::ostringstream Title;
        Title << "Render Screen " << (Source + 1);
        TitleName = Title.str();
    }
}

RenderWindow::~RenderWindow()
{
    std::erase(CurrentInstances, this);
    DestroyTexture();
}

std::uint64_t RenderWindow::TypeId()
{
    return StaticTypeId();
}

void RenderWindow::Render()
{
    ImGui::Begin(Title().c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (RenderTexture != ImTextureID_Invalid)
    {
        const ImVec2 RegionMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
        float MinScale = std::min(ContentSize.x / RenderWidth, ContentSize.y / RenderHeight);

        if (MinScale > 1.f)
            MinScale = std::floor(MinScale);

        const ImVec2 ImageSize = { RenderWidth * MinScale, RenderHeight * MinScale };
        const ImVec2 ImagePos = { (ContentSize.x - ImageSize.x) * 0.5f + RegionMin.x, (ContentSize.y - ImageSize.y) * 0.5f + RegionMin.y };

        ImGui::SetCursorPos(ImagePos);
        ImGui::Image(RenderTexture, ImageSize, ImVec2(0,0), ImVec2(1, 1));
    }

    ImGui::End();
}

void RenderWindow::DestroyTexture()
{
    if (RenderTexture != ImTextureID_Invalid)
    {
        const GLuint TextureId = static_cast<GLuint>(static_cast<intptr_t>(RenderTexture));
        glDeleteTextures(1, &TextureId);
        RenderTexture = ImTextureID_Invalid;
    }
}

void RenderWindow::CreateTexture(std::uint32_t Width, std::uint32_t Height)
{
    GLuint TextureId;
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height), 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    RenderTexture = static_cast<ImTextureID>(static_cast<intptr_t>(TextureId));
    RenderWidth = static_cast<float>(Width);
    RenderHeight = static_cast<float>(Height);
}

void RenderWindow::UpdateTexture(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels)
{
    if (RenderWidth != Width || RenderHeight != Height)
    {
        DestroyTexture();
        CreateTexture(Width, Height);
    }

    const GLuint TextureId = static_cast<GLuint>(static_cast<intptr_t>(RenderTexture));
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(Width), static_cast<GLsizei>(Height), GL_BGRA, GL_UNSIGNED_BYTE, Pixels.data());
}

void RenderWindow::RenderCallback(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels)
{
    // Todo: support multiple output screen
    if (!CurrentInstances.empty() && CurrentInstances[0] != nullptr)
    {
        CurrentInstances[0]->UpdateTexture(Width, Height, Pixels);
    }
}

void RenderWindow::OnEmulationCoreStart(IEmulatorCore* Emulator)
{
    Emulator->SetRenderCallback(&RenderWindow::RenderCallback);
}

void RenderWindow::OnEmulationCoreStop()
{
    DestroyTexture();
}

const std::string& RenderWindow::Title()
{
    return TitleName;
}
