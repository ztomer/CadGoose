# Included from cmake/Tests.cmake inside its if(APPLE) block, after
# enable_testing(); split out to keep that file under the house line cap.

# ThreadSanitizer suite (ROADMAP M1.2, M1.6): the config sources that more than
# one thread touches, compiled with -fsanitize=thread into their own
# binary. The main suite links ObjC++ that TSan cannot run under, so this
# target carries exactly the sources the race tests reach plus inert link
# stubs. Run by scripts/run_tests_tsan.sh (CI and the pre-push gate).
add_executable(CadGooseTsanTests
    tests/tsan/test_config_save_race.cpp
    tests/common/config_link_stubs.cpp
    tests/common/command_socket_stub.cpp
    src/common/mcp_handlers.cpp
    src/common/mcp_json_rpc.cpp
    src/common/mcp_server.cpp
    src/common/accept_loop.cpp
    src/common/config.cpp
    src/common/config_load.cpp
    src/common/config_save.cpp
    src/common/config_string.cpp
    src/common/config_registry_ai.cpp
    src/common/config_registry_behaviors.cpp
    src/common/config_registry_generated.cpp
    src/common/config_registry_render.cpp
    src/common/hotkey.cpp
    src/common/hotkey_cache.cpp
)
target_include_directories(CadGooseTsanTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos
)
target_compile_options(CadGooseTsanTests PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
target_link_options(CadGooseTsanTests PRIVATE -fsanitize=thread)
target_link_libraries(CadGooseTsanTests PRIVATE toml11 GTest::GTest GTest::Main Threads::Threads)
add_test(NAME CadGooseTsanTests
         COMMAND ${CMAKE_COMMAND} -E env TSAN_OPTIONS=halt_on_error=1 $<TARGET_FILE:CadGooseTsanTests>
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
