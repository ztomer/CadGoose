#pragma once

void PlatformInputMock_SetKeyState(int keyCode, bool pressed);
void PlatformInputMock_SetMouseDown(bool down);
void PlatformInputMock_SetMousePosition(double x, double y, bool valid = true);
void PlatformInputMock_Reset();
