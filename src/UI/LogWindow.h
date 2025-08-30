#pragma once

#include "IWindow.h"
#include "Util/HashUtil.h"

class LogWindow final : public IWindow
{
public:
    static consteval std::uint64_t StaticTypeId() { return SourceLocationUniqueId64(); }

    LogWindow();
    virtual ~LogWindow() override;

    virtual std::uint64_t TypeId() override;

    virtual void OnEmulationCoreStart(IEmulatorCore* emulator) override;
    virtual void OnEmulationCoreStop() override;

    virtual const std::string& Title() override;
    virtual void Render() override;

private:
    void StartLogCapture();
    void ReadLogCapture();
    void ShutdownLogCapture();

    int LogRedirectPipe = -1;
    std::string Logs;
    std::vector<std::size_t> Lines;
    std::size_t SelectionStart = 0;
    std::size_t SelectionEnd = 0;
};

