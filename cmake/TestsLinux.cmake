# The Linux test suite (ROADMAP M1.5). The macOS suite links ObjC++ and the
# Cocoa/CoreGraphics frameworks, which is why CI once commented its Linux test
# step out and ran no Linux test at all: a Linux-only defect (config.toml never
# loaded) then had nothing that could go red. This binary carries the portable
# sources -- the config layer every platform shares -- plus inert link stubs,
# and runs under ctest in CI's Linux job and in scripts/run_tests_linux.sh.
if(LINUX)
    find_package(GTest REQUIRED)
    # Test sources listed explicitly (the house test registration gate reads
    # only what an add_executable block names).
    add_executable(CadGooseLinuxTests
        tests/common/test_config_startup.cpp
        tests/common/config_link_stubs.cpp
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
    target_include_directories(CadGooseLinuxTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/linux
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GTK4_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2MIXER_INCLUDE_DIRS}
        ${GDKPIXBUF_INCLUDE_DIRS}
    )
    # config_save.cpp reaches world.h, which pulls in the GTK and SDL headers
    # the app is built against: the same flags, from the same pkg-config probes.
    target_compile_options(CadGooseLinuxTests PRIVATE
        ${GTK4_CFLAGS_OTHER} ${SDL2_CFLAGS_OTHER} ${SDL2MIXER_CFLAGS_OTHER}
        ${GDKPIXBUF_CFLAGS_OTHER})
    target_link_libraries(CadGooseLinuxTests PRIVATE
        toml11 GTest::gtest GTest::gtest_main ${GTK4_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES}
        Threads::Threads stdc++fs)
    enable_testing()
    add_test(NAME CadGooseLinuxTests COMMAND CadGooseLinuxTests
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
