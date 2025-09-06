#include "MemoryViewerWindow.h"

#include "UI/ShortcutAndMenuUtils.h"
#include "Util/ImGuiMathUtil.h"

std::uint64_t MemoryViewerWindow::TypeId()
{
    return StaticTypeId();
}

void MemoryViewerWindow::OnEmulationMediaOpen(std::uint32_t MediaSource, const std::string& MediaPath)
{
    const std::vector<MemoryRegion>& MemoryRegions = IEmulatorCore::Current()->GetMemoryRegions();

    MemoryRegionNames = "";

    for (const MemoryRegion& MemoryRegion : MemoryRegions)
    {
        MemoryRegionNames += MemoryRegion.Name;
        MemoryRegionNames += '\0';
    }
}

void MemoryViewerWindow::OnEmulationCoreStop()
{
    MemEditorState.ReadCallback = nullptr;
    MemEditorState.WriteCallback = nullptr;
    MemRegion = nullptr;
}

const std::string& MemoryViewerWindow::Title()
{
    static std::string Title = "Memory Viewer";
    return Title;
}

void MemoryViewerWindow::Render()
{
    SelectMemoryRegion(SelectedMemoryRegion);

    ImGui::Begin(Title().c_str());

    if (MemRegion != nullptr && MemEditorState.ReadCallback != nullptr)
    {
        bool bInside = false;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Memory Region:");
        ImGui::SameLine();
        ImGuiUtil_ComboAutoWidth("##SelectedMemoryRegion", &SelectedMemoryRegion, MemoryRegionNames.c_str());
        ImGui::Separator();

        ImGui::BeginHexEditor("HexEditor", &MemEditorState);

        if (ImGui::IsWindowHovered())
        {
            const ImVec2 ContentPos = ImGui::GetMousePos() - ImGui::GetWindowPos();
            const ImVec2 ContentLimit = ImGui::GetWindowSize();

            if (ContentPos.x >= 0 && ContentPos.y >= 0 && ContentPos.x < ContentLimit.x && ContentPos.y < ContentLimit.y)
            {
                //ImGui::SetTooltip("%d, %d", static_cast<std::uint32_t>(ContentPos.x), static_cast<std::uint32_t>(ContentPos.y));
                bInside = true;
            }
        }

        ImGui::EndHexEditor();

        if (bInside && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) // touche "Menu" de certains claviers
        {
            ImGui::OpenPopup("ItemCtx");
        }

        const ImGuiID confirm_id = ImGui::GetID("ConfirmDelete");

        if (ImGui::BeginPopup("ItemCtx"))
        {
            if (ImGui::MenuItem("Ajouter noeud"))
            {
                ImGui::OpenPopup(confirm_id);

            }
            ImGui::Separator();
            if (ImGui::Button("Coller")) { ; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Es-tu sûr de vouloir supprimer ce fichier ?");
            ImGui::Separator();
            if (ImGui::Button("Oui", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup(); // ferme la boîte
            }
            ImGui::SameLine();
            if (ImGui::Button("Non", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup(); // ferme la boîte
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void MemoryViewerWindow::SelectMemoryRegion(std::size_t Index)
{
    if (const IEmulatorCore* EmulatorCore = IEmulatorCore::Current())
    {
        const std::vector<MemoryRegion>& MemRegions = EmulatorCore->GetMemoryRegions();

        if (Index < MemRegions.size() && MemRegion != &MemRegions[Index])
        {
            SelectedMemoryRegion = Index;
            MemRegion = &MemRegions[SelectedMemoryRegion];
            MemEditorState.BytesPerLine = 16;
            MemEditorState.MaxBytes = static_cast<int>(MemRegion->EndAddress - MemRegion->StartAddress + 1ull);
            MemEditorState.UserData = const_cast<MemoryRegion*>(MemRegion);
            MemEditorState.AddressChars = static_cast<int>(MemRegion->AddressBits / 4);
            MemEditorState.ReadCallback = [](ImGuiHexEditorState* State, int Offset, void* Buffer, int Size) -> int
            {
                if (const auto Mem = static_cast<const MemoryRegion*>(State->UserData))
                {
                    for (std::byte& Byte : std::span(static_cast<std::byte*>(Buffer), Size))
                        Byte = Mem->Read(Offset++);

                    return Size;
                }

                return 0;
            };
            MemEditorState.WriteCallback = [](ImGuiHexEditorState* State, int Offset, void* Buffer, int Size) -> int
            {
                if (const auto Mem = static_cast<const MemoryRegion*>(State->UserData))
                {
                    for (const std::byte& Byte : std::span(static_cast<std::byte*>(Buffer), Size))
                        Mem->Write(Offset++, Byte);

                    return Size;
                }

                return 0;
            };
        }

        return;
    }

    MemRegion = nullptr;
    MemEditorState.UserData = nullptr;
    MemEditorState.ReadCallback = nullptr;
    MemEditorState.WriteCallback = nullptr;
    SelectedMemoryRegion = 0;
}
