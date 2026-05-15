#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

@interface GooseView : NSView
- (void)startAnimation;
- (void)stopAnimation;
@property (nonatomic, readonly) BOOL isPrimary;
+ (void)resetPrimaryGuard;
- (void)mouseDownAtPoint:(NSPoint)pt viewY:(float)viewY;
- (void)mouseDraggedAtPoint:(NSPoint)pt viewY:(float)viewY;
- (void)mouseUpAtPoint:(NSPoint)pt;
@end