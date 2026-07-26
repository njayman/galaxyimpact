cmake_minimum_required(VERSION 3.20)

project(GalaxyImpact)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Building for Web: use the Emscripten SDK's toolchain file, e.g.
#   emcmake cmake -S . -B build-web && cmake --build build-web
# EMSCRIPTEN is set by CMake automatically when that toolchain is active.
if (EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)
FetchContent_MakeAvailable(raylib)

# raygui is a single header (src/raygui.h) - no build target of its own.
# RAYGUI_IMPLEMENTATION is defined in exactly one translation unit
# (src/raygui_impl.cpp) to generate its function bodies once.
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
)
FetchContent_MakeAvailable(raygui)

file(GLOB_RECURSE GALAXYIMPACT_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(GalaxyImpact ${GALAXYIMPACT_SOURCES})

target_include_directories(GalaxyImpact PRIVATE include "${raygui_SOURCE_DIR}/src")

target_link_libraries(GalaxyImpact PRIVATE raylib)

target_compile_options(GalaxyImpact PRIVATE -Wall -Wextra)

if (EMSCRIPTEN)
    # -sASYNCIFY: lets platformInitSaveData/platformSyncSaveData (src/platform.cpp)
    #   block with emscripten_sleep() until the async IndexedDB round-trip
    #   finishes, instead of needing every caller restructured into callbacks.
    # -sFORCE_FILESYSTEM: keeps IDBFS's FS.* JS API linked in even though no
    #   C++ code calls a filesystem function Emscripten can statically see
    #   (the save/load path goes through EM_ASM's inline JS instead).
    # No --preload-file: the game has no asset files to bundle (the font
    # falls back to raylib's built-in default when no system font is found,
    # and all audio is synthesized in sound.cpp, not loaded from disk) - add
    # one here if that changes.
    set_target_properties(GalaxyImpact PROPERTIES SUFFIX ".html")
    target_link_options(GalaxyImpact PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sFORCE_FILESYSTEM=1
        -sALLOW_MEMORY_GROWTH=1
        -lidbfs.js
        --shell-file "${CMAKE_SOURCE_DIR}/web/shell.html"
    )

    # index.html: the landing/store page a browser hits first (see
    # web/landing.html) - its "Play" button links to GalaxyImpact.html, and
    # platformExitToLanding() (src/platform.cpp) navigates back to it when the
    # player exits from the title screen.
    add_custom_command(TARGET GalaxyImpact POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/web/landing.html"
            "$<TARGET_FILE_DIR:GalaxyImpact>/index.html"
    )
endif()
