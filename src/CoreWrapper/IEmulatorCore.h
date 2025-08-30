#pragma once

#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

enum class EmulatorError {
    Success = 0,
    UnableToLoadTheMediaSource,
};


using RenderCallback = void(*)(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels);
using AudioCallback = void(*)(std::uint32_t NumChannels, std::span<std::int16_t> Samples);

class IEmulatorCore
{
public:
    virtual ~IEmulatorCore() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Reset(bool Hard) = 0;

    virtual std::string GetMediaFilter(int MediaSource) = 0;
    virtual std::error_code InsertMediaSource(std::string_view path, int MediaSource) = 0;
    virtual void RemoveMediaSource(int MediaSource) = 0;

    virtual void PlugController(int Port, int ControllerType) = 0;
    virtual void UnplugController(int Port) = 0;

    virtual void SetControllerInputValue(int Port, int Input, float Value) = 0;
    virtual void SetControllerInputValues(int Port, std::span<float> Values) = 0;

    void SetRenderCallback(const RenderCallback Render) { RenderFunc = Render; };
    void SetAudioCallback(const AudioCallback Audio) { AudioFunc = Audio; };

    virtual double GetRefreshUpdate() = 0;
    virtual void DoFrame() = 0;

    [[nodiscard]] virtual std::vector<std::byte> SaveState() const = 0;
    virtual std::error_code LoadState(std::span<const std::byte> state_data) = 0;

protected:
    RenderCallback RenderFunc = nullptr;
    AudioCallback AudioFunc = nullptr;
};
