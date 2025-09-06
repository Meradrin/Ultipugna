#include "AppFramework.h"
#include "EmulatorCoreManager.h"

int main(int, char**)
{
    AppFramework& App = AppFramework::Get();
    EmulatorCoreManager& EmuManager = EmulatorCoreManager::Get();

    EmuManager.Initialize();

    while (App.IsRunning())
    {
        EmuManager.Update();
        App.DoUpdateUIAndRender();
    }

    return 0;
}
