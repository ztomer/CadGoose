#pragma once

// EffectWindow — Lightweight floating windows for environmental effects
// (footprints, pomodoro bed). Always click-through, no cursor
// interaction, no close button. Uses circular buffer to cap memory usage.

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSInteger, EffectType) {
    EffectTypeFootprint,
    EffectTypePomodoroBed,
};

@interface EffectContentView : NSView
@property (nonatomic, assign) EffectType effectType;
@property (nonatomic, assign) float posX;
@property (nonatomic, assign) float posY;
@property (nonatomic, assign) float radius;
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) void* cgImage; // CGImageRef for pomodoro bed
@property (nonatomic, assign) int gooseId; // For per-goose effects (pomodoro bed)
- (instancetype)initWithFrame:(NSRect)frame effectType:(EffectType)type;
@end

@interface EffectWindow : NSWindow
@property (nonatomic, assign) EffectType effectType;
@property (nonatomic, assign) float posX;
@property (nonatomic, assign) float posY;
@property (nonatomic, assign) float radius;
@property (nonatomic, assign) void* cgImage; // CGImageRef for pomodoro bed
@property (nonatomic, assign) int gooseId; // For per-goose effects (pomodoro bed)
- (instancetype)initWithType:(EffectType)type posX:(float)x posY:(float)y radius:(float)rad cgImage:(void*)img;
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
