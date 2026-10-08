# The test targets, split out of CMakeLists.txt to keep it under the house line cap.
# include() does not change CMAKE_CURRENT_SOURCE_DIR, so every relative path and
# ${CMAKE_CURRENT_SOURCE_DIR} below still means the repository root.

# =========================================================
# Unit Tests (macOS only — behaviors have CoreGraphics deps)
# =========================================================
if(APPLE)
    enable_language(CXX)
    find_package(GTest REQUIRED)
    
    # Cross-platform test sources
    
    
# Test sources listed explicitly (no set()-and-splat): the house test
    # registration gate reads only what an add_executable/add_test block
    # names, so a variable indirection would hide every file from it.
    add_executable(CadGooseTests
        tests/test_main.cpp
        src/common/crash_logger.mm
        tests/test_goose_behavior.cpp
        tests/test_goose_integration.cpp
        tests/test_goose_world_forces.cpp
        tests/test_goose_physics.cpp
        tests/test_goose_behaviors.cpp
        tests/common/test_goose_behaviors_internal.cpp
        tests/test_soak.cpp
        tests/common/test_behavior_helpers.cpp
        tests/common/test_behavior_manager.cpp
        tests/common/test_behavior_states.cpp
        tests/common/test_behavior_typed.cpp
        tests/common/test_behavior_interaction.cpp
        tests/common/test_behavior_registry.cpp
        tests/common/test_behavior_exception.cpp
        tests/common/test_behavior_ball.cpp
        tests/common/test_behavior_breadcrumbs.cpp
        tests/common/test_behavior_core.cpp
        tests/common/test_behavior_physics.cpp
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
        tests/common/test_mcp_json_rpc.cpp
        tests/common/test_ring_buffer.cpp
        tests/common/test_random.cpp
        tests/common/test_coordinate.cpp
        tests/common/test_mcp_integration.cpp
        tests/common/test_ai_tokens.cpp
        tests/common/test_ai_routing.cpp
        tests/common/test_ai_bridge.cpp
        tests/common/test_ai_interfaces.cpp
        tests/common/test_window_interfaces.cpp
        tests/common/test_config_registry.cpp
        tests/common/test_config_get_set.cpp
        tests/common/test_config_path.cpp
        tests/common/test_config_load_save.cpp
        tests/common/test_config_themes.cpp
        tests/common/test_actor.cpp
        tests/common/test_config_toggles.cpp
        tests/common/test_dropped_item_hit.cpp
        tests/common/test_ai_text_meme.cpp
        tests/common/test_regression_portal_leaves_honk.cpp
        tests/common/test_actor_manager.cpp
        tests/common/test_app_actions.cpp
        tests/common/test_app_cli.cpp
        tests/common/test_behavior_sim.cpp
        tests/common/test_cursor_backend.cpp
        tests/common/test_items.cpp
        tests/common/test_log.cpp
        tests/common/test_tick_manager_logic.cpp
        tests/common/test_audio.cpp
        tests/common/test_asset_manager.cpp
        tests/common/test_math.cpp
        tests/common/test_world.cpp
        tests/common/test_baby_stalin_spawn.mm
        tests/common/test_mcp_server.cpp
        tests/common/test_mcp_http_server.cpp
        tests/common/platform_input_mock.cpp
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
        src/common/goose_rig.cpp
        src/common/goose_forces.cpp
        src/common/goose_behaviors_internal.cpp
        src/common/goose_behaviors_wander.cpp
        src/common/goose_behaviors_fetch.cpp
        src/common/goose_behaviors_interact.cpp
        src/common/items.cpp
        src/common/item_window_logic.cpp
        src/common/effect_window_logic.cpp
        src/common/tick_manager_logic.cpp
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
        src/common/hotkey_cache.cpp
        src/common/event_bus.cpp
        src/common/mcp_server.cpp
        src/common/mcp_http_server.cpp
        src/common/mcp_json_rpc.cpp
        src/common/mcp_handlers.cpp
        src/common/ai_mcp_bridge.cpp
        src/common/ai_local_llm_mock.cpp
        src/common/ai_text_meme_mock.cpp
        src/common/ai_http_client_mock.cpp
        src/common/window_system_mock.cpp
        tests/common/test_item_window_logic.cpp
        tests/common/test_effect_window_logic.cpp
        tests/common/test_goose_window_size.mm
        tests/common/test_cg_renderer.mm
        tests/platform/macos/test_cg_primitives.mm
        tests/common/test_goose_render.mm
        tests/common/test_config_gui_rendering.mm
        tests/platform/macos/test_window_lifecycle.mm
        tests/platform/macos/test_goose_rendering.mm
        tests/common/test_gui_config.mm
        tests/common/test_local_llm.cpp
        tests/common/test_behavior_ai.mm
        tests/common/test_ai_helpers.mm
        tests/common/test_effect_registration.mm
        tests/common/test_effect_reg_live.mm
        tests/platform/macos/test_behavior_element_window.mm
        tests/platform/macos/test_mac_cursor_backend.mm
        tests/common/test_world_utils.mm
        tests/common/test_behavior_health.mm
        tests/common/test_behavior_rainbow.mm
        tests/common/test_behavior_acid.mm
        tests/common/test_behavior_boredom.mm
        tests/common/test_behavior_nametag.mm
        tests/common/test_behavior_peeking.mm
        tests/common/test_behavior_anger.mm
        tests/common/test_behavior_presence.mm
        tests/common/test_behavior_honcker.mm
        tests/common/test_behavior_drag.mm
        tests/common/test_behavior_breadcrumbs.mm
        tests/common/test_behavior_hats.mm
        tests/common/test_behavior_toys.mm
        tests/common/test_behavior_interactive_drops.mm
        tests/common/test_behavior_pomodoro.mm
        tests/common/test_behavior_pomodoro_render.mm
        tests/common/test_behavior_jail.mm
        tests/common/test_behavior_portal.mm
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
        src/platform/macos/platform_input.mm
        src/platform/macos/tick_manager.mm
        src/platform/macos/item_drag_controller.mm
        tests/platform/macos/test_renderer.mm
        tests/platform/macos/test_gui_accessibility.mm
        tests/platform/macos/test_gui_statusbar_chat.mm
        tests/platform/macos/test_gui_accessibility_goose_view.mm
        tests/platform/macos/test_dragging_integration.mm
        tests/platform/macos/test_headless_rendering.mm
        tests/platform/macos/test_drag_controller.mm
        tests/platform/macos/test_window_trail.mm
    )
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
            tests/common/command_socket_stub.cpp
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
             COMMAND CadGooseTests --gtest_filter=-MCPIntegrationTest*:LocalLLMTest*:AccessibilityGUITest*:DraggingIntegration*:WindowTrailTest*:BehaviorToggles.ToysBehaviorRegistered:PortalCleanup.BehaviorHasCleanupFunction:StalinHonk.*
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # Regression guard — runs AFTER the suite (DEPENDS) and fails if the run
    # left anything dirty or untracked under tracked config/. Structural gate
    # for the test-sandbox leak class; see scripts/check_config_clean.cmake.
    add_test(NAME config_tree_clean_after_tests
             COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_config_clean.cmake
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(config_tree_clean_after_tests PROPERTIES DEPENDS CadGooseTests)

    # Standalone E2E targets — require a display server (ScreenCaptureKit).
    # Excluded in headless CI via `ctest -E "requires_display"`.
    add_test(NAME trail_detection_test COMMAND trail_detection_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(trail_detection_test PROPERTIES LABELS "requires_display")

    add_test(NAME soak_fetch_test COMMAND soak_fetch_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(soak_fetch_test PROPERTIES LABELS "requires_display")

    add_test(NAME multi_goose_test COMMAND multi_goose_test
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(multi_goose_test PROPERTIES LABELS "requires_display")

    # Trail detection test — standalone binary, not a gtest.
    # Connects to running CadGoose app via Unix socket, captures screenshots,
    # and analyzes pixels for window trail artifacts.
    add_executable(trail_detection_test
        tests/platform/macos/test_window_trail_detection.mm
        src/platform/macos/command_socket.mm
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
        src/platform/macos/command_socket.mm
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
        src/platform/macos/command_socket.mm
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

    # One-off QA probe: dumps the pixel format ScreenCaptureKit actually
    # delivers (BGRA vs RGBA confusion once broke trail detection). Not an
    # assertion test — a diagnostic that saves /tmp/test_captured_frame.png.
    add_executable(pixel_format_probe
        tests/platform/macos/test_pixel_format_debug.mm
    )
    target_include_directories(pixel_format_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
    )
    target_link_libraries(pixel_format_probe PRIVATE
        ${COCOA_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${SCREEN_CAPTURE_KIT_LIBRARY}
        ${CORE_VIDEO_LIBRARY}
        ${CORE_MEDIA_LIBRARY}
        Threads::Threads
    )
    set_target_properties(pixel_format_probe PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()
endif()
