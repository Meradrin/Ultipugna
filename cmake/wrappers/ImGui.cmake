# cmake/wrappers/ImGui.cmake
include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../CPM.cmake")

# ImGui (docking)
CPMAddPackage(
        NAME imgui
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG v1.92.2b-docking
)

# GL3W via CPM (lets its own CMake generate/build into gl3w-build/)
CPMAddPackage(
        NAME gl3w
        GITHUB_REPOSITORY skaslev/gl3w
        GIT_TAG master
)

add_library(imgui_sdl_opengl STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui::sdl_opengl ALIAS imgui_sdl_opengl)

target_include_directories(imgui_sdl_opengl PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${gl3w_BINARY_DIR}/include
)

# Use GL3W loader
target_compile_definitions(imgui_sdl_opengl PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W)

# Link the gl3w target produced by its CMake (name can be 'gl3w' or 'gl3w::gl3w')
if(TARGET gl3w::gl3w)
    target_link_libraries(imgui_sdl_opengl PUBLIC gl3w::gl3w)
elseif(TARGET gl3w)
    target_link_libraries(imgui_sdl_opengl PUBLIC gl3w)
else()
    message(FATAL_ERROR "gl3w target not found (expected 'gl3w' or 'gl3w::gl3w').")
endif()

if(TARGET SDL2::SDL2)
    target_link_libraries(imgui_sdl_opengl PUBLIC SDL2::SDL2)
else()
    # Some FindSDL2.cmake modules only provide variables instead of an imported target
    if(NOT SDL2_INCLUDE_DIRS OR NOT SDL2_LIBRARIES)
        message(FATAL_ERROR "SDL2 not found: provide either the target SDL2::SDL2 or SDL2_INCLUDE_DIRS/SDL2_LIBRARIES.")
    endif()
    target_include_directories(imgui_sdl_opengl PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(imgui_sdl_opengl PUBLIC ${SDL2_LIBRARIES})
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(imgui_sdl_opengl PUBLIC OpenGL::GL)