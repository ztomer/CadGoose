#import "mac_cursor_backend.h"
#import "goose_math.h"
#import <CoreGraphics/CoreGraphics.h>
#include <string>

MacCursorBackend::MacCursorBackend() : m_eventSource(nullptr) {}

std::string MacCursorBackend::Name() const { return "MacCGEvent"; }

uint32_t MacCursorBackend::Caps() const { return 0x1 | 0x2 | 0x4; }

bool MacCursorBackend::Init() {
    m_eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    return m_eventSource != nullptr;
}

Vector2 MacCursorBackend::GetCursorPos() {
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

void MacCursorBackend::MoveCursorAbs(int x, int y) {
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

void MacCursorBackend::MoveCursorRel(int dx, int dy) {
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