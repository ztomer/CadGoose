#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

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
    
    // Draw string using CoreText with Scale(1.0, 1.0)
    CTFontRef font = CTFontCreateWithName(CFSTR("Courier"), 40.0f, NULL);
    CGColorRef red = CGColorCreateGenericRGB(1.0f, 0.0f, 0.0f, 1.0f);
    CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, red };
    CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    // TEST WITH 1.0, 1.0
    CGContextSetTextMatrix(ctx, CGAffineTransformMakeScale(1.0, 1.0));
    
    CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)"A", 1, kCFStringEncodingUTF8, false);
    CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
    CTLineRef line = CTLineCreateWithAttributedString(attrStr);
    
    CGContextSetTextPosition(ctx, 50, 50);
    CTLineDraw(line, ctx);
    
    CGContextRestoreGState(ctx);
}
@end

int main() {
    @autoreleasepool {
        NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 100, 100) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
        TestView *view = [[TestView alloc] initWithFrame:window.contentView.bounds];
        window.contentView = view;
        [view display];
        NSBitmapImageRep *rep = [view bitmapImageRepForCachingDisplayInRect:view.bounds];
        [view cacheDisplayInRect:view.bounds toBitmapImageRep:rep];
        NSData *pngData = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        [pngData writeToFile:@"test_ct.png" atomically:YES];
    }
    return 0;
}
