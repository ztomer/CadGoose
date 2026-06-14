#include "platform_input.h"
#include <ApplicationServices/ApplicationServices.h>

__attribute__((weak)) bool Platform_IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

__attribute__((weak)) bool Platform_IsMouseButtonDown(int button) {
    return CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, (CGMouseButton)button);
}

__attribute__((weak)) bool Platform_GetMousePosition(double* outX, double* outY) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    if (!source) return false;
    CGEventRef event = CGEventCreate(source);
    CFRelease(source);
    if (!event) return false;
    CGPoint pos = CGEventGetLocation(event);
    CFRelease(event);
    *outX = pos.x;
    *outY = pos.y;
    return true;
}
