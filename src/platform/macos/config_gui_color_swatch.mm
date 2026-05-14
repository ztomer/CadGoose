// config_gui_color_swatch.mm
// ColorSwatchView — click to open NSColorPanel
#import "config_gui_helpers.h"

@implementation ColorSwatchView

- (void)setColor:(NSColor*)color {
    _color = color;
    self.wantsLayer = YES;
    self.layer.backgroundColor = color.CGColor;
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[NSColor colorWithWhite:0.5 alpha:0.5] CGColor];
}

- (void)mouseUp:(NSEvent*)event {
    [[NSColorPanel sharedColorPanel] setColor:self.color];
    [[NSColorPanel sharedColorPanel] setTarget:self];
    [[NSColorPanel sharedColorPanel] setAction:@selector(colorPickDone:)];
    [[NSColorPanel sharedColorPanel] orderFront:nil];
}

- (void)colorPickDone:(NSColorPanel*)sender {
    if (!self.colorPrefix) return;
    NSColor* c = [sender.color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
    CGFloat r, g, b, a;
    [c getRed:&r green:&g blue:&b alpha:&a];

    NSString* prefix = self.colorPrefix;
    NSString* rId = [prefix stringByAppendingString:@"R"];
    NSString* gId = [prefix stringByAppendingString:@"G"];
    NSString* bId = [prefix stringByAppendingString:@"B"];

    for (NSView* sv in self.superview.subviews) {
        if ([sv.identifier isEqualToString:rId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:r];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", r]];
            }
        } else if ([sv.identifier isEqualToString:gId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:g];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", g]];
            }
        } else if ([sv.identifier isEqualToString:bId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:b];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", b]];
            }
        }
    }
    self.color = [NSColor colorWithRed:r green:g blue:b alpha:1];
}
@end
