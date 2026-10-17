# Target platform policy (ROADMAP G0.5; the house rule): 64-bit only, and
# macOS is Apple silicon only. Supported: Darwin/arm64, Linux/x86_64,
# Linux/aarch64. Every other combination is a FATAL_ERROR at configure time,
# never a build that nobody has compiled, run or tested.
#
# Seam: CG_TARGET_OS, CG_TARGET_ARCH and CG_TARGET_POINTER_SIZE override the
# detected values, so both directions are testable on one machine. Script
# mode decides one combination without configuring the project:
#
#   cmake -DCG_TARGET_OS=Linux -DCG_TARGET_ARCH=i686 -P cmake/PlatformPolicy.cmake
#
# tools/check_platform_policy.py drives the whole matrix that way, plus one
# real configure to prove CMakeLists.txt still calls this before anything.

# Sets <out> to "" when the combination is supported, else to the reason.
function(cg_platform_reason os arch pointer_size out)
    set(reason "")
    list(LENGTH arch arch_count)
    if(arch STREQUAL "")
        set(reason "no target architecture detected")
    elseif(arch_count GREATER 1)
        set(reason "multi-architecture build (${arch}); one 64-bit architecture per build")
    elseif(NOT pointer_size STREQUAL "8")
        set(reason "${pointer_size}-byte pointers: 64-bit targets only")
    elseif(os STREQUAL "Darwin")
        if(NOT arch STREQUAL "arm64")
            set(reason "macOS ${arch}: macOS is Apple silicon (arm64) only")
        endif()
    elseif(os STREQUAL "Linux")
        if(NOT arch MATCHES "^(x86_64|aarch64)$")
            set(reason "Linux ${arch}: Linux is x86_64 or aarch64 only")
        endif()
    else()
        set(reason "${os}: only macOS (Darwin) and Linux are supported")
    endif()
    set(${out} "${reason}" PARENT_SCOPE)
endfunction()

# Normalises the spellings toolchains use for the same architecture.
function(cg_platform_normalise_arch os arch out)
    set(result "")
    foreach(a IN LISTS arch)
        string(TOLOWER "${a}" a)
        if(a STREQUAL "amd64")
            set(a "x86_64")
        elseif(os STREQUAL "Linux" AND a STREQUAL "arm64")
            set(a "aarch64")
        endif()
        list(APPEND result "${a}")
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(cg_enforce_platform os arch pointer_size)
    cg_platform_normalise_arch("${os}" "${arch}" arch)
    cg_platform_reason("${os}" "${arch}" "${pointer_size}" reason)
    if(reason)
        message(FATAL_ERROR "✗ unsupported target platform: ${reason}. "
            "Supported: macOS arm64, Linux x86_64, Linux aarch64 (ROADMAP G0.5).")
    endif()
    message(STATUS "target platform: ${os}/${arch}, supported")
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    if(NOT DEFINED CG_TARGET_POINTER_SIZE)
        set(CG_TARGET_POINTER_SIZE 8)
    endif()
    cg_enforce_platform("${CG_TARGET_OS}" "${CG_TARGET_ARCH}" "${CG_TARGET_POINTER_SIZE}")
else()
    # Configure mode: the detected target, unless the seam overrides it.
    if(NOT DEFINED CG_TARGET_OS)
        set(CG_TARGET_OS "${CMAKE_SYSTEM_NAME}")
    endif()
    if(NOT DEFINED CG_TARGET_ARCH)
        if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_OSX_ARCHITECTURES)
            set(CG_TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
        else()
            set(CG_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()
    if(NOT DEFINED CG_TARGET_POINTER_SIZE)
        set(CG_TARGET_POINTER_SIZE "${CMAKE_SIZEOF_VOID_P}")
    endif()
    cg_enforce_platform("${CG_TARGET_OS}" "${CG_TARGET_ARCH}" "${CG_TARGET_POINTER_SIZE}")
endif()
