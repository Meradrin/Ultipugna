#include <cstring>
#include <filesystem>
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"

#include "ImGuiFileDialog.h"
#include "CoreWrapper/GenesisPlusGX.h"
#include "CoreWrapper/IEmulatorCore.h"
#include "UI/IWindow.h"
#include "UI/LogWindow.h"
#include "UI/RenderWindow.h"
#include "UI/ShortcutAndMenuUtils.h"
#include "UI/UIManager.h"
#include "Util/Config.h"

static std::array<std::unique_ptr<IEmulatorCore>, 1> EmulatorCores =
{
    std::make_unique<GenesisPlusGX>(),
};

static IEmulatorCore* CurrentEmulatorCore = nullptr;
static std::uint64_t CurrentEmulatorLastTick = 0;
static bool ExitApp = false;

constexpr int AudioSampleRate = 48000;
static bool UpdateAudio = true;
static std::vector<int16_t> AudioBuffer;
SDL_AudioDeviceID AudioDevice;

void PushAudioCallback(std::uint32_t ChannelCount, std::span<std::int16_t> Samples)
{
    if (ChannelCount != 2 || Samples.empty())
        return;

    SDL_LockAudioDevice(AudioDevice);
    AudioBuffer.insert(AudioBuffer.end(), Samples.begin(), Samples.end());
    SDL_UnlockAudioDevice(AudioDevice);
}

void SDLAudioCallback(void*, Uint8* Stream, int Length)
{
    if (AudioBuffer.size() < Length)
    {
        std::memset(Stream, 0, Length);
    }
    else
    {
        std::memcpy(Stream, AudioBuffer.data(), Length);
        const std::uint32_t SampleLength = Length / 2;
        std::uint32_t LengthToRemove = SampleLength;
        while (AudioBuffer.size() - LengthToRemove >= Length * 2)
        {
            LengthToRemove += SampleLength;
        }

        AudioBuffer.erase(AudioBuffer.begin(), AudioBuffer.begin() + static_cast<std::ptrdiff_t>(LengthToRemove));
    }
}

void InitAudio()
{
    SDL_AudioSpec Desired = {};
    Desired.freq = AudioSampleRate;
    Desired.format = AUDIO_S16;
    Desired.channels = 2;
    Desired.samples = 2048;
    Desired.callback = SDLAudioCallback;
    Desired.userdata = nullptr;

    AudioBuffer.reserve(Desired.samples * Desired.channels * 20);
    AudioDevice = SDL_OpenAudioDevice(nullptr, 0, &Desired, nullptr, 0);
    if(!AudioDevice)
    {
        std::cerr << "SDL Audio open failed\n";
        return;
    }
    SDL_PauseAudioDevice(AudioDevice, 0);
}

void StopEmulatorCore()
{
    if (CurrentEmulatorCore != nullptr)
    {
        UIManager::Get().OnEmulationCoreStop();
        CurrentEmulatorCore->Shutdown();
        CurrentEmulatorCore = nullptr;
        IEmulatorCore::SetCurrent(nullptr);
        CurrentEmulatorLastTick = 0.0;
    }
}

void StartEmulatorCore(IEmulatorCore* Core)
{
    if (CurrentEmulatorCore != nullptr)
    {
        StopEmulatorCore();
    }

    if (Core != nullptr)
    {
        CurrentEmulatorCore = Core;
        IEmulatorCore::SetCurrent(CurrentEmulatorCore);
        CurrentEmulatorLastTick = SDL_GetPerformanceCounter();
        CurrentEmulatorCore->SetAudioCallback(&PushAudioCallback);
        CurrentEmulatorCore->Initialize();
        UIManager::Get().OnEmulationCoreStart(Core);

    }
}

void StartEmulationWithMedia(const std::string& FullMediaPath, const std::string& Filter);

