# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-src")
  file(MAKE_DIRECTORY "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-src")
endif()
file(MAKE_DIRECTORY
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-build"
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix"
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/tmp"
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/src/toml11-populate-stamp"
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/src"
  "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/src/toml11-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/src/toml11-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ztomer/Projects/CadGoose/build-release/_deps/toml11-subbuild/toml11-populate-prefix/src/toml11-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
