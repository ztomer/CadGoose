#import <Cocoa/Cocoa.h>

@interface MyView : NSView
@end
@implementation MyView
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor clearColor] setFill];
    NSRectFill(dirtyRect);
    [[NSColor redColor] setFill];
    NSRectFill(NSMakeRect(100, 100, 200, 200));
}
- (NSView *)hitTest:(NSPoint)point {
    NSPoint p = [self convertPoint:point fromView:self.superview];
    if (NSPointInRect(p, NSMakeRect(100, 100, 200, 200))) {
        return self;
    }
    return nil;
}
- (void)mouseDown:(NSEvent *)event {
    NSLog(@"Clicked red square!");
}
@end

int main() {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        NSRect frame = NSMakeRect(0, 0, 800, 600);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSWindowStyleMaskBorderless
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        [window setOpaque:NO];
        [window setBackgroundColor:[NSColor clearColor]];
        [window setIgnoresMouseEvents:NO];
        MyView *view = [[MyView alloc] initWithFrame:frame];
        [window setContentView:view];
        [window makeKeyAndOrderFront:nil];
        [app run];
    }
    return 0;
}
