cmake_minimum_required(VERSION 3.17)
project(CadGoose)

# Version string for `CadGoose --version`, derived from the latest git tag at
# configure time (e.g. "v1.66" or "v1.66-3-gabcdef" for untagged builds).
execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_OUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(GIT_DESCRIBE_OUT)
    set(CADGOOSE_VERSION "${GIT_DESCRIBE_OUT}")
else()
    set(CADGOOSE_VERSION "0.0.0")
endif()
add_compile_definitions(CADGOOSE_VERSION="${CADGOOSE_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ASan is applied per-language to avoid the macOS 26.x ObjC runtime crash
# ("realized class ... has corrupt data pointer: malloc_size(...) = 0").
# Only C++ (.cpp) files get instrumented; ObjC++ (.mm) files are excluded.
# For full coverage including ObjC, use MallocScribble instead:
#   MallocScribble=YES MallocPreScribble=YES ./CadGooseTests
#
# Enable via:
#   cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
#
# Runtime flags for common macOS false positives:
#   ASAN_OPTIONS=detect_odr_violation=0   suppress duplicate symbol warnings
#   ASAN_OPTIONS=alloc_dealloc_mismatch=0  suppress ObjC ARC false positives
set(ENABLE_ASAN_DEFAULT OFF)
option(ENABLE_ASAN "Enable AddressSanitizer (Debug builds, C++ files only)" ${ENABLE_ASAN_DEFAULT})

# Helper macro: apply ASan flags to a target (C++ only, excludes ObjC++)
macro(target_enable_asan TARGET)
    if(ENABLE_ASAN AND APPLE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "ASan enabled for target ${TARGET} (C++ files only)")
        target_compile_options(${TARGET} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address -fno-omit-frame-pointer>
        )
        target_link_options(${TARGET} PRIVATE -fsanitize=address)
    endif()
endmacro()

# Compile out all debug-level logging (CG_DEBUG / CG_DEBUG_ASYNC / DebugLog /
# DebugLogv / LogTick / the DEBUG_LOG macros) for release/CI builds. Applied to
# the CadGoose app target only — the test target keeps full logging so log tests
# still exercise it. Enabled by build_release.sh (-DCG_DISABLE_DEBUG_LOG=ON).
option(CG_DISABLE_DEBUG_LOG "Compile out all debug-level logging (release/CI build)" OFF)

option(CODE_COVERAGE "Build with LLVM source-based code coverage" OFF)
if(CODE_COVERAGE AND APPLE)
    message(STATUS "Code coverage instrumentation enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping" CACHE STRING "C++ compiler flags" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping" CACHE STRING "C compiler flags" FORCE)
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping" CACHE STRING "ObjC++ compiler flags" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate" CACHE STRING "Linker flags" FORCE)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(APPLE)
    set(MACOS TRUE)
elseif(UNIX)
    set(LINUX TRUE)
endif()

# toml11 — fetched at configure time instead of vendored as a git submodule.
# Pinned to a specific commit for reproducible builds. The fetched package
# provides the `toml11` target (header `<toml.hpp>`), which carries its own
# include directories, so no manual include paths are needed.
include(FetchContent)
FetchContent_Declare(
    toml11
    GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
    GIT_TAG v4.4.0
)
FetchContent_MakeAvailable(toml11)

if(MACOS)
    enable_language(OBJCXX)
    find_package(Threads REQUIRED)

    file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp" "src/common/*.mm")
    file(GLOB_RECURSE PLATFORM_SOURCES "src/platform/macos/*.cpp" "src/platform/macos/*.mm")

    set(SOURCES ${COMMON_SOURCES} ${PLATFORM_SOURCES})

    message(STATUS "macOS sources: ${SOURCES}")

    find_library(COCOA_LIBRARY Cocoa)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
    find_library(CURL_LIBRARY curl)
    find_library(COREML_LIBRARY CoreML)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(QUARTZCORE_LIBRARY QuartzCore)


    if(NOT COCOA_LIBRARY)
        message(FATAL_ERROR "Cocoa framework not found")
    endif()

    option(USE_MIMALLOC "Use mimalloc allocator" ON)
    if(USE_MIMALLOC)
        find_library(MIMALLOC_LIBRARY mimalloc)
        find_path(MIMALLOC_INCLUDE_DIR mimalloc.h)
        if(MIMALLOC_LIBRARY AND MIMALLOC_INCLUDE_DIR)
            message(STATUS "mimalloc found: ${MIMALLOC_LIBRARY}")
        else()
            message(WARNING "mimalloc not found, falling back to system malloc")
        endif()
    else()
        message(STATUS "mimalloc disabled via USE_MIMALLOC=OFF")
        unset(MIMALLOC_LIBRARY)
        unset(MIMALLOC_INCLUDE_DIR)
    endif()

    add_executable(CadGoose ${SOURCES})
    target_enable_asan(CadGoose)
    if(CG_DISABLE_DEBUG_LOG)
        message(STATUS "CadGoose: debug logging compiled out (CG_DISABLE_DEBUG_LOG)")
        target_compile_definitions(CadGoose PRIVATE CG_DISABLE_DEBUG_LOG)
    endif()

    target_link_libraries(CadGoose
        toml11
        ${COCOA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${APPLICATIONSERVICES_LIBRARY}
        ${CURL_LIBRARY}
        ${COREML_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        Threads::Threads
        ${MIMALLOC_LIBRARY}
    )

    target_include_directories(CadGoose PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${MIMALLOC_INCLUDE_DIR}
    )

    set_target_properties(CadGoose PROPERTIES
        COMPILE_FLAGS "-fobjc-arc -Wno-deprecated-literal-operator"
        MACOSX_BUNDLE FALSE
    )

    # Compile Swift FoundationLLM wrapper (CMake doesn't support Swift with Unix Makefiles)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(SWIFT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/common/behaviors/FoundationLLM.swift")
    set(SWIFT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/FoundationLLM.o")
    add_custom_command(
        OUTPUT ${SWIFT_OBJ}
        COMMAND swiftc -c -parse-as-library ${SWIFT_SRC} -o ${SWIFT_OBJ} -emit-module-path "${CMAKE_CURRENT_BINARY_DIR}/FoundationLLM.swiftmodule" -target arm64-apple-macos15 -sdk "${MACOS_SDK_PATH}"
        DEPENDS ${SWIFT_SRC}
        COMMENT "Compiling FoundationLLM.swift"
        VERBATIM
    )
    add_custom_target(CompileSwiftFoundationLLM DEPENDS ${SWIFT_OBJ})
    add_dependencies(CadGoose CompileSwiftFoundationLLM)
    target_link_libraries(CadGoose ${SWIFT_OBJ})
    target_link_options(CadGoose PRIVATE "-L/usr/lib/swift")

elseif(LINUX)
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)
    pkg_check_modules(LS gtk4-layer-shell-0)
    if(NOT LS_FOUND)
        message(STATUS "gtk4-layer-shell-0 not found, building without layer shell support")
        set(LS_LIBRARIES "")
        set(LS_INCLUDE_DIRS "")
        set(LS_CFLAGS_OTHER "")
    else()
        target_compile_definitions(CadGoose PRIVATE GTK4_LAYER_SHELL_ENABLED)
    endif()
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

    find_program(WAYLAND_SCANNER NAMES wayland-scanner)

    function(wayland_generate protocol_file out_header out_code)
        get_filename_component(protocol_name ${protocol_file} NAME_WE)
        add_custom_command(
            OUTPUT ${out_header}
            COMMAND ${WAYLAND_SCANNER} client-header ${protocol_file} ${out_header}
            DEPENDS ${protocol_file}
        )
        add_custom_command(
            OUTPUT ${out_code}
            COMMAND ${WAYLAND_SCANNER} private-code ${protocol_file} ${out_code}
            DEPENDS ${protocol_file}
        )
    endfunction()

    set(VIRTUAL_POINTER_PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/protocols/wlr-virtual-pointer-unstable-v1.xml")
    wayland_generate(${VIRTUAL_POINTER_PROTOCOL}
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-client-protocol.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-protocol.c")

    set(GENERATED_SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-protocol.c"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-client-protocol.h"
    )

    file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp")
    file(GLOB_RECURSE PLATFORM_SOURCES "src/platform/linux/*.cpp")
    set(SOURCES ${COMMON_SOURCES} ${PLATFORM_SOURCES} ${GENERATED_SOURCES})

    if(NOT APPLE)
        set_source_files_properties(${COMMON_SOURCES} PROPERTIES LANGUAGE CXX)
    endif()

    find_package(X11 REQUIRED)
    if(X11_FOUND)
        message(STATUS "X11 Found")
        if (NOT X11_XTest_FOUND)
            find_library(X11_XTest_LIB Xtst ${X11_LIBRARY_DIR})
            if (X11_XTest_LIB)
                set(X11_XTest_FOUND TRUE)
                message(STATUS "Xtst Found at ${X11_XTest_LIB}")
            endif()
        endif()
    endif()

    add_executable(CadGoose ${SOURCES})
    target_enable_asan(CadGoose)
    if(CG_DISABLE_DEBUG_LOG)
        message(STATUS "CadGoose: debug logging compiled out (CG_DISABLE_DEBUG_LOG)")
        target_compile_definitions(CadGoose PRIVATE CG_DISABLE_DEBUG_LOG)
    endif()

    target_link_libraries(CadGoose
        toml11
        ${GTK4_LIBRARIES}
        ${LS_LIBRARIES}
        ${SDL2_LIBRARIES}
        ${SDL2MIXER_LIBRARIES}
        ${GDKPIXBUF_LIBRARIES}
        ${X11_LIBRARIES}
        ${X11_XTest_LIB}
        ${WAYLAND_CLIENT_LIBRARIES}
        Threads::Threads
        m
        stdc++fs
    )

    target_include_directories(CadGoose PUBLIC
        ${GTK4_INCLUDE_DIRS}
        ${LS_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2MIXER_INCLUDE_DIRS}
        ${GDKPIXBUF_INCLUDE_DIRS}
        ${WAYLAND_CLIENT_LIBRARIES}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/linux
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    target_compile_options(CadGoose PUBLIC
        ${GTK4_CFLAGS_OTHER}
        ${LS_CFLAGS_OTHER}
        ${SDL2_CFLAGS_OTHER}
        ${SDL2MIXER_CFLAGS_OTHER}
        ${GDKPIXBUF_CFLAGS_OTHER}
        ${WAYLAND_CLIENT_CFLAGS_OTHER}
    )

else()
    message(FATAL_ERROR "Unsupported platform. Only macOS and Linux are supported.")
endif()

# Unit tests and the display-bound E2E targets (macOS only).
include(cmake/Tests.cmake)
