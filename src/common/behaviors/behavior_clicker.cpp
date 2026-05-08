// ===========================
// behavior_clicker.cpp
// Clicker Behavior - Randomly clicks at goose position
// Based on Clicker by Wolf/NE1W01F
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <ApplicationServices/ApplicationServices.h>

static bool s_enabled = true;

static constexpr int CLICK_CHANCE = 300;
static constexpr int MOUSEEVENTF_LEFTDOWN = 0x02;
static constexpr int MOUSEEVENTF_LEFTUP = 0x04;

static CGPoint GetCurrentMousePos() {
    CGEventRef event = CGEventCreate(nullptr);
    if (!event) return CGPointZero;
    CGPoint pos = CGEventGetLocation(event);
    CFRelease(event);
    return pos;
}

static void SimulateClick(float x, float y) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return;

    CGPoint clickPoint = CGPointMake(x, y);

    CGEventRef mouseDown = CGEventCreateMouseEvent(source, kCGEventLeftMouseDown, clickPoint, kCGMouseButtonLeft);
    CGEventRef mouseUp = CGEventCreateMouseEvent(source, kCGEventLeftMouseUp, clickPoint, kCGMouseButtonLeft);

    if (mouseDown && mouseUp) {
        CGEventPost(kCGHIDEventTap, mouseDown);
        CGEventPost(kCGHIDEventTap, mouseUp);
        CFRelease(mouseDown);
        CFRelease(mouseUp);
    }

    CFRelease(source);
}

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.clicker) return;

    int randomValue = rand() % g_config.behaviors.clicker.chance + 1;
    if (randomValue == 3) {
        CGPoint originalPos = GetCurrentMousePos();

        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        if (source) {
            CGEventRef moveEvent = CGEventCreateMouseEvent(source, kCGEventMouseMoved, CGPointMake(goose->pos.x, goose->pos.y), kCGMouseButtonLeft);
            if (moveEvent) {
                CGEventPost(kCGHIDEventTap, moveEvent);
                CFRelease(moveEvent);
            }
            CFRelease(source);
        }

        SimulateClick(goose->pos.x, goose->pos.y);
        SimulateClick(goose->pos.x, goose->pos.y);

        source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        if (source) {
            CGEventRef moveEvent = CGEventCreateMouseEvent(source, kCGEventMouseMoved, originalPos, kCGMouseButtonLeft);
            if (moveEvent) {
                CGEventPost(kCGHIDEventTap, moveEvent);
                CFRelease(moveEvent);
            }
            CFRelease(source);
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_clickerBehavior = {
    .id = "clicker",
    .name = "Clicker",
    .description = "Randomly clicks at the goose's position. Based on Clicker by Wolf",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = true, .isStarter = false }
};

REGISTER_BEHAVIOR(g_clickerBehavior);