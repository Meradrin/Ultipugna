#include "UI/IWindow.h"

void IWindow::OnEmulationCoreStart(IEmulatorCore* Emulator)
{
}

void IWindow::OnEmulationCoreStop()
{
}

void IWindow::OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath)
{
}

void IWindow::OnEmulationMediaClose(std::uint32_t MediaSource)
{
}
