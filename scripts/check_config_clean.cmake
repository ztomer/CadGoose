# Regression guard: the ctest suite must leave tracked config/ untouched.
#
# CadGooseTests writes real state (every registry change fires
# OnConfigChange -> Config_SaveAll into ConfigDirPath()). The suite is
# sandboxed to /tmp/cadgoose-test-config via CADGOOSE_CONFIG_DIR, but any
# test that unsets that var mid-suite drops every later save through the
# <cwd>/config fallback -- the repo's own tracked config.toml. That was the
# mcp_port 31073->31072 flip. Registered as a ctest test with DEPENDS on
# CadGooseTests so it always runs AFTER the suite, not as a "remember to
# check" step.
execute_process(
    COMMAND git status --porcelain -- config/
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    OUTPUT_VARIABLE goh_status
    ERROR_VARIABLE goh_err
    RESULT_VARIABLE goh_rc
)
if(NOT goh_rc EQUAL 0)
    message(FATAL_ERROR "git status failed: ${goh_err}")
endif()
if(NOT goh_status STREQUAL "")
    message(FATAL_ERROR "the test suite mutated tracked config/:\n${goh_status}")
endif()
message(STATUS "config/ clean after test suite")
