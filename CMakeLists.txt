cmake_minimum_required(VERSION 3.25)
project(Ultipugna LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include(cmake/wrappers/ImGui.cmake)
include(cmake/wrappers/LibAsm.cmake)

add_executable(Ultipugna src/main.cpp)

target_link_libraries(Ultipugna PRIVATE
        imgui::sdl_opengl
        libasm::libasm
        SDL2
)
