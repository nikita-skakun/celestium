cmake_minimum_required(VERSION 3.29)
project(Celestium VERSION 1.0 LANGUAGES CXX)

# Ensure CMake has compilers set
if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER ${CC} CACHE STRING "" FORCE)
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${CXX} CACHE STRING "" FORCE)
endif()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify the C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Specify the build type
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

# Include FetchContent module
include(FetchContent)

# Set the base directory for external content
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/external)

# Enable verbose output for FetchContent
set(FETCHCONTENT_QUIET OFF)

# Configure RayLib
set(USE_EXTERNAL_GLFW ON CACHE BOOL "" FORCE)
set(USE_WAYLAND ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)

# Disable examples to speed up build
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_RAYGUI_EXAMPLES OFF CACHE BOOL "" FORCE)

# Automatically find all source files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Add the executable
add_executable(celestium ${SOURCES})

# Ensure the target uses C++23 and C17
set_target_properties(celestium PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CMAKE_C_STANDARD 17
    CMAKE_C_STANDARD_REQUIRED YES
)

# Add extra warnings for the main target
target_compile_options(celestium PRIVATE
    $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Link to installed libraries
find_package(PkgConfig REQUIRED)

pkg_check_modules(OpusFile REQUIRED opusfile)
target_link_libraries(celestium PRIVATE ${OpusFile_LIBRARIES})
include_directories(${OpusFile_INCLUDE_DIRS})

pkg_check_modules(RtAudio REQUIRED rtaudio)
target_link_libraries(celestium PRIVATE ${RtAudio_LIBRARIES})

include_directories(${MAGIC_ENUM_DIR}/include)
include_directories(${TERM_COLOR_DIR}/include)

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
fetch_and_link_dependency(ryml https://github.com/biojppm/rapidyaml.git master)

FetchContent_Declare(
    raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG master
    GIT_SHALLOW OFF
    GIT_PROGRESS TRUE
)

FetchContent_GetProperties(raygui)
if(NOT raygui_POPULATED)
    FetchContent_MakeAvailable(raygui)
    add_subdirectory(${raygui_SOURCE_DIR}/projects/CMake ${raygui_BINARY_DIR})
endif()

target_link_libraries(celestium PRIVATE raygui)

# Disable warnings for raygui, as they are harmless
target_include_directories(celestium SYSTEM PRIVATE ${raygui_SOURCE_DIR}/src)
