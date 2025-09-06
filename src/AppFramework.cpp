#include "AppFramework.h"

#include <algorithm>
#include <iostream>
#include <SDL_opengl.h>

#include "EmulatorCoreManager.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "UI/ShortcutAndMenuUtils.h"
#include "UI/UIManager.h"
#include "Util/Config.h"

#if DEBUG_BUILD
bool ShowDemoWindow = false;

IMGUI_UTIL_CREATE_MENU_ITEM("About->Show Debug", ImGuiKey_None, "")
{
    ShowDemoWindow = !ShowDemoWindow;
}
#endif

IMGUI_UTIL_CREATE_MENU_ITEM("File->|Settings@3", ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_S, "")
{
    ImGui::OpenPopup(AppFramework::Get().GetSettingsWindowID());
}

IMGUI_UTIL_CREATE_MENU_ITEM("File@0->|Exit", ImGuiMod_Alt | ImGuiKey_F4, "")
{
    AppFramework::Get().RequestExitApp();
}

AppFramework::AppFramework()
{
    IsInitialized = Config::Instance().Load()
        && InitSDL()
        && InitImGui()
        && InitImGuiStyle()
        && UIManager::Get().Initialize()
        && EmulatorCoreManager::Get().Initialize();

    RequestExit = !IsInitialized;
}

AppFramework::~AppFramework()
{
    EmulatorCoreManager::Get().StopEmulation();
    UIManager::Get().Stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(OpenGLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
}


bool AppFramework::InitSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "SDL Error: " << SDL_GetError() << '\n';
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    MainScale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

    Window = SDL_CreateWindow("Ultipugna", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(1280 * MainScale), static_cast<int>(720 * MainScale),
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (Window == nullptr)
    {
        std::cerr << "SDL Window Error: " << SDL_GetError() << '\n';
        return false;
    }

    OpenGLContext = SDL_GL_CreateContext(Window);

    if (OpenGLContext == nullptr || SDL_GL_MakeCurrent(Window, OpenGLContext) == -1)
    {
        std::cerr << "SDL OpenGL Context Error: " << SDL_GetError() << '\n';
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    return true;
}

bool AppFramework::InitImGui()
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& Style = ImGui::GetStyle();
    Style.ScaleAllSizes(MainScale);
    Style.FontScaleDpi = MainScale;
    IO.ConfigDpiScaleFonts = true;
    IO.ConfigDpiScaleViewports = true;

    return ImGui_ImplSDL2_InitForOpenGL(Window, OpenGLContext) && ImGui_ImplOpenGL3_Init("#version 150");
}

bool AppFramework::InitImGuiStyle()
{
    // Source from: https://github.com/shivang51/bess/blob/main/Bess/src/settings/themes.cpp#L39

    ImGuiStyle& Style = ImGui::GetStyle();
    ImVec4* Colors = Style.Colors;

    // Primary background
    Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);  // #131318
    Colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f); // #131318

    Colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    // Headers
    Colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

    // Buttons
    Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

    // Frame BG
    Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

    // Tabs
    Colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    Colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
    Colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
    Colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

    // Title
    Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    // Borders
    Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text
    Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    // Highlights
    Colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    Colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
    Colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
    Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
    Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

    // Scrollbar
    Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

    // Style tweaks
    Style.WindowRounding = 5.0f;
    Style.FrameRounding = 5.0f;
    Style.GrabRounding = 5.0f;
    Style.TabRounding = 5.0f;
    Style.PopupRounding = 5.0f;
    Style.ScrollbarRounding = 5.0f;
    Style.WindowPadding = ImVec2(10, 10);
    Style.FramePadding = ImVec2(6, 4);
    Style.ItemSpacing = ImVec2(8, 6);
    Style.PopupBorderSize = 0.f;

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    return true;
}

