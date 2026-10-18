cmake_minimum_required(VERSION 3.25)  # FetchContent SYSTEM
# The minimum macOS, before project() as CMAKE_OSX_DEPLOYMENT_TARGET must be
# (ROADMAP G0.14). The floor is written there and only there.
include(cmake/MacOSFloor.cmake)
project(CadGoose)

# Before anything else: an unsupported OS/arch is a hard failure (G0.5).
include(cmake/PlatformPolicy.cmake)

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

# toml11 (header-only), fetched at configure time as the v4.4.0 release
# tarball (the newest release), pinned by hash. Not a git clone (ROADMAP
# G0.10): GIT_TAG fetched the full history and every submodule (nlohmann/json,
# doctest, a Hugo theme), 368 MB for 0.35 MB of tarball, on every clean
# configure, including the pre-push gate's.
#
# Its own CMakeLists.txt is not run (SOURCE_SUBDIR names a directory without
# one): for a consumer it defines only an INTERFACE target over include/, and
# running it cost 13 compiler-flag probes and a CMake deprecation warning
# (it sets policy CMP0127 to OLD). The target is defined here instead, as a
# SYSTEM include so its headers' warnings are not ours (v4.4.0 declares
# `operator"" _toml`, which clang deprecates; this replaced a blanket
# -Wno-deprecated-literal-operator that hid the same thing in our own code).
include(FetchContent)
FetchContent_Declare(
    toml11
    URL https://github.com/ToruNiina/toml11/archive/refs/tags/v4.4.0.tar.gz
    URL_HASH SHA256=815bfe6792aa11a13a133b86e7f0f45edc5d71eb78f5fb6686c49c7f792b9049
    SOURCE_SUBDIR cadgoose-header-only-no-cmake
)
FetchContent_MakeAvailable(toml11)
if(NOT EXISTS "${toml11_SOURCE_DIR}/include/toml.hpp")
    message(FATAL_ERROR "toml11 fetch has no include/toml.hpp under ${toml11_SOURCE_DIR}")
endif()
add_library(toml11 INTERFACE)
target_include_directories(toml11 SYSTEM INTERFACE "${toml11_SOURCE_DIR}/include")

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

    # mimalloc replaces malloc process-wide. It is built here, from the
    # v3.5.3 release tarball (the newest release) pinned by hash, as the
    # single object file its README recommends for a static override: the
    # object's constructor makes its zone the default one, so every
    # allocation, the system frameworks' included, goes through it. It used to
    # be Homebrew's libmimalloc dylib, copied into the bundle, which carries
    # the minos of whatever macOS built the bottle (27.0 on the author's Mac):
    # the floor CadGoose claimed was not the floor it shipped (ROADMAP G0.14).
    # Built here it inherits CMAKE_OSX_DEPLOYMENT_TARGET like our own code,
    # and the bundle carries no dylib at all.
    option(USE_MIMALLOC "Use mimalloc allocator" ON)
    if(USE_MIMALLOC)
        set(MI_BUILD_SHARED OFF)
        set(MI_BUILD_TESTS OFF)
        # The object's copy step depends on the static target, so it stays on.
        set(MI_BUILD_STATIC ON)
        set(MI_BUILD_OBJECT ON)
        FetchContent_Declare(
            mimalloc
            URL https://github.com/microsoft/mimalloc/archive/refs/tags/v3.5.3.tar.gz
            URL_HASH SHA256=3b4a15153a59905995f7070296ed604bb5ccc00cabb8b93446931aff77224d47
            SYSTEM
        )
        FetchContent_MakeAvailable(mimalloc)
        set(CG_MIMALLOC mimalloc-obj)
        message(STATUS "mimalloc: v3.5.3 object file, static override")
    else()
        message(STATUS "mimalloc disabled via USE_MIMALLOC=OFF")
        set(CG_MIMALLOC "")
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
        ${CG_MIMALLOC}
    )

    target_include_directories(CadGoose PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    )

    set_target_properties(CadGoose PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
        MACOSX_BUNDLE FALSE
    )

    # Compile Swift FoundationLLM wrapper (CMake doesn't support Swift with Unix Makefiles).
    # Its target triple carries the floor from cmake/MacOSFloor.cmake (G0.14).
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(SWIFT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/common/behaviors/FoundationLLM.swift")
    set(SWIFT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/FoundationLLM.o")
    add_custom_command(
        OUTPUT ${SWIFT_OBJ}
        COMMAND swiftc -c -parse-as-library ${SWIFT_SRC} -o ${SWIFT_OBJ} -emit-module-path "${CMAKE_CURRENT_BINARY_DIR}/FoundationLLM.swiftmodule" -target arm64-apple-macos${CMAKE_OSX_DEPLOYMENT_TARGET} -sdk "${MACOS_SDK_PATH}"
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

# Deprecations are build errors, on every compiled target this file defines
# (ROADMAP G0.4, house rule 17: the warning is the only notice before an API
# becomes unavailable). Applied by enumeration, not a list, so a new target
# cannot be forgotten. Apple marks APIs it is about to retire with
# API_TO_BE_DEPRECATED, which never warns (activateIgnoringOtherApps:,
# lockFocus, kCGBitmapByteOrder*, NSBezelStyleRounded all sat in the tree
# that way); defining it as an already-past version turns every such mark
# into a deprecation warning, and so into an error.
if(APPLE)
    get_property(CG_ALL_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(CG_TARGET IN LISTS CG_ALL_TARGETS)
        get_target_property(CG_TARGET_TYPE ${CG_TARGET} TYPE)
        if(CG_TARGET_TYPE MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
            target_compile_options(${CG_TARGET} PRIVATE
                -Wdeprecated-declarations -Werror=deprecated-declarations)
            target_compile_definitions(${CG_TARGET} PRIVATE
                API_TO_BE_DEPRECATED=10.0 API_TO_BE_DEPRECATED_MACOS=10.0)
        endif()
    endforeach()
endif()
