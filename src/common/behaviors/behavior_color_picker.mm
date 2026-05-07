// ===========================
// behavior_color_picker.cpp
// Color Picker Behavior - Change goose color
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

static bool s_enabled = true;

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

@interface ColorPickerDelegate : NSObject <NSWindowDelegate>
+ (void)openColorPanel;
+ (void)closeColorPanel;
@end

@implementation ColorPickerDelegate

+ (void)openColorPanel {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSColorPanel* panel = [NSColorPanel sharedColorPanel];
        [panel setTarget:nil];
        [panel setAction:@selector(changeColor:)];
        [panel makeKeyAndOrderFront:nil];
        [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    });
}

+ (void)closeColorPanel {
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSColorPanel sharedColorPanel] close];
    });
}

@end

extern void ColorPicker_SetColor(float r, float g, float b);
#endif

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

void ColorPicker_Open() {
#ifdef __APPLE__
    [ColorPickerDelegate openColorPanel];
#endif
}

void ColorPicker_Close() {
#ifdef __APPLE__
    [ColorPickerDelegate closeColorPanel];
#endif
}

static Behavior g_colorPickerBehavior = {
    .id = "colorPicker",
    .name = "Color Picker",
    .description = "Change goose color using macOS color panel",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_colorPickerBehavior);