void UpdateRecentMenuItems()
{
    static std::vector<std::string> RecentFiles;
    Config::Instance().GetArray("File.RecentFiles", RecentFiles);

    ImGuiUtil_RemoveMenuItem("File->Recent Files");
    std::int32_t PriorityOrder = 10;

    for (const std::string& RecentFile : RecentFiles)
    {
        const std::size_t Split = RecentFile.find('|');
        const std::string FullPath = RecentFile.substr(0, Split);
        const std::string Filter = RecentFile.substr(Split + 1);

        std::stringstream RecentFilesString;
        RecentFilesString << "File->Recent Files@5->" << std::filesystem::path{FullPath}.filename().string() << '@' << (--PriorityOrder);
        ImGuiUtil_AddMenuItem(RecentFilesString.str(), ImGuiKey_None, FullPath, [RecentFile]()
        {
            const std::size_t RecentSplitIndex = RecentFile.find('|');
            const std::string RecentFullPath = RecentFile.substr(0, RecentSplitIndex);
            const std::string RecentFilter = RecentFile.substr(RecentSplitIndex + 1);
            StartEmulationWithMedia(RecentFullPath, RecentFilter);
        });
    }
}

void StartEmulationWithMedia(const std::string& FullMediaPath, const std::string& Filter)
{
    const auto ItCore = std::ranges::find_if(EmulatorCores, [&Filter](const std::unique_ptr<IEmulatorCore>& Core)
    {
        return Core != nullptr && Core->GetMediaFilter(0).starts_with(Filter);
    });

    if (ItCore != EmulatorCores.end())
    {
        StartEmulatorCore(ItCore->get());
        using FilePath = std::filesystem::path;

        if (CurrentEmulatorCore != nullptr && CurrentEmulatorCore->InsertMediaSource(FullMediaPath, 0) != std::error_code{})
        {
            StopEmulatorCore();
        }
        else
        {
            std::vector<std::string> LastOpenFiles;
            Config::Instance().GetArray("File.RecentFiles", LastOpenFiles);
            const std::string FullMediaPathAndFilter = FullMediaPath + '|' + Filter;

            if (const auto Itr = std::ranges::find(LastOpenFiles.begin(), LastOpenFiles.end(), FullMediaPathAndFilter); Itr != LastOpenFiles.end())
                LastOpenFiles.erase(Itr);

            LastOpenFiles.push_back(FullMediaPathAndFilter);

            while (LastOpenFiles.size() > 5)
                LastOpenFiles.erase(LastOpenFiles.begin());

            Config::Instance().SetArray("File.RecentFiles", LastOpenFiles);
            Config::Instance()["File.LastOpenPath"] = ImGuiFileDialog::Instance()->GetCurrentPath();
            Config::Instance().Save();

            UIManager::Get().OnEmulationMediaOpen(0, FullMediaPath);

            UpdateRecentMenuItems();
        }
    }
}

IMGUI_UTIL_CREATE_MENU_ITEM("File@0->Open@0", ImGuiMod_Ctrl | ImGuiKey_O, "Open a media source for a emulator code.")
{
    static std::string AllFilters = []()
    {
        std::string Filters;
        for (const std::unique_ptr<IEmulatorCore>& Core : EmulatorCores)
        {
            if (!Filters.empty())
                Filters += ",";
            Filters += Core->GetMediaFilter(0);
        }
        return Filters;
    }();

    ImGuiUtil_OpenModalFileDialog("Open ROM", AllFilters, Config::Instance().Get("File.LastOpenPath", "."), [](const std::string_view& Key)
    {
        using FilePath = std::filesystem::path;
        const std::string FullPath = (FilePath{ImGuiFileDialog::Instance()->GetCurrentPath()} / FilePath{ImGuiFileDialog::Instance()->GetCurrentFileName()}).string();
        const std::string Filter = ImGuiFileDialog::Instance()->GetCurrentFilter();
        StartEmulationWithMedia(FullPath, Filter);
    });
}

IMGUI_UTIL_CREATE_MENU_ITEM("File@0->|Exit", ImGuiMod_Alt | ImGuiKey_F4, "")
{
    ExitApp = true;
}

