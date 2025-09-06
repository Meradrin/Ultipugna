#include "EmulatorCoreManager.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <SDL.h>

#include "ImGuiFileDialog.h"
#include "UI/ShortcutAndMenuUtils.h"
#include "UI/UIManager.h"
#include "Util/Config.h"

IMGUI_UTIL_CREATE_MENU_ITEM("File@0->Open@0", ImGuiMod_Ctrl | ImGuiKey_O, "Open a media source for a emulator code.")
{
    static std::string AllFilters = []()
    {
        std::string Filters;
        for (const std::unique_ptr<IEmulatorCore>& Core : EmulatorCoreManager::Get().EmulatorCores)
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
        EmulatorCoreManager::Get().StartEmulationWithMedia(FullPath, Filter);
    });
}

EmulatorCoreManager::EmulatorCoreManager()
{
}

EmulatorCoreManager::~EmulatorCoreManager()
{
}

void EmulatorCoreManager::StartEmulatorCore(IEmulatorCore* Core)
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

void EmulatorCoreManager::StopEmulatorCore()
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

bool EmulatorCoreManager::Initialize()
{
    InitAudio();
    RefreshRecentFiles();
    return true;
}

void EmulatorCoreManager::Update()
{
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
}

void EmulatorCoreManager::StartEmulationWithMedia(const std::string& FullMediaPath, const std::string& Filter)
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
            StopEmulation();
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
            Get().RefreshRecentFiles();
        }
    }
}

void EmulatorCoreManager::StopEmulation()
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

void EmulatorCoreManager::RefreshRecentFiles()
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
            Get().StartEmulationWithMedia(RecentFullPath, RecentFilter);
        });
    }
}

void EmulatorCoreManager::InitAudio()
{
    SDL_AudioSpec Desired = {};
    Desired.freq = AudioSampleRate;
    Desired.format = AUDIO_S16;
    Desired.channels = 2;
    Desired.samples = 2048;
    Desired.callback = EmulatorCoreManager::UpdateAudioCallback;
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

void EmulatorCoreManager::PushAudioCallback(std::uint32_t ChannelCount, std::span<std::int16_t> Samples)
{
    if (ChannelCount != 2 || Samples.empty())
        return;

    SDL_LockAudioDevice(Get().AudioDevice);
    Get().AudioBuffer.insert(Get().AudioBuffer.end(), Samples.begin(), Samples.end());
    SDL_UnlockAudioDevice(Get().AudioDevice);
}

void EmulatorCoreManager::UpdateAudioCallback(void*, Uint8* Stream, int Length)
{
    if (Get().AudioBuffer.size() < Length)
    {
        std::memset(Stream, 0, Length);
    }
    else
    {
        std::memcpy(Stream, Get().AudioBuffer.data(), Length);
        const std::uint32_t SampleLength = Length / 2;
        std::uint32_t LengthToRemove = SampleLength;
        while (Get().AudioBuffer.size() - LengthToRemove >= Length * 2)
        {
            LengthToRemove += SampleLength;
        }

        Get().AudioBuffer.erase(Get().AudioBuffer.begin(), Get().AudioBuffer.begin() + static_cast<std::ptrdiff_t>(LengthToRemove));
    }
}

void EmulatorCoreManager::DestroyAudio()
{
}
