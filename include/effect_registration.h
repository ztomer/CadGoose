// effect_registration.h
// Registration protocol for EffectWindow types.
// Each effect type registers itself from its own source file — no central registry.
// EffectWindowManager iterates registrations generically.

#pragma once

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#include "coordinate_system.h"
#include <vector>

// Forward declarations
@class EffectWindow;
@class EffectContentView;

struct EffectRegistration {
    int type;  // EffectType enum value

    // Returns all active effect positions for this type
    std::vector<Vector2> (*getPositions)();

    // Returns the visual radius for an effect at given position
    float (*getRadius)(const Vector2& pos);

    // Returns true if an effect still exists at given position (for stale window cleanup)
    bool (*existsAt)(const Vector2& pos);

    // Optional: configure window properties beyond position/radius (e.g. gooseId, cgImage)
    void (*configureWindow)(EffectWindow* win, const Vector2& pos);

    // Optional: configure content view properties for drawing (e.g. growth, hue, stemHeight)
    void (*configureContentView)(EffectContentView* cv, const Vector2& pos);
};

// Register an effect type. Call once per effect type, typically from a static initializer.
void EffectRegister(const EffectRegistration& reg);

// Get all registered effect types. EffectWindowManager iterates this.
const std::vector<EffectRegistration>& EffectGetRegistrations();

#endif
