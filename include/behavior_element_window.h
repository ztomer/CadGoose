#pragma once

// BehaviorElementWindow — Lightweight floating windows for behavior-rendered
// elements that live at fixed screen positions (ball, portals, bed, toys, etc.).
// Click-through, no interaction, sized to content.

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

// Drawing block: receives a CGContext and the element's device-coordinate position.
// The context is already translated so (0,0) is the element's center.
typedef void (^BehaviorElementDrawBlock)(CGContextRef ctx);

@interface BehaviorElementContentView : NSView
@property (nonatomic, copy) BehaviorElementDrawBlock drawBlock;
- (instancetype)initWithFrame:(NSRect)frame drawBlock:(BehaviorElementDrawBlock)block;
@end

@interface BehaviorElementWindow : NSWindow
@property (nonatomic, copy) BehaviorElementDrawBlock drawBlock;
- (instancetype)initWithDrawBlock:(BehaviorElementDrawBlock)block
                         deviceX:(float)x deviceY:(float)y
                           width:(float)w height:(float)h;
- (void)updatePosition:(float)x y:(float)y width:(float)w height:(float)h;
- (void)closeAndRemove;
@end

@interface BehaviorElementWindowManager : NSObject
+ (instancetype)shared;
- (NSNumber*)registerWindow:(BehaviorElementWindow*)window;
- (void)unregisterWindow:(NSNumber*)key;
- (void)syncWindows;
- (void)closeAll;
@end

#endif
