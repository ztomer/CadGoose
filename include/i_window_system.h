#pragma once
#include <string>
#include <functional>

struct WindowSpec {
    float x, y, width, height;
    float level;
    bool clickThrough;
    bool transparent;
    std::string title;
    std::function<void(void*)> drawBlock;
};

class IWindowSystem {
public:
    virtual ~IWindowSystem() = default;

    virtual int CreateWindow(const WindowSpec& spec) = 0;
    virtual void DestroyWindow(int windowId) = 0;
    virtual void UpdatePosition(int windowId, float x, float y, float width, float height) = 0;
    virtual void SetVisible(int windowId, bool visible) = 0;
    virtual void RequestRedraw(int windowId) = 0;
    virtual int WindowCount() const = 0;
};