ImGuiID SettingsWindowID;

void ShowSettingsWindow()
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

        for (const std::unique_ptr<IEmulatorCore>& Emulator : EmulatorCores)
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

        if (CurrentCategory < EmulatorCores.size())
        {
            for (const auto& [Name, Type] : EmulatorCores[CurrentCategory]->GetSettingsTypes())
            {
                std::string ConfigKey = "Core." + EmulatorCores[CurrentCategory]->Name() + "." + Name;

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

#if DEBUG_BUILD
static bool ShowDemoWindow = false;

IMGUI_UTIL_CREATE_MENU_ITEM("About->Show Debug", ImGuiKey_None, "")
{
    ShowDemoWindow = !ShowDemoWindow;
}
#endif

void SetThemeStyle()
{
    // Source from: https://github.com/shivang51/bess/blob/main/Bess/src/settings/themes.cpp#L39
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    // Primary background
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);  // #131318
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f); // #131318

    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

    // Highlights
    colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

    // Style tweaks
    style.WindowRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.PopupBorderSize = 0.f;
}

IMGUI_UTIL_CREATE_MENU_ITEM("File->|Settings@3", ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_S, "")
{
    ImGui::OpenPopup(SettingsWindowID);
}

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "SDL Error: " << SDL_GetError() << '\n';
        return -1;
    }

    Config::Instance().Load();
    InitAudio();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    const float MainScale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_Window* Window = SDL_CreateWindow(
        "Ultipugna",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        static_cast<int>(1280 * MainScale), static_cast<int>(720 * MainScale),
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    SDL_GLContext OpenGLContext = SDL_GL_CreateContext(Window);
    SDL_GL_MakeCurrent(Window, OpenGLContext);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO(); (void)IO;
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    SetThemeStyle();
    ImGuiStyle& Style = ImGui::GetStyle();
    Style.ScaleAllSizes(MainScale);
    Style.FontScaleDpi = MainScale;
    IO.ConfigDpiScaleFonts = true;
    IO.ConfigDpiScaleViewports = true;
    if (IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        Style.WindowRounding = 0.0f;
        Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_InitForOpenGL(Window, OpenGLContext);
    ImGui_ImplOpenGL3_Init("#version 150");

    UIManager::Get().Initialize();
    UpdateRecentMenuItems();

    const std::uint64_t OneSecondFrequency = SDL_GetPerformanceFrequency();

    while (!ExitApp)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            ImGui_ImplSDL2_ProcessEvent(&Event);
            if (Event.type == SDL_QUIT)
                ExitApp = true;

            if (Event.type == SDL_WINDOWEVENT &&
                Event.window.event == SDL_WINDOWEVENT_CLOSE &&
                Event.window.windowID == SDL_GetWindowID(Window))
                ExitApp = true;
        }

        if (SDL_GetWindowFlags(Window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (CurrentEmulatorCore != nullptr)
        {
            const std::uint64_t UpdateInterval = OneSecondFrequency / static_cast<uint64_t>(CurrentEmulatorCore->GetRefreshUpdate());
            const std::uint64_t CurrentEmulatorTick = SDL_GetPerformanceCounter();

            if (CurrentEmulatorTick < CurrentEmulatorLastTick)
            {
                CurrentEmulatorLastTick = CurrentEmulatorTick;
            }

            while (CurrentEmulatorTick - CurrentEmulatorLastTick >= UpdateInterval)
            {
                CurrentEmulatorLastTick += UpdateInterval;
                CurrentEmulatorCore->DoFrame();
            }
        }

        UIManager::Get().Render();
        ShowSettingsWindow();

#if DEBUG_BUILD
        if (ShowDemoWindow)
        {
            ImGui::ShowDemoWindow(&ShowDemoWindow);
        }
#endif

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)IO.DisplaySize.x, (int)IO.DisplaySize.y);
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

    StopEmulatorCore();
    UIManager::Get().Stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(OpenGLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
