#pragma once

#include <imgui.h>
#include <SDL.h>

class AppFramework
{
public:
    static AppFramework& Get() { static AppFramework Instance; return Instance; }

    AppFramework(const AppFramework&) = delete;
    AppFramework& operator=(const AppFramework&) = delete;
    ~AppFramework();

    [[nodiscard]] bool IsRunning() const;

    void DoUpdateUIAndRender();
    void RequestExitApp() { RequestExit = true; }

    [[nodiscard]] ImGuiID GetSettingsWindowID() const { return SettingsWindowID; }

private:
    AppFramework();

    bool InitSDL();
    bool InitImGui();
    bool InitImGuiStyle();

    void ProcessInputsAndNewFrame();
    void PostRender();

    void ShowSettingsWindow();

    bool IsInitialized = false;
    bool RequestExit = false;
    SDL_Window* Window = nullptr;
    SDL_GLContext OpenGLContext = nullptr;
    float MainScale = 1.0f;
    ImGuiID SettingsWindowID = 0;
};