#pragma once

#include "CoreWrapper/IEmulatorCore.h"

class GenesisPlusGX : public IEmulatorCore
{
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Reset(bool Hard) override;

    virtual std::string GetMediaFilter(int MediaSource) override;
    virtual std::error_code InsertMediaSource(std::string_view Path, int MediaSource)  override;
    virtual void RemoveMediaSource(int MediaSource) override;

    virtual void PlugController(int Port, int Controller_type) override;
    virtual void UnplugController(int Port) override;

    virtual void SetControllerInputValue(int Port, int Input, float Value) override;
    virtual void SetControllerInputValues(int Port, std::span<float> Values) override;

    virtual double GetRefreshUpdate() override;
    virtual void DoFrame() override;

    [[nodiscard]] virtual std::vector<std::byte> SaveState() const override;
    virtual std::error_code LoadState(std::span<const std::byte> StateData) override;

    [[nodiscard]] virtual const std::vector<MemoryRegion>& GetMemoryRegions() const override;
    [[nodiscard]] virtual const std::vector<CPUDescription>& GetCPUs() const override;

private:
    std::vector<std::uint32_t> m_FrameBuffer;
};
