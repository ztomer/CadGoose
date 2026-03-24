#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

@interface GooseView : NSView
- (void)startAnimation;
- (void)stopAnimation;
- (void)handleClickAtPoint:(NSPoint)point;
@end