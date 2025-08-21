# cmake/wrappers/LibAsm.cmake
include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../CPM.cmake")

set(LIBASM_GIT_TAG "main" CACHE STRING "Git tag/branch/commit for tgtakaoka/libasm")

CPMAddPackage(
        NAME libasm_src
        GITHUB_REPOSITORY tgtakaoka/libasm
        GIT_TAG ${LIBASM_GIT_TAG}
)

file(GLOB_RECURSE LIBASM_SOURCES
        "${libasm_src_SOURCE_DIR}/src/*.cpp"
)

if(LIBASM_SOURCES STREQUAL "")
    message(FATAL_ERROR "No libasm sources found under ${libasm_src_SOURCE_DIR}/src")
endif()

add_library(libasm_thirdparty STATIC ${LIBASM_SOURCES})
add_library(libasm::libasm ALIAS libasm_thirdparty)

target_include_directories(libasm_thirdparty PUBLIC
        "${libasm_src_SOURCE_DIR}/src"
)

target_compile_features(libasm_thirdparty PUBLIC cxx_std_14)

if(MSVC)
    target_compile_options(libasm_thirdparty PRIVATE /W3)
else()
    target_compile_options(libasm_thirdparty PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
