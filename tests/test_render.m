#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

@interface TestView : NSView
@end

@implementation TestView
- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    
    // Fill white
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, self.bounds);
    
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
        NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(300, 200)];
        [img lockFocus];
        TestView *view = [[TestView alloc] initWithFrame:NSMakeRect(0, 0, 300, 200)];
        [view drawRect:view.bounds];
        [img unlockFocus];
        
        NSData *tiffData = [img TIFFRepresentation];
        NSBitmapImageRep *bitmapRep = [NSBitmapImageRep imageRepWithData:tiffData];
        NSData *pngData = [bitmapRep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        [pngData writeToFile:@"test_out.png" atomically:YES];
    }
    return 0;
}