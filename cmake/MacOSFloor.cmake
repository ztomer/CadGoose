# The minimum macOS CadGoose supports (ROADMAP G0.14; owner ruling
# 2026-10-18: macOS 26, the macOS CI builds the release on).
#
# This file is the ONE place the floor is written. Everything else derives it:
#   - CMakeLists.txt includes this before project(), which sets
#     CMAKE_OSX_DEPLOYMENT_TARGET, so every C/C++/ObjC++ object, the Swift
#     object (-target arm64-apple-macos<floor>) and the fetched mimalloc are
#     built for it, and the linker stamps it into LC_BUILD_VERSION `minos`.
#   - scripts/create_bundle.sh asks for it in script mode and writes it into
#     Info.plist as LSMinimumSystemVersion:
#         cmake -P cmake/MacOSFloor.cmake      prints the floor, e.g. 26.0
#   - README.md and the cask state it in prose; tools/check_macos_floor.py
#     holds them, the plist, and the minos of every Mach-O in the built bundle
#     (or the DMG) to it, and refuses a second place that writes a floor.
#
# Raising or lowering the floor is an edit HERE and to the prose; the check
# names every place that disagrees.
set(CG_MACOS_FLOOR "26.0")

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    # stdout, bare, so a shell can capture it: message() would add a prefix
    # or go to stderr.
    execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${CG_MACOS_FLOOR}")
    return()
endif()

# A build tree configured for any other target is refused, not silently
# corrected: -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 used to be honoured, and a
# value nobody acts on is a control that lies. (The MACOSX_DEPLOYMENT_TARGET
# environment variable is CMake's default for an unset cache entry and never
# reaches the compiler past the explicit -mmacosx-version-min CMake passes.)
if(DEFINED CACHE{CMAKE_OSX_DEPLOYMENT_TARGET}
   AND NOT "$CACHE{CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL ""
   AND NOT "$CACHE{CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "${CG_MACOS_FLOOR}")
    message(FATAL_ERROR "✗ CMAKE_OSX_DEPLOYMENT_TARGET is "
        "$CACHE{CMAKE_OSX_DEPLOYMENT_TARGET}, but CadGoose's macOS floor is "
        "${CG_MACOS_FLOOR} (cmake/MacOSFloor.cmake, ROADMAP G0.14). Drop the "
        "override, or change the floor there.")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "${CG_MACOS_FLOOR}" CACHE STRING
    "Minimum macOS; set by cmake/MacOSFloor.cmake, not here" FORCE)
