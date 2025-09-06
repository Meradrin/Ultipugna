#include "UI/ShortcutAndMenuUtils.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <ranges>
#include "imgui_internal.h"
#include "ImGuiFileDialog.h"
#include "Util/StringUtil.h"

namespace
{
    struct MenuNode
    {
        std::int32_t Priority = std::numeric_limits<std::int32_t>::max();
        std::string Name;
        std::string Description;
        std::string Shortcut;
        ImGuiKeyChord ShortcutKey = ImGuiKey_None;
        MenuCallback Callback = nullptr;
        std::vector<MenuNode> Children;
        bool* IsSelected = nullptr;
        bool AddSeparatorBefore = false;
    };

    // Make sure this follows the construct-on-first-use idiom for MenuNode,
    // since they are initialized at the same time as other static variables.
    std::vector<MenuNode>& GetMenuNodes() { static std::vector<MenuNode> Nodes; return Nodes; }

    std::map<std::string, std::function<void(std::string_view)>> FileDialogIdAndCallback;

    MenuNode& FindOrAddMenuNode(const std::string_view Name, const std::int32_t Priority, MenuNode* Parent = nullptr)
    {
        auto MenuLess = [](const MenuNode& A, const MenuNode& B)
        {
            if (A.Priority != B.Priority)
                return A.Priority < B.Priority;
            return A.Name < B.Name;
        };

        std::vector<MenuNode>& Nodes = (Parent ? Parent->Children : GetMenuNodes());

        if (auto ItByName = std::ranges::find(Nodes, Name, &MenuNode::Name); ItByName != Nodes.end())
        {
            if (ItByName->Priority > Priority)
            {
                MenuNode Temp = std::move(*ItByName);
                Temp.Priority = Priority;
                Nodes.erase(ItByName);
                Nodes.insert(std::ranges::lower_bound(Nodes, Temp, MenuLess), std::move(Temp));
                ItByName = std::ranges::find(Nodes, Name, &MenuNode::Name);
            }
            return *ItByName;
        }

        MenuNode NewNode;
        NewNode.Name = Name;
        NewNode.Priority = Priority;
        return *Nodes.insert(std::ranges::lower_bound(Nodes, NewNode, MenuLess), std::move(NewNode));
    }

    MenuNode* FindMenuNode(const std::string_view Name, MenuNode* Parent = nullptr)
    {
        std::vector<MenuNode>& Nodes = (Parent ? Parent->Children : GetMenuNodes());
        const auto NodeItr = std::ranges::find(Nodes, Name, &MenuNode::Name);
        return NodeItr != Nodes.end() ? &*NodeItr : nullptr;
    }

    bool RemoveMenuNode(const std::string_view Name, MenuNode* Parent = nullptr)
    {
        std::vector<MenuNode>& Nodes = (Parent ? Parent->Children : GetMenuNodes());
        if (const auto NodeItr = std::ranges::find(Nodes, Name, &MenuNode::Name); NodeItr != Nodes.end())
        {
            Nodes.erase(NodeItr);
            return true;
        }
        return false;
    }

    void RenderMenuNodeAndChildren(const MenuNode& Node)
    {
        if (Node.AddSeparatorBefore)
        {
            ImGui::Separator();
        }

        if (Node.Children.empty())
        {
            if (ImGui::MenuItem(Node.Name.c_str(), Node.Shortcut.c_str(), Node.IsSelected))
            {
                if (Node.Callback)
                    Node.Callback();
            }
        }
        else
        {
            if (ImGui::BeginMenu(Node.Name.c_str()))
            {
                for (const MenuNode& Child : Node.Children)
                {
                    RenderMenuNodeAndChildren(Child);
                }

                ImGui::EndMenu();
            }
        }

        if (!Node.Description.empty() && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(Node.Description.c_str());
            ImGui::EndTooltip();
        }
    }

    void UpdateShortcut(MenuNode& Node)
    {
        if (Node.ShortcutKey != ImGuiKey_None)
        {
            if (Node.Shortcut.empty())
            {
                Node.Shortcut = ImGui::GetKeyChordName(Node.ShortcutKey);
            }

            if (ImGui::Shortcut(Node.ShortcutKey, ImGuiInputFlags_RouteGlobal))
            {
                Node.Callback();
            }
        }

        for (MenuNode& Child : Node.Children)
        {
            UpdateShortcut(Child);
        }
    }
}

