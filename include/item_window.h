#pragma once

// ItemWindow — A per-item floating window for dropped memes/text/toys.
// Each item gets its own window so dragging works without blocking
// clicks on apps underneath. Uses dynamic ignoresMouseEvents toggling
// based on cursor position to achieve true click-through outside the
// item's rectangular bounds.

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#include "items.h"

@interface ItemContentView : NSView
@property (nonatomic, assign) DroppedItem* item;
@property (nonatomic, weak) NSWindow* parentWindow;
- (instancetype)initWithFrame:(NSRect)frame item:(DroppedItem*)item;
@end

@interface ItemWindow : NSWindow
@property (nonatomic, assign) DroppedItem* item;
- (instancetype)initWithItem:(DroppedItem*)item;
- (void)updatePosition;
- (void)closeAndRemove;
// Returns YES if a local view point is inside the item's rotated rectangle
- (BOOL)isPointInsideItem:(NSPoint)pt;
@end

// Manager for all item windows — handles dynamic ignoresMouseEvents toggling
// based on cursor position (done in syncWindows, no separate timer)
@interface ItemWindowManager : NSObject
+ (instancetype)shared;
- (void)syncWindows;
- (void)closeAll;
@property (nonatomic, readonly) NSMutableDictionary<NSNumber*, ItemWindow*>* windows;
@end

// Programmatic drag test — synthesizes CGEvents to drag the first dropped item
void ItemWindow_DragTest(float targetX, float targetY);

#endif
