cmake_minimum_required(VERSION 3.28)
project(TheShadesBelowProject LANGUAGES CXX)

message(STATUS "Building for architecture: ${TARGET_TRIPLET}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
# message(STATUS "Loading toolchain file: ${CMAKE_SOURCE_DIR}/scripts/cmake-${TARGET_TRIPLET}.cmake")
# include(${CMAKE_SOURCE_DIR}/scripts/cmake-${TARGET_TRIPLET}.cmake)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    # Points ccache at the project-local .ccache.conf (enables sloppiness needed
    # to cache compiles that use target_precompile_headers) instead of relying
    # on each machine's global ccache config.
    set(CCACHE_LAUNCHER "${CMAKE_COMMAND}" -E env "CCACHE_CONFIGPATH=${CMAKE_SOURCE_DIR}/.ccache.conf" "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_LAUNCHER})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_LAUNCHER})
else()
    message(STATUS "ccache not found - compilation caching disabled")
endif()

include(FetchContent)

set(SFML_USE_SYSTEM_DEPS OFF CACHE BOOL "Use system dependencies" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 8835b6b955c96fa1ad1278a8dae6f05c4143effc    # v3.1.0
    GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)     

FetchContent_Declare(SPDLog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG 6fa36017cfd5731d617e1a934f0e5ea9c4445b13    # v1.15.3
    GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)

FetchContent_Declare(EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG d4014c74dc3793aba95ae354d6e23a026c2796db    # v3.15.0
    EXCLUDE_FROM_ALL SYSTEM
)

FetchContent_Declare(ImGUI
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1
    GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM
)

FetchContent_Declare(imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG        v3.0
)

FetchContent_Declare(json-nlohmann
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
  GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM
)

# Tracy profiler client. Fetched unconditionally (like the other deps above) so
# `#include <tracy/Tracy.hpp>` always resolves, but TRACY_ENABLE gates whether its
# macros compile to real instrumentation or no-ops - flip -DENABLE_TRACY=ON to profile.
# TRACY_ON_DEMAND defers any profiling work/allocation until a Tracy profiler GUI
# actually connects, so an un-profiled run has near-zero overhead even when enabled.
option(ENABLE_TRACY "Build with Tracy profiler instrumentation" OFF)
set(TRACY_ENABLE ${ENABLE_TRACY} CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
FetchContent_Declare(tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
    GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM
)

set(IMGUI_DIR ${FETCHCONTENT_BASE_DIR}/imgui-src)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
FetchContent_MakeAvailable(
    SFML
    SPDLog
    EnTT
    ImGUI
    imgui-sfml
    json-nlohmann
    tracy
)

# ThirdParty Lib warning Supression:
# 1. spdlog-src/include/spdlog/logger.h:317:10: error: 
# variable 'args' might be clobbered by 'longjmp' or 'vfork' 
target_compile_options(spdlog INTERFACE -Wno-clobbered)
# 2. flac-src/src/share/win_utf8_io/win_utf8_io.c:132:29: warning:
# "cast between incompatible function types from 'FARPROC' to int"
target_compile_options(FLAC PRIVATE -Wno-cast-function-type)
# 3. imgui-sfml-src/imgui-SFML.cpp: warning:
# 'sf::Touch::isDown'/'sf::Touch::getPosition' is deprecated (SFML 3 event API)
target_compile_options(ImGui-SFML PRIVATE -Wno-deprecated-declarations)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory( ${CMAKE_SOURCE_DIR}/src )



# Copy resources to binary dir
file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR}/bin
     PATTERN "res/shaders/unused" EXCLUDE )