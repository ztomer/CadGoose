#!/bin/bash
g++ -std=c++23 -Iinclude -Isrc/common -Isrc/platform/macos src/common/*.cpp tests/common/*.cpp tests/test_main_legacy.cpp -o build/LegacyTests
