#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "Util/HashUtil.h"
#include "imgui.h"

using MenuCallback = std::function<void()>;

bool ImGuiUtil_AddMenuItem(const std::string& Path, const ImGuiKeyChord& ShortcutKey, const std::string& Description, const MenuCallback& Callback, bool* IsSelected = nullptr);
bool ImGuiUtil_RemoveMenuItem(const std::string& Path);
void ImGuiUtil_DisplayMenuBar();
void ImGuiUtil_UpdateShortcut();

void ImGuiUtil_OpenModalFileDialog(const std::string& Title, const std::string& Filter, const std::string_view& Path, std::function<void(const std::string_view&)> Callback, const std::string& Key = SourceLocationUniqueCStrHexId().data());

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define UNIQUE_ID(base)   CONCAT(base, __COUNTER__)

#define IMGUI_UTIL_CREATE_MENU_ITEM(Path, HotKeys, Description) \
    IMGUI_UTIL_CREATE_MENU_ITEM_IMPL(Path, HotKeys, Description, UNIQUE_ID(MenuItemFunc_))

#define IMGUI_UTIL_CREATE_MENU_ITEM_IMPL(Path, HotKeys, Description, Name) \
    static void Name(); \
    static bool CONCAT(IsAdded_, Name) = ImGuiUtil_AddMenuItem(Path, HotKeys, Description, &Name); \
    static void Name()

bool ImGuiUtil_ComboAutoWidth(const char* Label, int* CurrentItem, const char* ItemsZeroSep, int HeightItems = -1);