bool ImGuiUtil_AddMenuItem(const std::string& Path, const ImGuiKeyChord& ShortcutKey, const std::string& Description, const MenuCallback& Callback, bool* IsSelected)
{
    MenuNode* Parent = nullptr;
    const auto SplitPath = Path | std::views::split("->"sv) | AsStringView | ToVector;

    for (const std::string_view& MenuItem : SplitPath)
    {
        auto Params = MenuItem | std::views::split("@"sv) | AsStringView | ToVector;
        bool AddSeparator = false;

        if (Params[0][0] == '|')
        {
            Params[0] = Params[0].substr(1);
            AddSeparator = true;
        }

        std::int32_t Priority = std::numeric_limits<std::int32_t>::max();
        Params.size() > 1 && StringToNumber(Params.back(), Priority);
        MenuNode& Node = FindOrAddMenuNode(Params[0], Priority, Parent);
        Node.AddSeparatorBefore = AddSeparator;

        if (&SplitPath.back() == &MenuItem)
        {
            Node.Description = Description;
            Node.ShortcutKey = ShortcutKey;
            Node.Callback = Callback;
            Node.IsSelected = IsSelected;
        }

        Parent = &Node;
    }

    return true;
}

bool ImGuiUtil_RemoveMenuItem(const std::string& Path)
{
    MenuNode* Parent = nullptr;
    const auto SplitPath = Path | std::views::split("->"sv) | AsStringView | ToVector;

    for (const std::string_view& MenuItem : SplitPath)
    {
        auto Params = MenuItem | std::views::split("@"sv) | AsStringView | ToVector;

        std::int32_t Priority = std::numeric_limits<std::int32_t>::max();
        Params.size() > 1 && StringToNumber(Params.back(), Priority);

        if (&SplitPath.back() == &MenuItem)
        {
            return RemoveMenuNode(Params[0], Parent);
        }
        else
        {
            Parent = FindMenuNode(Params[0], Parent);

            if (Parent == nullptr)
            {
                return false;
            }
        }
    }

    return false;
}

void ImGuiUtil_DisplayMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        for (const MenuNode& Node : GetMenuNodes())
        {
            RenderMenuNodeAndChildren(Node);
        }

        ImGui::EndMainMenuBar();
    }

    for (const auto& [Key, Callback] : FileDialogIdAndCallback)
    {
        if (ImGuiFileDialog::Instance()->IsOpened(Key))
        {
            const ImVec2 Center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Appearing);

            if (ImGuiFileDialog::Instance()->Display(Key))
            {
                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    Callback(Key);
                }

                ImGuiFileDialog::Instance()->Close();
            }
        }
    }
}

void ImGuiUtil_UpdateShortcut()
{
    ImGui::Begin("##ShortcutsHost", nullptr,
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoInputs    |
                         ImGuiWindowFlags_NoSavedSettings);

    for (MenuNode& Node : GetMenuNodes())
    {
        UpdateShortcut(Node);
    }

    ImGui::End();
}


void ImGuiUtil_OpenModalFileDialog(const std::string& Title, const std::string& Filter, const std::string_view& Path, std::function<void(const std::string_view&)> Callback, const std::string& Key)
{
    IGFD::FileDialogConfig Config;

    Config.path = Path;
    Config.flags = ImGuiFileDialogFlags_Modal;

    ImGuiFileDialog::Instance()->OpenDialog(Key, Title, Filter.c_str(), Config);
    FileDialogIdAndCallback[Key] = Callback;
}

bool ImGuiUtil_ComboAutoWidth(const char* Label, int* CurrentItem, const char* ItemsZeroSep, const int HeightItems)
{
    const ImGuiStyle& Style = ImGui::GetStyle();
    float MaxTextWidth = 0.0f;

    for (const char* Item = ItemsZeroSep; *Item; )
    {
        MaxTextWidth = ImMax(MaxTextWidth, ImGui::CalcTextSize(Item).x);
        Item += std::strlen(Item) + 1;
    }

    ImGui::PushItemWidth(MaxTextWidth + Style.FramePadding.x * 2 + ImGui::GetFrameHeight());
    const bool Changed = ImGui::Combo(Label, CurrentItem, ItemsZeroSep, HeightItems);
    ImGui::PopItemWidth();
    return Changed;
}
