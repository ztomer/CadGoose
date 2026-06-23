// actor.mm
// ActorManager macOS-only bits (closeWindowOnMainThread).
// The bulk of ActorManager lives in actor.cpp for cross-platform use.

#include "actor.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

void Actor::closeWindowOnMainThread(void (^closeBlock)()) {
#ifdef __APPLE__
    if (!closeBlock) return;
    if ([NSThread isMainThread]) {
        closeBlock();
    } else {
        dispatch_sync(dispatch_get_main_queue(), closeBlock);
    }
#else
    (void)closeBlock;
#endif
}
