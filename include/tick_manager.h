#ifndef TICK_MANAGER_H
#define TICK_MANAGER_H

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

@interface TickManager : NSObject
@property (nonatomic, assign, readonly) double currentTime;
@property (nonatomic, assign, readonly) int tickCount;
+ (instancetype)shared;
- (void)start;
- (void)stop;
- (BOOL)isRunning;
@end

#endif // TICK_MANAGER_H
