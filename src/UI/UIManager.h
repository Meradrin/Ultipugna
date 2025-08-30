#pragma once

#include <memory>

#include "IWindow.h"

class UIManager
{
public:
    ~UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    static UIManager& Get() { return Instance; }

    void Initialize();
    void Render();
    void Stop();

    void OnEmulationCoreStart(IEmulatorCore* emulator);
    void OnEmulationCoreStop();

    template<typename WindowType, typename... Args>
    WindowType& AddWindow(Args&&... args)
    {
        static_assert(std::is_base_of_v<IWindow, WindowType>, "WindowType must be from IWindow");
        Windows.push_back(std::make_unique<WindowType>(std::forward<Args>(args)...));
        OnNewWindow(*Windows.back());
        return static_cast<WindowType&>(*Windows.back());
    }

    template <typename WindowType>
    [[nodiscard]] WindowType* GetWindow(std::size_t InstanceIndex = 0) const
    {
        static_assert(std::is_base_of_v<IWindow, WindowType>, "WindowType must be from IWindow");
        for (const auto& Window : Windows)
        {
            if (auto* CastWindow = dynamic_cast<WindowType*>(Window.get()))
            {
                if (InstanceIndex == 0)
                    return CastWindow;
                --InstanceIndex;
            }
        }
        return nullptr;
    }

    template <typename WindowType>
    bool RemoveWindow(std::size_t InstanceIndex = 0)
    {
        static_assert(std::is_base_of_v<IWindow, WindowType>, "WindowType must be from IWindow");
        for (auto WindowIterator = Windows.begin(); WindowIterator != Windows.end(); ++WindowIterator)
        {
            if (dynamic_cast<WindowType*>(WindowIterator->get()))
            {
                if (InstanceIndex == 0)
                {
                    OnRemoveWindow(**WindowIterator);
                    Windows.erase(WindowIterator);
                    return true;
                }
                --InstanceIndex;
            }
        }
        return false;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<IWindow>>& GetWindows() const { return Windows; }

private:
    UIManager() = default;

    void OnNewWindow(IWindow& Window);
    void OnRemoveWindow(IWindow& Window);

    static UIManager Instance;

    std::vector<std::unique_ptr<IWindow>> Windows;
};