#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

@interface GooseView : NSView
- (void)startAnimation;
- (void)stopAnimation;
@property (nonatomic, readonly) BOOL isPrimary;
+ (void)resetPrimaryGuard;
@end