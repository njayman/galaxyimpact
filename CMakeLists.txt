cmake_minimum_required(VERSION 3.20)

project(GalaxyImpact)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE GALAXYIMPACT_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(GalaxyImpact ${GALAXYIMPACT_SOURCES})

target_include_directories(GalaxyImpact PRIVATE include)

target_link_libraries(GalaxyImpact PRIVATE raylib)
