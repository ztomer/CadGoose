cmake_minimum_required(VERSION 3.17)
project(CadGoose)

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
    GIT_TAG b32a2fff0d27e1f7522f26a125101500ddb47156
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

# =========================================================
# Unit Tests (macOS only — behaviors have CoreGraphics deps)
# =========================================================
if(APPLE)
    enable_language(CXX)
    find_package(GTest REQUIRED)
    
    # Cross-platform test sources
    set(TEST_SOURCES_COMMON
        tests/test_main.cpp
        src/common/crash_logger.mm
        tests/test_goose_behavior.cpp
        tests/test_goose_physics.cpp
        tests/test_goose_behaviors.cpp
        tests/test_soak.cpp
        tests/common/test_behavior_helpers.cpp
        tests/common/test_behavior_manager.cpp
        tests/common/test_behavior_states.cpp
        tests/common/test_behavior_typed.cpp
        tests/common/test_behavior_interaction.cpp
        tests/common/test_behavior_registry.cpp
        tests/common/test_behavior_ball.cpp
        tests/common/test_behavior_breadcrumbs.cpp
        tests/common/test_leafpile_scaling.cpp
        tests/common/test_footprint_registration.cpp
        tests/common/test_behavior_toggle.cpp
        tests/common/test_pomodoro_quiet.cpp
        tests/common/test_goose_sync.cpp
        tests/common/test_behavior_hats.cpp
        tests/common/test_timer.cpp
        tests/common/test_behaviors_fun.cpp
        tests/common/test_behaviors_pomodoro_portal.cpp
        tests/common/test_behaviors_visual.cpp
        tests/common/test_behaviors_control.cpp
        tests/common/test_behaviors_registry_ai.cpp
        tests/common/test_hotkey.cpp
        tests/common/test_event_bus.cpp
        tests/common/test_mcp_protocol.cpp
        tests/common/test_mcp_config.cpp
        tests/common/test_mcp_resources.cpp
        tests/common/test_mcp_integration.cpp
        tests/common/test_ai_tokens.cpp
        tests/common/test_ai_routing.cpp
        tests/common/test_ai_bridge.cpp
        tests/common/test_config_registry.cpp
        tests/common/test_config_get_set.cpp
        tests/common/test_config_load_save.cpp
        tests/common/test_config_toggles.cpp
        tests/common/test_dropped_item_hit.cpp
        tests/common/test_ai_text_meme.cpp
        tests/common/presence_stub.cpp
        tests/common/test_regression_portal_leaves_honk.cpp
        tests/common/test_baby_stalin_spawn.mm
        src/common/app_actions.cpp
        src/common/app_cli.cpp
        src/common/behavior.cpp
        src/common/log.cpp
        src/common/config.cpp
        src/common/config_helpers.h
        src/common/config_load.cpp
        src/common/config_registry_generated.cpp
        src/common/config_registry_behaviors.cpp
        src/common/config_registry_render.cpp
        src/common/config_registry_ai.cpp
        src/common/config_save.cpp
        src/common/cursor_backend.cpp
        src/common/goose.cpp
        src/common/goose_forces.cpp
        src/common/goose_behaviors_internal.cpp
        src/common/goose_behaviors_wander.cpp
        src/common/goose_behaviors_fetch.cpp
        src/common/goose_behaviors_interact.cpp
        src/common/items.cpp
        src/common/actor.cpp
        src/common/actor.mm
        src/common/actor_ball.mm
        src/common/actor_toy.mm
        src/common/actor_flower.mm
        src/common/actor_jail.mm
        src/common/actor_portal.mm
        src/common/actor_breadcrumb.mm
        src/common/actor_leafpile.mm
        src/common/actor_dropped_item.mm
        src/common/baby_stalin_actor.mm
        src/common/world.cpp
        src/common/behaviors/behavior_honcker.cpp
        src/common/behaviors/behavior_rainbow.cpp
        src/common/behaviors/behavior_health.cpp
        src/common/behaviors/behavior_pomodoro.cpp
        src/common/behaviors/behavior_ball.mm
        src/common/behaviors/behavior_breadcrumbs.cpp
        src/common/behaviors/behavior_anger.cpp
        src/common/behaviors/behavior_hats.cpp
        src/common/behaviors/behavior_acid.cpp
        src/common/behaviors/behavior_drag.cpp
        src/common/behaviors/behavior_jail.cpp
        src/common/behaviors/behavior_portal.cpp
        src/common/behaviors/behavior_interactive_drops.cpp
        src/common/behaviors/behavior_toys.cpp
        src/common/behaviors/behavior_boredom.cpp
        src/common/behaviors/behavior_nametag.cpp
        src/common/behaviors/behavior_presence.cpp
        src/common/behaviors/behavior_peeking.cpp
        src/common/hotkey.cpp
        src/common/event_bus.cpp
        src/common/mcp_server.cpp
        src/common/mcp_http_server.cpp
        src/common/mcp_json_rpc.cpp
        src/common/mcp_handlers.cpp
        src/common/ai_mcp_bridge.cpp
    )
    
    if(APPLE)
        set(TEST_SOURCES_PLATFORM
            tests/common/test_gui_config.mm
            tests/common/test_local_llm.cpp
            tests/common/test_behavior_ai.mm
            src/common/goose_drawing.mm
            src/common/item_renderer.mm
            src/common/world_utils.mm
            src/common/behaviors/behavior_ai.mm
            src/common/behaviors/ai_http_client.mm
            src/common/behaviors/ai_model_profiles.mm
            src/common/behaviors/ai_think_block_stripper.mm
            src/common/behaviors/ai_local_llm_adapter.mm
            src/common/behaviors/ai_prompt_builder.mm
            src/common/behaviors/ai_text_meme.mm
            src/common/behaviors/local_llm_tokenizer.mm
            src/common/behaviors/local_llm_model.mm
            src/common/behaviors/local_llm_inference.mm
            src/platform/macos/window.mm
            src/platform/macos/cursor_backend.mm
            src/platform/macos/tick_manager.mm
            src/platform/macos/item_drag_controller.mm
            src/platform/macos/audio.mm
            src/platform/macos/command_socket.cpp
            tests/platform/macos/test_renderer.mm
            tests/platform/macos/test_gui_accessibility.mm
            tests/platform/macos/test_dragging_integration.mm
            tests/platform/macos/test_headless_rendering.mm
            tests/platform/macos/test_goose_rendering.mm
            tests/platform/macos/test_window_trail.mm
        )
    else()
        set(TEST_SOURCES_PLATFORM
            src/platform/linux/ui.cpp
            src/platform/linux/ui_debug.cpp
            src/platform/linux/ui_callbacks.cpp
            src/platform/linux/hyprland.cpp
            src/platform/linux/x11_backend.cpp
            src/platform/linux/wlroots_backend.cpp
        )
    endif()
    
    add_executable(CadGooseTests ${TEST_SOURCES_COMMON} ${TEST_SOURCES_PLATFORM})
    target_enable_asan(CadGooseTests)
    add_dependencies(CadGooseTests CompileSwiftFoundationLLM)
    
    if(APPLE)
        target_sources(CadGooseTests PRIVATE 
            src/common/cursor_backend.cpp 
            src/common/world_utils.mm
            src/platform/macos/window.mm
            src/platform/macos/cursor_backend.mm 
            src/platform/macos/tick_manager.mm
            src/platform/macos/item_drag_controller.mm
            src/platform/macos/item_window.mm
            src/platform/macos/item_window_test.mm
            src/platform/macos/effect_window.mm
            src/platform/macos/effect_registration.mm
            src/platform/macos/effect_reg_footprint.mm
            src/platform/macos/effect_reg_pomodorobed.mm
            src/platform/macos/behavior_element_window.mm
            src/platform/macos/audio.mm
            src/platform/macos/command_socket.cpp
            tests/common/presence_stub.cpp
            tests/common/test_presence.cpp
            tests/platform/macos/test_renderer.mm
            tests/platform/macos/test_gui_accessibility.mm
            tests/platform/macos/test_dragging_integration.mm
            tests/platform/macos/test_headless_rendering.mm
            tests/platform/macos/test_window_trail.mm
        src/platform/macos/assets.mm
        )
    target_link_libraries(CadGooseTests
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
        ${SWIFT_OBJ}
        GTest::GTest
        GTest::Main
        Threads::Threads
        ${MIMALLOC_LIBRARY}
    )
    target_link_options(CadGooseTests PRIVATE "-L/usr/lib/swift")
    else()
        target_link_libraries(CadGooseTests
            GTest::GTest
            GTest::Main
            Threads::Threads
        )
    endif()
    
    target_include_directories(CadGooseTests PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${MIMALLOC_INCLUDE_DIR}
    )
    
    if(APPLE)
        set_target_properties(CadGooseTests PROPERTIES
            COMPILE_FLAGS "-fobjc-arc -Wno-deprecated-literal-operator"
        )
    endif()
    
    # Register the gtest binary with CTest. enable_testing() is required or
    # `ctest` finds no tests (which is why CI silently never ran them).
    # WORKING_DIRECTORY = source tree so the suite finds Assets/ at runtime.
    # WindowTrailTest.* drives a real NSWindow server and can't run on a
    # headless CI runner (it crashes), so it's excluded here — run it locally
    # with `./build/CadGooseTests` on a machine with a display.
    enable_testing()
    add_test(NAME CadGooseTests
             COMMAND CadGooseTests --gtest_filter=-WindowTrailTest.*
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Trail detection test — standalone binary, not a gtest.
    # Connects to running CadGoose app via Unix socket, captures screenshots,
    # and analyzes pixels for window trail artifacts.
    add_executable(trail_detection_test
        tests/platform/macos/test_window_trail_detection.mm
        src/platform/macos/command_socket.cpp
    )
    target_enable_asan(trail_detection_test)
    target_include_directories(trail_detection_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    )
    if(APPLE)
        find_library(SCREEN_CAPTURE_KIT_LIBRARY ScreenCaptureKit)
        find_library(CORE_VIDEO_LIBRARY CoreVideo)
        find_library(CORE_MEDIA_LIBRARY CoreMedia)
        target_link_libraries(trail_detection_test PRIVATE
            ${COCOA_LIBRARY}
            ${COREGRAPHICS_LIBRARY}
            ${SCREEN_CAPTURE_KIT_LIBRARY}
            ${CORE_VIDEO_LIBRARY}
            ${CORE_MEDIA_LIBRARY}
            Threads::Threads
        )
    set_target_properties(trail_detection_test PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    add_executable(soak_fetch_test
        tests/platform/macos/test_soak_fetch_visibility.mm
        src/platform/macos/command_socket.cpp
    )
    target_enable_asan(soak_fetch_test)
    target_include_directories(soak_fetch_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    )
    target_link_libraries(soak_fetch_test PRIVATE
        ${COCOA_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${SCREEN_CAPTURE_KIT_LIBRARY}
        ${CORE_VIDEO_LIBRARY}
        ${CORE_MEDIA_LIBRARY}
        Threads::Threads
    )
    set_target_properties(soak_fetch_test PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )

    add_executable(multi_goose_test
        tests/platform/macos/test_multi_goose.mm
        src/platform/macos/command_socket.cpp
    )
    target_enable_asan(multi_goose_test)
    target_include_directories(multi_goose_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    )
    target_link_libraries(multi_goose_test PRIVATE
        ${COCOA_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${SCREEN_CAPTURE_KIT_LIBRARY}
        ${CORE_VIDEO_LIBRARY}
        ${CORE_MEDIA_LIBRARY}
        Threads::Threads
    )
    set_target_properties(multi_goose_test PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()
endif()