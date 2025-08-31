cmake_minimum_required(VERSION 3.25)
project(Ultipugna LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

FetchContent_Declare(
        ImGuiFileDialog
        GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(ImGuiFileDialog)

include(cmake/wrappers/ImGui.cmake)
include(cmake/wrappers/LibAsm.cmake)
include(cmake/wrappers/GenesisPlusGx.cmake)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")

add_executable(Ultipugna ${SRC_FILES} "external/imgui_hex_editor/imgui_hex.cpp")

target_compile_features(Ultipugna PRIVATE cxx_std_20)

target_include_directories(Ultipugna PUBLIC "src")
target_include_directories(Ultipugna PRIVATE external/imgui_hex_editor)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Ultipugna PRIVATE DEBUG_BUILD=1)
endif()

target_link_libraries(ImGuiFileDialog PUBLIC imgui::sdl_opengl)

target_link_libraries(Ultipugna PRIVATE
        imgui::sdl_opengl
        ImGuiFileDialog
        libasm::libasm
        SDL2
        genesis-plus-gx::genesis-plus-gx
)
