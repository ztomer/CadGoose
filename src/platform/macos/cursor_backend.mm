#import "cursor_backend.h"
#import <CoreGraphics/CoreGraphics.h>

class MacCursorBackend : public CursorBackend {
public:
    MacCursorBackend() : m_eventSource(nullptr) {}

    std::string Name() const override { return "MacCGEvent"; }

    uint32_t Caps() const override {
        return CAP_ABSOLUTE | CAP_RELATIVE;
    }

    bool Init() override {
        m_eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        return m_eventSource != nullptr;
    }

    Vector2 GetCursorPos() override {
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        if (!source) return {-1.0f, -1.0f};

        CGEventRef event = CGEventCreate(source);
        if (!event) {
            CFRelease(source);
            return {-1.0f, -1.0f};
        }

        CGPoint point = CGEventGetLocation(event);
        CFRelease(event);
        CFRelease(source);

        return {(float)point.x, (float)point.y};
    }

    void MoveCursorAbs(int x, int y) override {
        CGEventSourceRef source = m_eventSource ? (CGEventSourceRef)m_eventSource :
                                  CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

        CGEventRef event = CGEventCreateMouseEvent(source,
                                                     kCGEventMouseMoved,
                                                     CGPointMake(x, y),
                                                     kCGMouseButtonLeft);
        if (event) {
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        }

        if (m_eventSource == nullptr && source) CFRelease(source);
    }

    void MoveCursorRel(int dx, int dy) override {
        CGEventSourceRef source = m_eventSource ? (CGEventSourceRef)m_eventSource :
                                  CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

        CGEventRef event = CGEventCreateMouseEvent(source,
                                                     kCGEventMouseMoved,
                                                     CGPointMake(0, 0),
                                                     kCGMouseButtonLeft);
        if (event) {
            CGEventSetIntegerValueField(event, kCGMouseEventDeltaX, dx);
            CGEventSetIntegerValueField(event, kCGMouseEventDeltaY, dy);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        }

        if (m_eventSource == nullptr && source) CFRelease(source);
    }

private:
    CGEventSourceRef m_eventSource;
};