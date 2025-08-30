#include "UI/LogWindow.h"

#include <iostream>
#include <functional>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "imgui.h"

using LoggerCallback = std::function<void(std::string_view, bool)>;

LogWindow::LogWindow()
{
    StartLogCapture();
}

LogWindow::~LogWindow()
{
    ShutdownLogCapture();
}

std::uint64_t LogWindow::TypeId()
{
    return StaticTypeId();
}

void LogWindow::OnEmulationCoreStart(IEmulatorCore* emulator)
{

}

void LogWindow::OnEmulationCoreStop()
{

}

const std::string& LogWindow::Title()
{
    static std::string Title = "Log Window";
    return Title;
}

void LogWindow::Render()
{
    ReadLogCapture();

    ImGui::Begin(Title().c_str());

    ImGui::BeginChild("LogRegion", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);

    const float LineHeight = ImGui::GetTextLineHeight();

    ImGuiListClipper Clipper;
    Clipper.Begin(static_cast<int>(Lines.size()) + 1, LineHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    //draw_list->AddRectFilled(marker_min, marker_max, IM_COL32(255, 0, 255, 255));

    while (Clipper.Step())
    {
        for (int LineIndex = Clipper.DisplayStart; LineIndex < Clipper.DisplayEnd; LineIndex++)
        {
            const char* LineStart = Logs.c_str() + (LineIndex == 0 ? 0 : Lines[LineIndex - 1]);
            const char* LineEnd = Logs.c_str() + (LineIndex + 1 < Lines.size() ? Lines[LineIndex] : Logs.size());

            ImGui::TextUnformatted(LineStart, LineEnd);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                const ImVec2 ClickPos = ImGui::GetMousePos();
                const ImVec2 RectMin = ImGui::GetItemRectMin();
                std::cout << "Clicked at " << (ClickPos.x - RectMin.x) << ", " << (ClickPos.y - RectMin.y) << '\n';
            }
        }
    }
    Clipper.End();

    ImGui::EndChild();
    ImGui::End();
}

void LogWindow::StartLogCapture()
{
    int PipeDescriptor[2];

    pipe(PipeDescriptor);
    dup2(PipeDescriptor[1], STDOUT_FILENO);
    dup2(PipeDescriptor[1], STDERR_FILENO);
    close(PipeDescriptor[1]);

    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    std::ios::sync_with_stdio(true);
    std::cout.setf(std::ios::unitbuf);

    LogRedirectPipe = PipeDescriptor[0];

    int flags = fcntl(LogRedirectPipe, F_GETFL, 0);
    fcntl(LogRedirectPipe, F_SETFL, flags | O_NONBLOCK);
}

void LogWindow::ReadLogCapture()
{
    char Buffer[0x4000];

    if (const ssize_t ReadCount = read(LogRedirectPipe, &Buffer, sizeof(Buffer) - 1); ReadCount > 0)
    {
        Buffer[ReadCount] = '\0';

        for (const char* Character = Buffer; *Character; ++Character)
        {
            Logs.push_back(*Character);
            if (*Character == '\n')
                Lines.emplace_back(Logs.size());
        }
    }
}

void LogWindow::ShutdownLogCapture()
{
    close(LogRedirectPipe);
    LogRedirectPipe = -1;
}
