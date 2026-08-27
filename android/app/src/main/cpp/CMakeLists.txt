cmake_minimum_required(VERSION 3.20)

project(GalaxyImpact)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PLATFORM "Android" CACHE STRING "" FORCE)

get_filename_component(GALAXYIMPACT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)
FetchContent_MakeAvailable(raylib)

# raygui is a single header (src/raygui.h) - no build target of its own.
FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
)
FetchContent_MakeAvailable(raygui)

include(${GALAXYIMPACT_ROOT}/cmake/GalaxyImpactSources.cmake)
list(TRANSFORM GALAXYIMPACT_SOURCES PREPEND "${GALAXYIMPACT_ROOT}/")

# Android loads the game as a shared library via android.app.NativeActivity
# (see AndroidManifest.xml's android.app.lib_name meta-data), not as an
# executable - raylib's Android glue calls this library's main() itself.
add_library(GalaxyImpact SHARED ${GALAXYIMPACT_SOURCES})

target_include_directories(GalaxyImpact PRIVATE
    "${GALAXYIMPACT_ROOT}/include"
    "${raygui_SOURCE_DIR}/src"
)

# Lets our own code (touch controls, asset loading) branch on Android at
# compile time, matching raylib's own PLATFORM_ANDROID macro name.
target_compile_definitions(GalaxyImpact PRIVATE PLATFORM_ANDROID)

target_compile_options(GalaxyImpact PRIVATE -Wall -Wextra -Wno-missing-field-initializers)

# The NDK's libc++ still guards std::format behind the "experimental library"
# feature flag (see __config's _LIBCPP_HAS_NO_INCOMPLETE_FORMAT) - without
# this, every std::format call site fails with "no member named 'format'".
target_compile_options(GalaxyImpact PRIVATE -fexperimental-library)
target_link_options(GalaxyImpact PRIVATE -fexperimental-library)

target_link_libraries(GalaxyImpact PRIVATE raylib log android)

# NativeActivity looks up this symbol at load time; raylib's own build only
# applies this flag to its own static archive, not our final .so, so it has
# to be repeated here.
target_link_options(GalaxyImpact PRIVATE "-u" "ANativeActivity_onCreate")
