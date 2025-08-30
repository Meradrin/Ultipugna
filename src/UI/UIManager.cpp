#include "UI/UIManager.h"

#include <functional>
#include "UI/LogWindow.h"
#include "UI/RenderWindow.h"
#include "UI/ShortcutAndMenuUtils.h"

UIManager UIManager::Instance;

void UIManager::Initialize()
{
    AddWindow<RenderWindow>(0);
    AddWindow<LogWindow>();
}

void UIManager::Render()
{
    ImGuiUtil_UpdateShortcut();
    ImGuiUtil_DisplayMenuBar();

    ImGuiID DockSpaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(DockSpaceId, ImGui::GetMainViewport());

    for (const std::unique_ptr<IWindow>& Window : Windows)
    {
        if (Window->IsOpen)
        {
            Window->Render();
        }
    }
}

void UIManager::Stop()
{
    RemoveWindow<RenderWindow>();
    RemoveWindow<LogWindow>();
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

void UIManager::OnNewWindow(IWindow& Window)
{
    ImGuiUtil_AddMenuItem("View@2->" + Window.Title(), Window.GetDisplayShortcutKey(), Window.Title(), nullptr, &Window.IsOpen);
}

void UIManager::OnRemoveWindow(IWindow& Window)
{
    ImGuiUtil_RemoveMenuItem("View@2->" + Window.Title());
}