void AppFramework::ProcessInputsAndNewFrame()
{
    SDL_Event Event;

    while (SDL_PollEvent(&Event))
    {
        ImGui_ImplSDL2_ProcessEvent(&Event);

        if (Event.type == SDL_QUIT)
            RequestExit = true;

        if (Event.type == SDL_WINDOWEVENT && Event.window.event == SDL_WINDOWEVENT_CLOSE && Event.window.windowID == SDL_GetWindowID(Window))
            RequestExit = true;
    }

    if (SDL_GetWindowFlags(Window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void AppFramework::PostRender()
{
    ImGui::Render();

    const ImGuiIO& IO = ImGui::GetIO();

    glViewport(0, 0, static_cast<GLsizei>(IO.DisplaySize.x), static_cast<GLsizei>(IO.DisplaySize.y));
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* BackupCurrentWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext BackupCurrentOpenGlContext = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(BackupCurrentWindow, BackupCurrentOpenGlContext);
    }

    SDL_GL_SwapWindow(Window);
}

void AppFramework::ShowSettingsWindow()
{
    SettingsWindowID = ImGui::GetID("Settings");

    if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char Filter[512] = "";
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::PushItemWidth(650);
        ImGui::InputText("##FilterSetting", Filter, IM_ARRAYSIZE(Filter));
        ImGui::PopItemWidth();
        ImGui::Separator();

        ImGui::BeginChild("LeftPane", ImVec2(200.0f, 400.0f), ImGuiChildFlags_None);
        ImGui::PushItemWidth(-FLT_MIN);
        const float ContentAvailableY = ImGui::GetContentRegionAvail().y;

        std::vector<std::string> CategoryNames =
        {
            "Video",
            "Audio",
        };

        for (const std::unique_ptr<IEmulatorCore>& Emulator : EmulatorCoreManager::Get().EmulatorCores)
        {
            if (!Emulator->GetSettingsTypes().empty())
            {
                CategoryNames.insert(CategoryNames.begin(), "Core " + Emulator->Name());
            }
        }

        static std::size_t CurrentCategory = 0;

        if (ImGui::BeginListBox("##SettingsSection", ImVec2(-FLT_MIN, ContentAvailableY)))
        {
            for (std::size_t Index = 0; Index < CategoryNames.size(); ++Index)
            {
                if (const bool Selected = (CurrentCategory == Index); ImGui::Selectable(CategoryNames[Index].c_str(), Selected))
                    CurrentCategory = Index;
            }
            ImGui::EndListBox();
        }

        ImGui::PopItemWidth();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("RightPane", ImVec2(500.0f, 400.0f), ImGuiChildFlags_None);
        ImGui::SetWindowFontScale(2.0f);
        ImGui::TextUnformatted(CategoryNames[CurrentCategory].c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();

        if (CurrentCategory < EmulatorCoreManager::Get().EmulatorCores.size())
        {
            for (const auto& [Name, Type] : EmulatorCoreManager::Get().EmulatorCores[CurrentCategory]->GetSettingsTypes())
            {
                std::string ConfigKey = "Core." + EmulatorCoreManager::Get().EmulatorCores[CurrentCategory]->Name() + "." + Name;

                ImGui::Dummy(ImVec2(10.0f, 0.0f));
                ImGui::TextUnformatted((Name + ":").c_str());
                ImGui::SameLine();

                switch (Type)
                {
                    case SettingType::Boolean:
                    {
                        bool Value = Config::Instance().Get(ConfigKey, "False") == "True";
                        ImGui::Checkbox(("##" + Name).c_str(), &Value);
                        Config::Instance()[ConfigKey] = Value ? "True" : "False";
                        break;
                    }
                    case SettingType::Integer:
                    {
                        int Value = std::stoi(Config::Instance().Get(ConfigKey, "0"));
                        ImGui::InputInt(("##" + Name).c_str(), &Value);
                        Config::Instance()[ConfigKey] = std::to_string(Value);
                        break;
                    }
                    case SettingType::String:
                    {
                        std::string CurrentValue = Config::Instance().Get(ConfigKey, "");
                        char Text[512] = "";
                        std::copy_n(CurrentValue.c_str(), std::min(CurrentValue.size(), sizeof(Text) - 1), Text);
                        Text[std::min(CurrentValue.size(), sizeof(Text) - 1)] = '\0';
                        ImGui::InputText(("##" + Name).c_str(), Text, IM_ARRAYSIZE(Text));
                        Config::Instance()[ConfigKey] = Text;
                        break;
                    }
                    case SettingType::Float:
                    {
                        float Value = std::stof(Config::Instance().Get(ConfigKey, "0.0"));
                        ImGui::InputFloat(("##" + Name).c_str(), &Value);
                        Config::Instance()[ConfigKey] = std::to_string(Value);
                        break;
                    }
                    case SettingType::Directory:
                    {
                        std::string CurrentValue = Config::Instance().Get(ConfigKey, "");
                        char Text[512] = "";
                        std::copy_n(CurrentValue.c_str(), std::min(CurrentValue.size(), sizeof(Text) - 1), Text);
                        Text[std::min(CurrentValue.size(), sizeof(Text) - 1)] = '\0';
                        ImGui::InputText(("##" + Name).c_str(), Text, IM_ARRAYSIZE(Text));
                        Config::Instance()[ConfigKey] = Text;
                        break;
                    }
                    case SettingType::File:
                    {
                        std::string CurrentValue = Config::Instance().Get(ConfigKey, "");
                        char Text[512] = "";
                        std::copy_n(CurrentValue.c_str(), std::min(CurrentValue.size(), sizeof(Text) - 1), Text);
                        Text[std::min(CurrentValue.size(), sizeof(Text) - 1)] = '\0';
                        ImGui::InputText(("##" + Name).c_str(), Text, IM_ARRAYSIZE(Text));
                        Config::Instance()[ConfigKey] = Text;
                        break;
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Close"))
        {
            Config::Instance().Save();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool AppFramework::IsRunning() const
{
    return !RequestExit;
}

void AppFramework::DoUpdateUIAndRender()
{
    if (RequestExit)
        return;

    ProcessInputsAndNewFrame();

    UIManager::Get().Render();
    ShowSettingsWindow();

#if DEBUG_BUILD
    if (ShowDemoWindow)
    {
        ImGui::ShowDemoWindow(&ShowDemoWindow);
    }
#endif

    PostRender();
}
