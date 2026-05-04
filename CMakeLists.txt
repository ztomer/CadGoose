cmake_minimum_required(VERSION 3.17)
project(CadGoose)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(MACOS TRUE)
elseif(UNIX)
    set(LINUX TRUE)
endif()

if(MACOS)
    enable_language(OBJCXX)
    find_package(Threads REQUIRED)

    file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp")
    file(GLOB_RECURSE PLATFORM_SOURCES "src/platform/macos/*.cpp" "src/platform/macos/*.mm")

    set(SOURCES ${COMMON_SOURCES} ${PLATFORM_SOURCES})

    message(STATUS "macOS sources: ${SOURCES}")

    find_library(COCOA_LIBRARY Cocoa)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)

    if(NOT COCOA_LIBRARY)
        message(FATAL_ERROR "Cocoa framework not found")
    endif()

    add_executable(CadGoose ${SOURCES})

    target_link_libraries(CadGoose
        ${COCOA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${APPLICATIONSERVICES_LIBRARY}
        Threads::Threads
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

elseif(LINUX)
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)
    pkg_check_modules(LS REQUIRED gtk4-layer-shell-0)
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

    set(VIRTUAL_POINTER_PROTOCOL "${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-virtual-pointer-unstable-v1.xml")
    wayland_generate(${VIRTUAL_POINTER_PROTOCOL}
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-client-protocol.h"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-protocol.c")

    set(GENERATED_SOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-protocol.c"
        "${CMAKE_CURRENT_BINARY_DIR}/wlr-virtual-pointer-unstable-v1-client-protocol.h"
    )

    file(GLOB_RECURSE COMMON_SOURCES "src/common/*.cpp")
    file(GLOB_recurSE PLATFORM_SOURCES "src/platform/linux/*.cpp")
    set(SOURCES ${COMMON_SOURCES} ${PLATFORM_SOURCES} ${GENERATED_SOURCES})

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

    target_link_libraries(CadGoose
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

# =========================================================
# Unit Tests
# =========================================================
if(APPLE OR LINUX)
    enable_language(CXX)
    find_package(GTest REQUIRED)
    
    add_executable(CadGooseTests tests/test_main.cpp)
    
    target_link_libraries(CadGooseTests
        GTest::GTest
        GTest::Main
        Threads::Threads
    )
    
    target_include_directories(CadGooseTests PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    add_test(NAME CadGooseTests COMMAND CadGooseTests)
endif()