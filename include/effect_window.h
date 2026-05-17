#pragma once

// EffectWindow — Lightweight floating windows for environmental effects
// (autumn leaves, mud splatters, etc.). Always click-through, no cursor
// interaction, no close button. Uses circular buffer to cap memory usage.

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSInteger, EffectType) {
    EffectTypeLeafPile,
    EffectTypeFootprint,
};

@interface EffectContentView : NSView
@property (nonatomic, assign) EffectType effectType;
@property (nonatomic, assign) float posX;
@property (nonatomic, assign) float posY;
@property (nonatomic, assign) float radius;
@property (nonatomic, assign) double currentTime;
- (instancetype)initWithFrame:(NSRect)frame effectType:(EffectType)type;
@end

@interface EffectWindow : NSWindow
@property (nonatomic, assign) EffectType effectType;
@property (nonatomic, assign) float posX;
@property (nonatomic, assign) float posY;
@property (nonatomic, assign) float radius;
@property (nonatomic, assign) double currentTime;
- (instancetype)initWithType:(EffectType)type posX:(float)x posY:(float)y radius:(float)rad;
- (void)updatePosition;
- (void)closeAndRemove;
@end

// Manager with circular buffer limit to prevent memory leaks
@interface EffectWindowManager : NSObject
+ (instancetype)shared;
- (void)syncWindows;
- (void)closeAll;
@end

#endif
