#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

@interface ConfigGUIRenderingTest : XCTestCase
@end

@implementation ConfigGUIRenderingTest

- (void)setUp {
    // Ensure NSApp is running
    if (NSApp == nil) {
        [NSApplication sharedApplication];
    }
}

- (void)testAppearanceTabBackgroundIsDark {
    // Create a window with the same setup as ConfigGUI
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 830)
                                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskFullSizeContentView
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
    window.titleVisibility = NSWindowTitleHidden;
    window.titlebarAppearsTransparent = YES;
    window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    
    NSView* contentView = window.contentView;
    contentView.wantsLayer = YES;
    contentView.layer.backgroundColor = [[NSColor colorWithWhite:0.12 alpha:1.0] CGColor];
    
    // Content container (no wantsLayer, like committed version)
    NSView* contentContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 792)];
    contentContainer.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    [contentView addSubview:contentContainer];
    
    // Create AppearanceTabView 
    // Need to import the class - use direct view hierarchy
    NSView* appearanceView = [[NSView alloc] initWithFrame:contentContainer.bounds];
    appearanceView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    appearanceView.hidden = NO;
    [contentContainer addSubview:appearanceView];
    
    // Add subviews like the AppearanceTabView does
    // Appearance label
    NSTextField* appLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, 764, 80, 20)];
    appLabel.stringValue = @"Appearance:";
    appLabel.textColor = [NSColor whiteColor];
    appLabel.backgroundColor = [NSColor clearColor];
    appLabel.bordered = NO;
    appLabel.editable = NO;
    [appearanceView addSubview:appLabel];
    
    // Popup
    NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(96, 760, 130, 24)];
    [popup addItemWithTitle:@"Light"];
    [popup addItemWithTitle:@"Dark"];
    [popup addItemWithTitle:@"System"];
    [popup addItemWithTitle:@"Custom"];
    [appearanceView addSubview:popup];
    
    // Body label
    NSTextField* bodyLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, 722, 44, 16)];
    bodyLabel.stringValue = @"Body";
    bodyLabel.textColor = [NSColor whiteColor];
    bodyLabel.backgroundColor = [NSColor clearColor];
    bodyLabel.bordered = NO;
    bodyLabel.editable = NO;
    [appearanceView addSubview:bodyLabel];
    
    // R slider
    NSSlider* rSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(60, 722, 90, 16)];
    rSlider.minValue = 0; rSlider.maxValue = 1;
    [appearanceView addSubview:rSlider];
    
    // R value
    NSTextField* rVal = [[NSTextField alloc] initWithFrame:NSMakeRect(154, 722, 36, 16)];
    rVal.stringValue = @"0.50";
    rVal.textColor = [NSColor whiteColor];
    rVal.backgroundColor = [NSColor clearColor];
    rVal.bordered = NO;
    rVal.editable = YES;
    [appearanceView addSubview:rVal];
    
    // G slider
    NSSlider* gSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(196, 722, 90, 16)];
    gSlider.minValue = 0; gSlider.maxValue = 1;
    [appearanceView addSubview:gSlider];
    
    // G value
    NSTextField* gVal = [[NSTextField alloc] initWithFrame:NSMakeRect(290, 722, 36, 16)];
    gVal.stringValue = @"0.50";
    gVal.textColor = [NSColor whiteColor];
    gVal.backgroundColor = [NSColor clearColor];
    gVal.bordered = NO;
    gVal.editable = YES;
    [appearanceView addSubview:gVal];
    
    // B slider
    NSSlider* bSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(332, 722, 90, 16)];
    bSlider.minValue = 0; bSlider.maxValue = 1;
    [appearanceView addSubview:bSlider];
    
    // B value
    NSTextField* bVal = [[NSTextField alloc] initWithFrame:NSMakeRect(426, 722, 36, 16)];
    bVal.stringValue = @"0.50";
    bVal.textColor = [NSColor whiteColor];
    bVal.backgroundColor = [NSColor clearColor];
    bVal.bordered = NO;
    bVal.editable = YES;
    [appearanceView addSubview:bVal];
    
    // Force layout and display
    [window layoutIfNeeded];
    [window displayIfNeeded];
    
    // Capture bitmap of contentContainer
    NSBitmapImageRep* bitmap = [contentContainer bitmapImageRepForCachingDisplayInRect:contentContainer.bounds];
    [contentContainer cacheDisplayInRect:contentContainer.bounds toBitmapImageRep:bitmap];
    
    // Analyze pixels - check content area (below the controls at ~700)
    int darkCount = 0, grayCount = 0, totalPixels = 0;
    for (int y = 0; y < (int)bitmap.pixelsHigh; y += 5) {
        for (int x = 0; x < (int)bitmap.pixelsWide; x += 5) {
            NSColor* color = [bitmap colorAtX:x y:y];
            CGFloat r, g, b, a;
            [color getRed:&r green:&g blue:&b alpha:&a];
            totalPixels++;
            if (r < 0.2 && g < 0.2 && b < 0.2) darkCount++;
            else if (r > 0.3 && r < 0.7 && g > 0.3 && g < 0.7 && b > 0.3 && b < 0.7 &&
                     fabs(r-g) < 0.1 && fabs(g-b) < 0.1) grayCount++;
        }
    }
    
    NSLog(@"Analysis: dark=%d/%d (%.0f%%) gray=%d/%d (%.0f%%)",
          darkCount, totalPixels, 100.0*darkCount/totalPixels,
          grayCount, totalPixels, 100.0*grayCount/totalPixels);
    
    // The background should be mostly dark
    XCTAssertGreaterThan(darkCount, totalPixels * 0.5,
                         "Expected mostly dark pixels in background area");
    XCTAssertLessThan(grayCount, totalPixels * 0.3,
                      "Expected few gray pixels in background area");
}

@end
