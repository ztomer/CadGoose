#include "platform_input_mock.h"
#include "platform_input.h"

static int s_captureKeyCode = 0;
static bool s_keyPressed = false;
static bool s_mouseDown = false;
static double s_mouseX = 0;
static double s_mouseY = 0;
static bool s_mouseValid = true;

void PlatformInputMock_SetKeyState(int keyCode, bool pressed) {
    s_captureKeyCode = keyCode;
    s_keyPressed = pressed;
}

void PlatformInputMock_SetMouseDown(bool down) {
    s_mouseDown = down;
}

void PlatformInputMock_SetMousePosition(double x, double y, bool valid) {
    s_mouseX = x;
    s_mouseY = y;
    s_mouseValid = valid;
}

void PlatformInputMock_Reset() {
    s_captureKeyCode = 0;
    s_keyPressed = false;
    s_mouseDown = false;
    s_mouseX = 0;
    s_mouseY = 0;
    s_mouseValid = true;
}

bool Platform_IsKeyPressed(int keyCode) {
    return (keyCode == s_captureKeyCode) && s_keyPressed;
}

bool Platform_IsMouseButtonDown(int) {
    return s_mouseDown;
}

bool Platform_GetMousePosition(double* outX, double* outY) {
    *outX = s_mouseX;
    *outY = s_mouseY;
    return s_mouseValid;
}
