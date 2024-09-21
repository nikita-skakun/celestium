cmake_minimum_required(VERSION 3.29)
project(Celestium VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify the build type
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

# Include FetchContent module
include(FetchContent)

# Set the base directory for external content
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/external)

# Enable verbose output for FetchContent
set(FETCHCONTENT_QUIET OFF)

# Enable Wayland
set(USE_EXTERNAL_GLFW ON CACHE BOOL "" FORCE)
set(USE_WAYLAND ON CACHE BOOL "" FORCE)
# Disable examples to speed up build
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Automatically find all source files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Add the executable
add_executable(celestium ${SOURCES})

# Add extra warnings and optimizations for the main target
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(celestium PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

# Helper function to fetch, make available, and link dependencies
function(fetch_and_link_dependency name repo tag)
    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY ${repo}
        GIT_TAG ${tag}
        GIT_SHALLOW OFF # Fetch the full repository, not a shallow clone
        GIT_PROGRESS TRUE # Show progress during the Git clone/fetch
    )
    FetchContent_MakeAvailable(${name})
    target_link_libraries(celestium PRIVATE ${name})
endfunction()

# Fetch and link dependencies
fetch_and_link_dependency(raylib https://github.com/raysan5/raylib.git master)
fetch_and_link_dependency(termcolor https://github.com/ikalnytskyi/termcolor.git master)
fetch_and_link_dependency(fmt https://github.com/fmtlib/fmt.git master)
