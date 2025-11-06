cmake_minimum_required(VERSION 3.29)
project(Celestium VERSION 1.0 LANGUAGES C CXX)

# Ensure CMake has compilers set
if(NOT DEFINED CMAKE_C_COMPILER)
    if(DEFINED CC)
        set(CMAKE_C_COMPILER ${CC} CACHE STRING "" FORCE)
    else()
        set(CMAKE_C_COMPILER /usr/bin/clang CACHE STRING "" FORCE)
    endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    if(DEFINED CXX)
        set(CMAKE_CXX_COMPILER ${CXX} CACHE STRING "" FORCE)
    else()
        set(CMAKE_CXX_COMPILER /usr/bin/clang++ CACHE STRING "" FORCE)
    endif()
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

# --- magic_enum ---
set(MAGIC_ENUM_DIR ${CMAKE_SOURCE_DIR}/external/magic_enum)
file(MAKE_DIRECTORY ${MAGIC_ENUM_DIR})
execute_process(COMMAND git -c protocol.version=2 clone --filter=blob:none --no-checkout --depth 1 --branch master https://github.com/Neargye/magic_enum.git ${MAGIC_ENUM_DIR})
execute_process(COMMAND git -C ${MAGIC_ENUM_DIR} sparse-checkout init --cone)
execute_process(COMMAND git -C ${MAGIC_ENUM_DIR} sparse-checkout set include)
execute_process(COMMAND git -C ${MAGIC_ENUM_DIR} checkout)
target_include_directories(celestium SYSTEM PRIVATE ${MAGIC_ENUM_DIR}/include)

# --- raylib ---
set(RAYLIB_SRC_DIR ${CMAKE_SOURCE_DIR}/external/raylib)
file(MAKE_DIRECTORY ${RAYLIB_SRC_DIR})
execute_process(COMMAND git -c protocol.version=2 clone --filter=blob:none --no-checkout --depth 1 --branch master https://github.com/raysan5/raylib.git ${RAYLIB_SRC_DIR})
execute_process(COMMAND git -C ${RAYLIB_SRC_DIR} sparse-checkout init --cone)
execute_process(COMMAND git -C ${RAYLIB_SRC_DIR} sparse-checkout set src cmake)
execute_process(COMMAND git -C ${RAYLIB_SRC_DIR} checkout)
add_subdirectory(${RAYLIB_SRC_DIR} ${CMAKE_SOURCE_DIR}/external/raylib-build)
target_link_libraries(celestium PRIVATE raylib)

# --- raygui ---
set(RAYGUI_SRC_DIR ${CMAKE_SOURCE_DIR}/external/raygui)
file(MAKE_DIRECTORY ${RAYGUI_SRC_DIR})
execute_process(COMMAND git -c protocol.version=2 clone --filter=blob:none --no-checkout --depth 1 --branch master https://github.com/raysan5/raygui.git ${RAYGUI_SRC_DIR})
execute_process(COMMAND git -C ${RAYGUI_SRC_DIR} sparse-checkout init --cone)
execute_process(COMMAND git -C ${RAYGUI_SRC_DIR} sparse-checkout set src projects/CMake)
execute_process(COMMAND git -C ${RAYGUI_SRC_DIR} checkout)
add_subdirectory(${RAYGUI_SRC_DIR}/projects/CMake ${CMAKE_SOURCE_DIR}/external/raygui-build)
target_link_libraries(celestium PRIVATE raygui)
target_include_directories(celestium SYSTEM PRIVATE ${RAYGUI_SRC_DIR}/src) # Disable warnings for raygui, as they are harmless

# --- LuaJIT v2.1 ---
set(LUAJIT_SRC_DIR ${CMAKE_SOURCE_DIR}/external/luajit)
file(MAKE_DIRECTORY ${LUAJIT_SRC_DIR})
execute_process(COMMAND git -c protocol.version=2 clone --filter=blob:none --no-checkout --depth 1 --branch v2.1 https://github.com/LuaJIT/LuaJIT.git ${LUAJIT_SRC_DIR})
execute_process(COMMAND git -C ${LUAJIT_SRC_DIR} sparse-checkout init --cone)
execute_process(COMMAND git -C ${LUAJIT_SRC_DIR} sparse-checkout set src dynasm || true)
execute_process(COMMAND git -C ${LUAJIT_SRC_DIR} checkout)
add_custom_command(OUTPUT ${LUAJIT_SRC_DIR}/src/libluajit.a COMMAND make -C ${LUAJIT_SRC_DIR}/src WORKING_DIRECTORY ${LUAJIT_SRC_DIR} COMMENT "Building LuaJIT static library" VERBATIM)
add_custom_target(luajit-build ALL DEPENDS ${LUAJIT_SRC_DIR}/src/libluajit.a)
add_library(luajit_static STATIC IMPORTED GLOBAL)
set_target_properties(luajit_static PROPERTIES IMPORTED_LOCATION ${LUAJIT_SRC_DIR}/src/libluajit.a)
add_dependencies(celestium luajit-build)
target_link_libraries(celestium PRIVATE luajit_static)
target_include_directories(celestium PRIVATE ${LUAJIT_SRC_DIR}/src)

# Helper function to fetch, make available, and link dependencies
function(fetch_and_link_dependency name repo tag)
    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY ${repo}
        GIT_TAG ${tag}
        GIT_SHALLOW ON
        GIT_PROGRESS OFF
    )
    FetchContent_MakeAvailable(${name})
    target_link_libraries(celestium PRIVATE ${name})
endfunction()

# --- Other Packages ---
fetch_and_link_dependency(sol2 https://github.com/ThePhD/sol2.git develop)
fetch_and_link_dependency(ryml https://github.com/biojppm/rapidyaml.git master)
