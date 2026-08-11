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

# raylib defaults its bundled GLFW to X11-only on Linux. Force Wayland support
# on too, so the game runs as a native Wayland client (correct output
# resolution, fullscreen, and mouse coordinates) under Wayland compositors
# (sway, etc.) instead of falling back to a resolution-constrained XWayland
# surface.
if (UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()

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

set(GALAXYIMPACT_SOURCES
    src/achievements.cpp
    src/bestiary.cpp
    src/democonfig.cpp
    src/draw.cpp
    src/entities/boss.cpp
    src/entities/enemy.cpp
    src/entities/player.cpp
    src/entities/space.cpp
    src/game.cpp
    src/guitheme.cpp
    src/highscore.cpp
    src/main.cpp
    src/menu.cpp
    src/platform.cpp
    src/raygui_impl.cpp
    src/sandbox.cpp
    src/settings.cpp
    src/sound.cpp
    src/update.cpp
)

add_executable(GalaxyImpact ${GALAXYIMPACT_SOURCES})

target_include_directories(GalaxyImpact PRIVATE include "${raygui_SOURCE_DIR}/src")

target_link_libraries(GalaxyImpact PRIVATE raylib)

target_compile_options(GalaxyImpact PRIVATE -Wall -Wextra -Wno-missing-field-initializers)

if (EMSCRIPTEN)
    set_target_properties(GalaxyImpact PROPERTIES SUFFIX ".html")
    target_link_options(GalaxyImpact PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sFORCE_FILESYSTEM=1
        -sALLOW_MEMORY_GROWTH=1
        -lidbfs.js
        --shell-file "${CMAKE_SOURCE_DIR}/web/shell.html"
        --preload-file "${CMAKE_SOURCE_DIR}/assets@assets"
    )

    add_custom_command(TARGET GalaxyImpact POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_SOURCE_DIR}/web/landing.html"
            "$<TARGET_FILE_DIR:GalaxyImpact>/index.html"
    )
else()
    add_custom_command(TARGET GalaxyImpact POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GalaxyImpact>/assets"
    )
endif()
