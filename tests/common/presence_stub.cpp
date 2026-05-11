// presence_stub.cpp
// Stub for Presence_SetGooseWindowVisible - does nothing in tests
#include <cstdio>

extern "C" void Presence_SetGooseWindowVisible(bool visible) {
    fprintf(stderr, "[Presence Stub] SetGooseWindowVisible(%d)\n", visible);
}