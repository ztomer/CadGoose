#ifndef CURSOR_IO_H
#define CURSOR_IO_H

#include "goose_math.h"
#include <cstdint>

enum CursorCaps {
    CAP_NONE     = 0,
    CAP_GET_POS  = 1 << 0,
    CAP_MOVE_ABS = 1 << 1,
    CAP_MOVE_REL = 1 << 2,
    CAP_CLICK    = 1 << 3,
};

struct CursorState {
    Vector2 position = {-1.0f, -1.0f};
    uint32_t caps = CAP_NONE;
    bool hasPos() const { return (caps & CAP_GET_POS) && position.x >= 0 && position.y >= 0; }
};

struct CursorAction {
    enum Type { NONE, MOVE_ABS, MOVE_REL } type = NONE;
    int x = 0, y = 0;
    static CursorAction MoveAbs(int x, int y) { return {MOVE_ABS, x, y}; }
    static CursorAction MoveRel(int dx, int dy) { return {MOVE_REL, dx, dy}; }
    bool isNone() const { return type == NONE; }
};

class ICursorProvider {
public:
    virtual ~ICursorProvider() = default;
    virtual CursorState Read() = 0;
    virtual void Execute(const CursorAction& action) = 0;
};

extern ICursorProvider* g_cursorProvider;

#endif // CURSOR_IO_H
