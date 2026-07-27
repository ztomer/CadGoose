#include "i_window_system.h"
#include <vector>
#include <algorithm>
#include <string>

struct MockWindow {
    int id;
    WindowSpec spec;
    bool visible;
};

class MockWindowSystem : public IWindowSystem {
public:
    std::vector<MockWindow> windows;
    int nextId = 1;
    int createCallCount = 0;
    int destroyCallCount = 0;
    int updatePositionCallCount = 0;
    int setVisibleCallCount = 0;
    int requestRedrawCallCount = 0;

    int CreateWindow(const WindowSpec& spec) override {
        createCallCount++;
        int id = nextId++;
        windows.push_back({id, spec, true});
        return id;
    }

    void DestroyWindow(int windowId) override {
        destroyCallCount++;
        auto it = std::remove_if(windows.begin(), windows.end(),
            [windowId](const MockWindow& w) { return w.id == windowId; });
        windows.erase(it, windows.end());
    }

    void UpdatePosition(int windowId, float x, float y, float width, float height) override {
        updatePositionCallCount++;
        for (auto& w : windows) {
            if (w.id == windowId) {
                w.spec.x = x;
                w.spec.y = y;
                w.spec.width = width;
                w.spec.height = height;
                break;
            }
        }
    }

    void SetVisible(int windowId, bool visible) override {
        setVisibleCallCount++;
        for (auto& w : windows) {
            if (w.id == windowId) {
                w.visible = visible;
                break;
            }
        }
    }

    void RequestRedraw(int windowId) override {
        requestRedrawCallCount++;
    }

    int WindowCount() const override { return (int)windows.size(); }
};

static MockWindowSystem* g_mock = nullptr;

MockWindowSystem* GetMockWindowSystem() {
    if (!g_mock) g_mock = new MockWindowSystem();
    return g_mock;
}

void ResetMockWindowSystem() {
    delete g_mock;
    g_mock = nullptr;
}

IWindowSystem* GetWindowSystem() {
    return GetMockWindowSystem();
}
