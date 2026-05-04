#ifndef MACOS_CURSOR_BACKEND_H
#define MACOS_CURSOR_BACKEND_H

#include "cursor_backend.h"

class MacCursorBackend : public CursorBackend {
public:
    MacCursorBackend();
    std::string Name() const override;
    uint32_t Caps() const override;
    bool Init() override;
    Vector2 GetCursorPos() override;
    void MoveCursorAbs(int x, int y) override;
    void MoveCursorRel(int dx, int dy) override;
};

#endif