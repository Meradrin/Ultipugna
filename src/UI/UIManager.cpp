#include "UI/UIManager.h"

#include <functional>

#include "MemoryViewerWindow.h"
#include "TileViewerWindow.h"
#include "UI/LogWindow.h"
#include "UI/RenderWindow.h"
#include "UI/ShortcutAndMenuUtils.h"

UIManager UIManager::Instance;

bool UIManager::Initialize()
{
    AddWindow<RenderWindow>(0);
    AddWindow<LogWindow>();
    AddWindow<MemoryViewerWindow>();
    AddWindow<TileViewerWindow>();

    return true;
}

void UIManager::Render()
{
    const ImGuiViewport* Viewport = ImGui::GetMainViewport();
    const float MenuFrameHeight = ImGui::GetFrameHeight();
    const float ToolbarHeight = ShowToolbar ? ImGui::GetFrameHeight() * 1.5f : 0.0f;

    ImGuiUtil_UpdateShortcut();
    ImGuiUtil_DisplayMenuBar();

    if (ShowToolbar)
        RenderToolbar(Viewport, MenuFrameHeight, ToolbarHeight);

    ImGui::SetNextWindowPos(ImVec2(Viewport->Pos.x, Viewport->Pos.y + MenuFrameHeight + ToolbarHeight));
    ImGui::SetNextWindowSize(ImVec2(Viewport->Size.x, Viewport->Size.y - (MenuFrameHeight + ToolbarHeight)));
    ImGui::SetNextWindowViewport(Viewport->ID);

    constexpr ImGuiWindowFlags DockingZoneFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0,0,0,0));
    ImGui::Begin("DockHost", nullptr, DockingZoneFlags);
    ImGui::PopStyleColor();

    const ImGuiID DockSpaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(DockSpaceId, ImVec2(0,0), ImGuiDockNodeFlags_PassthruCentralNode);

    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        if (Window->IsOpen)
        {
            Window->Render();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(1);
}

void UIManager::Stop()
{
    RemoveWindow<RenderWindow>();
    RemoveWindow<LogWindow>();
    RemoveWindow<MemoryViewerWindow>();
    RemoveWindow<TileViewerWindow>();
}

void UIManager::OnEmulationCoreStart(IEmulatorCore* EmulatorCore)
{
    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        Window->OnEmulationCoreStart(EmulatorCore);
    }
}

void UIManager::OnEmulationCoreStop()
{
    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        Window->OnEmulationCoreStop();
    }
}

void UIManager::OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath)
{
    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        Window->OnEmulationMediaOpen(MediaSource, MediaPath);
    }
}

void UIManager::OnEmulationMediaClose(std::uint32_t MediaSource)
{
    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        Window->OnEmulationMediaClose(MediaSource);
    }
}

void UIManager::OnNewWindow(IWindow& Window)
{
    ImGuiUtil_AddMenuItem("View@2->" + Window.Title(), Window.GetDisplayShortcutKey(), Window.Title(), nullptr, &Window.IsOpen);
}

void UIManager::OnRemoveWindow(IWindow& Window)
{
    ImGuiUtil_RemoveMenuItem("View@2->" + Window.Title());
}

void UIManager::RenderToolbar(const ImGuiViewport* Viewport, float MenuFrameHeight, float ToolbarHeight)
{
    constexpr ImGuiWindowFlags ToolbarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;

    ImGui::SetNextWindowPos(ImVec2(Viewport->Pos.x, Viewport->Pos.y + MenuFrameHeight));
    ImGui::SetNextWindowSize(ImVec2(Viewport->Size.x, ToolbarHeight));
    ImGui::SetNextWindowViewport(Viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4,5));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(6,4));

    if (ImGui::Begin("ToolbarOverlay", nullptr, ToolbarFlags))
    {
        if (ImGui::Button("Play"))
        {

        }
        ImGui::SameLine();
        if (ImGui::Button("Pause"))
        {

        }
        //ImGui::SameLine();
        //if (ImGui::Button("Step")) { /* ... */ }
        //ImGui::SameLine();
        //static int i1 = 0;
        //ImGui::SetNextItemWidth(100.0f);
        //ImGui::SliderInt("slider int", &i1, -1, 100);
        //ImGui::SameLine();
        //if (ImGui::Button("Step2")) { /* ... */ }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}
