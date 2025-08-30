cmake_minimum_required(VERSION 3.25)
project(Genesis_Plus_GX LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(ROOT_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/external/genesis-plus-gx")
set(PLATFORM_DIR     "${ROOT_DIR}/sdl")
set(CORE_DIR         "${ROOT_DIR}/core")
set(LIBCHDR_DIR      "${CORE_DIR}/cd_hw/libchdr")

find_package(ZLIB REQUIRED)

# Zstd: try CMake module first, then fallback to pkg-config.
find_package(ZSTD QUIET)
if(NOT ZSTD_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
    else()
        message(FATAL_ERROR "Zstd not found (neither CMake module nor pkg-config). Install libzstd-dev/zstd.")
    endif()
endif()

# libm (math)
find_library(M_LIB m)

# === Sources ===
file(GLOB_RECURSE CORE_SRC_FILES
        "${CORE_DIR}/*.c"
)
list(FILTER CORE_SRC_FILES EXCLUDE REGEX ".*/cd_hw/libchdr/tests/.*")
list(FILTER CORE_SRC_FILES EXCLUDE REGEX ".*/cd_hw/libchdr/deps/zstd-.*")
list(FILTER CORE_SRC_FILES EXCLUDE REGEX ".*/cd_hw/libchdr/deps/zlib-.*")

set(PLATFORM_SRC_FILES
        "${PLATFORM_DIR}/config.c"
        "${PLATFORM_DIR}/error.c"
        "${PLATFORM_DIR}/fileio.c"
        "${PLATFORM_DIR}/unzip.c"
)

add_library(genesis-plus-gx STATIC
        ${CORE_SRC_FILES}
        ${PLATFORM_SRC_FILES}
)

add_library(genesis-plus-gx::genesis-plus-gx ALIAS genesis-plus-gx)

target_compile_definitions(genesis-plus-gx
        PRIVATE
        INLINE=static\ inline
        LSB_FIRST
        USE_32BPP_RENDERING
        USE_LIBCHDR
        HAVE_ZLIB
        HAVE_LZMA
        HAVE_ZSTD
        HOOK_CPU
        MAXROMSIZE=33554432
        HAVE_YM3438_CORE
        Z7_ST
        _7ZIP_ST
)

set(ADD_INC_DIRS
        "${PLATFORM_DIR}"
        "${PLATFORM_DIR}/sdl2"
        "${CORE_DIR}"
        "${CORE_DIR}/m68k"
        "${CORE_DIR}/z80"
        "${CORE_DIR}/input_hw"
        "${CORE_DIR}/sound"
        "${CORE_DIR}/cart_hw"
        "${CORE_DIR}/cart_hw/svp"
        "${CORE_DIR}/cd_hw"
        "${CORE_DIR}/debug"
        "${CORE_DIR}/ntsc"
        "${LIBCHDR_DIR}/include"
        "${LIBCHDR_DIR}/deps/lzma-24.05/include"
)

foreach(dir ${ADD_INC_DIRS})
    if(EXISTS "${dir}")
        target_include_directories(genesis-plus-gx PUBLIC "${dir}")
    endif()
endforeach()

target_link_libraries(genesis-plus-gx
        PUBLIC
        ZLIB::ZLIB
        PRIVATE
        $<$<BOOL:${M_LIB}>:${M_LIB}>
)

if(TARGET ZSTD::ZSTD)
    target_link_libraries(genesis-plus-gx PUBLIC ZSTD::ZSTD)
elseif(TARGET PkgConfig::ZSTD)
    target_link_libraries(genesis-plus-gx PUBLIC PkgConfig::ZSTD)
endif()
