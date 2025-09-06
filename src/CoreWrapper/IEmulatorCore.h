#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <system_error>
#include <vector>

enum class EmulatorError
{
    Success = 0,
    UnableToLoadTheMediaSource,
};

using RenderCallback = void(*)(std::uint32_t Width, std::uint32_t Height, std::span<std::uint32_t> Pixels);
using AudioCallback = void(*)(std::uint32_t NumChannels, std::span<std::int16_t> Samples);

struct MemoryRegion
{
    std::string Name;
    std::uint32_t AddressBits;
    std::uint64_t StartAddress;
    std::uint64_t EndAddress ;
    std::function<std::byte(std::uint64_t)> Read;
    std::function<void(std::uint64_t, std::byte)> Write;
};

struct TileInfo
{
    std::function<std::vector<std::array<std::uint32_t, 256>>()> GetPreview;
};

struct RegisterDescription
{
    std::vector<std::string> RegisterNames;

    std::function<const std::string&(std::size_t RegisterIndex)> GetRegisterValue;
    std::function<void(std::size_t RegisterIndex, const std::string& RegisterValue)> SetRegisterValue;

    std::function<std::uint64_t()> ExecutionAddress;
    std::function<std::uint64_t()> StackAddress;
};

enum class EndiannessType
{
    Little,
    Big,
};

struct CPUDescription
{
    std::string Name;
    EndiannessType Endianness;
    MemoryRegion* Bus;
    RegisterDescription Registers;
    std::function<std::string(std::uint64_t Address, std::uint64_t Flags)> Disassemble;
    std::function<std::string(std::uint64_t Address, std::uint64_t Flags)> FormatedDisassemble;
};

enum class SettingType
{
    String,
    Integer,
    Float,
    Boolean,
    Directory,
    File,
};

class IEmulatorCore
{
public:
    static IEmulatorCore* Current();
    static void SetCurrent(IEmulatorCore* Core);

    virtual ~IEmulatorCore() = default;

    [[nodiscard]] virtual const std::string& Name() const = 0;

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

    [[nodiscard]] virtual const std::map<std::string, SettingType>& GetSettingsTypes() const = 0;

    [[nodiscard]] const std::string& GetSettingValue(const std::string& SettingName) const;
    void SetSettingValue(const std::string& SettingName, const std::string& Value) const;

    [[nodiscard]] virtual const std::vector<MemoryRegion>& GetMemoryRegions() const;
    [[nodiscard]] virtual const std::vector<CPUDescription>& GetCPUs() const;

    [[nodiscard]] virtual const std::vector<std::array<std::uint32_t, 256>>& GetTilePreviewPalettes() const;

protected:
    RenderCallback RenderFunc = nullptr;
    AudioCallback AudioFunc = nullptr;

    static IEmulatorCore* CurrentCore;
};
