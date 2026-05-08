#import <Cocoa/Cocoa.h>

@interface TestView : NSView
@end

@implementation TestView
- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, self.bounds);
    
    // Simulate renderer.mm base context (Y-DOWN)
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, self.bounds.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    
    // Create an image (a simple "F" letter to see orientation)
    NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(10, 10)];
    [img lockFocus];
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(0, 0, 10, 10)); // black background
    [[NSColor whiteColor] set];
    // Draw an F (remember NSImage lockFocus is Y-UP by default)
    NSRectFill(NSMakeRect(2, 2, 2, 6)); // vertical stem
    NSRectFill(NSMakeRect(2, 6, 4, 2)); // top bar
    NSRectFill(NSMakeRect(2, 4, 3, 2)); // middle bar
    [img unlockFocus];
    
    CGImageRef cgImg = [img CGImageForProposedRect:NULL context:nil hints:nil];
    
    // Draw using goose_drawing.mm logic:
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 50, 50 + 10);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    CGContextDrawImage(ctx, CGRectMake(0, 0, 10, 10), cgImg);
    CGContextRestoreGState(ctx);
    
    CGContextRestoreGState(ctx);
}
@end

int main() {
    @autoreleasepool {
        NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(100, 100)];
        [img lockFocus];
        TestView *view = [[TestView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
        [view drawRect:view.bounds];
        [img unlockFocus];
        NSData *pngData = [[NSBitmapImageRep imageRepWithData:[img TIFFRepresentation]] representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        [pngData writeToFile:@"out_img.png" atomically:YES];
    }
    return 0;
}
