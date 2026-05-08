#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

@interface TestView : NSView
@end

@implementation TestView
- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    self.wantsLayer = YES;
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    
    // Fill white
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, self.bounds);
    
    // DO WHAT CADGOOSE DOES
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, self.bounds.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    
    // Draw string using AppKit
    NSString* text = @"AppKit";
    NSDictionary* textAttrs = @{
        NSFontAttributeName: [NSFont systemFontOfSize:20.0],
        NSForegroundColorAttributeName: [NSColor blackColor]
    };
    
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 20, 50);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    [text drawInRect:NSMakeRect(0, 0, 200, 50) withAttributes:textAttrs];
    CGContextRestoreGState(ctx);
    
    CGContextRestoreGState(ctx);
}
@end

int main() {
    @autoreleasepool {
        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 300, 200) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
        TestView *view = [[TestView alloc] initWithFrame:window.contentView.bounds];
        window.contentView = view;
        [view display]; // force drawRect
        
        // Capture view to image
        NSBitmapImageRep *rep = [view bitmapImageRepForCachingDisplayInRect:view.bounds];
        [view cacheDisplayInRect:view.bounds toBitmapImageRep:rep];
        NSData *pngData = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        [pngData writeToFile:@"test_layer.png" atomically:YES];
    }
    return 0;
}
