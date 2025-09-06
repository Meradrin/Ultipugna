#pragma once

#include <array>
#include <memory>
#include <SDL_audio.h>
#include <SDL_timer.h>

#include "CoreWrapper/GenesisPlusGX.h"
#include "CoreWrapper/IEmulatorCore.h"

class EmulatorCoreManager
{
public:
    static EmulatorCoreManager& Get() { static EmulatorCoreManager Instance; return Instance; }

    std::array<std::unique_ptr<IEmulatorCore>, 1> EmulatorCores =
    {
        std::make_unique<GenesisPlusGX>(),
    };

    EmulatorCoreManager(const EmulatorCoreManager&) = delete;
    EmulatorCoreManager& operator=(const EmulatorCoreManager&) = delete;
    ~EmulatorCoreManager();

    bool Initialize();
    void Update();

    void StartEmulationWithMedia(const std::string& FullMediaPath, const std::string& Filter);
    void StopEmulation();

    [[nodiscard]] const IEmulatorCore* CurrentCore() const { return CurrentEmulatorCore; }

private:
    EmulatorCoreManager();

    void StartEmulatorCore(IEmulatorCore* Core);
    void StopEmulatorCore();

    void InitAudio();
    static void PushAudioCallback(std::uint32_t ChannelCount, std::span<std::int16_t> Samples);
    static void UpdateAudioCallback(void*, Uint8* Stream, int Length);
    void DestroyAudio();

    void RefreshRecentFiles();

    const std::uint64_t OneSecondFrequency = SDL_GetPerformanceFrequency();

    IEmulatorCore* CurrentEmulatorCore = nullptr;
    std::uint64_t CurrentEmulatorLastTick = 0;

    bool UpdateAudio = true;
    int AudioSampleRate = 48000;
    std::vector<int16_t> AudioBuffer;
    SDL_AudioDeviceID AudioDevice;
};
