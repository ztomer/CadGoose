#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

@interface TestView : NSView
@end

@implementation TestView
- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    
    // Fill white background
    CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
    CGContextFillRect(ctx, self.bounds);
    
    // Fill dark rect
    CGContextSetRGBFillColor(ctx, 0.1, 0.1, 0.1, 1);
    CGContextFillRect(ctx, CGRectMake(10, 10, 80, 80));
    
    // Draw string using CoreText
    CTFontRef font = CTFontCreateWithName(CFSTR("Courier"), 20.0f, NULL);
    CGColorRef white = CGColorCreateGenericRGB(1.0f, 1.0f, 1.0f, 1.0f);
    CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, white };
    CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)"A", 1, kCFStringEncodingUTF8, false);
    CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
    CTLineRef line = CTLineCreateWithAttributedString(attrStr);
    
    CGContextSetTextPosition(ctx, 20, 20);
    CTLineDraw(line, ctx);
    
    CFRelease(line);
    CFRelease(attrStr);
    CFRelease(string);
    CFRelease(attributes);
    CGColorRelease(white);
    CFRelease(font);
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
        [pngData writeToFile:@"test_ct_color.png" atomically:YES];
    }
    return 0;
}
