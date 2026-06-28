#import "tick_manager.h"
#import "window.h"
#import "item_window.h"
#import "effect_window.h"
#import "behavior_element_window.h"
#import "world.h"
#import "config.h"
#import "actor.h"
#import "random_util.h"
#import "world_utils.h"
#import "goose.h"
#import "goose_drawing.h"
#import "cg_renderer.h"

#import <os/signpost.h>

void Honcker_Honk(Goose* goose, double time);

static os_log_t g_tickLog = nullptr;
static os_signpost_id_t g_frameSignpostID = 0;

static inline os_log_t TickLog() {
    if (!g_tickLog) g_tickLog = os_log_create("com.cadgoose.frame", "TickManager");
    return g_tickLog;
}

#import <AppKit/AppKit.h>

static constexpr int kWorldCleanupTickInterval = 60;
static constexpr int kLeafSpawnProbabilityDenominator = 500;
static bool s_leafPilesInitialized = false;
static constexpr float kDisplayLinkMinFps = 30;
static constexpr float kDisplayLinkMaxFps = 60;
static constexpr float kDisplayLinkDefaultFps = 60;

@interface TickManager ()
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) int tickCount;
@property (nonatomic, strong) CADisplayLink* displayLink;
@property (nonatomic, strong) id keyMonitor;
@end

@implementation TickManager

+ (instancetype)shared {
    static TickManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[TickManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _currentTime = 0.0;
        _tickCount = 0;
        _displayLink = nil;
        _keyMonitor = nil;
    }
    return self;
}

- (void)dealloc {
    if (_keyMonitor) {
        [NSEvent removeMonitor:_keyMonitor];
    }
}

- (void)setupGlobalKeyMonitor {
    if (self.keyMonitor) return;
    __weak TickManager* weakSelf = self;
    self.keyMonitor = [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskKeyDown
        handler:^(NSEvent* event) {
            // The global event monitor runs on a private background thread.
            // Dispatch all work to the main thread to synchronize access to
            // ActorManager, g_config, and other shared state.
            dispatch_async(dispatch_get_main_queue(), ^{
                TickManager* strongSelf = weakSelf;
                if (!strongSelf) return;
                NSString* chars = [event characters];
                if (chars.length == 0) return;
                unichar key = [chars characterAtIndex:0];
                if (key == 'f' || key == 'F') {
                    for (auto* g : ActorManager::Instance().getGeese()) {
                        if (!g || !g->isActive()) continue;
                        Honcker_Honk(g, strongSelf.currentTime);
                    }
                }
            });
        }];
}

- (void)start {
    [self stop];
    [self setupGlobalKeyMonitor];
    NSScreen* screen = [NSScreen mainScreen];
    self.displayLink = [screen displayLinkWithTarget:self selector:@selector(onFrameRefresh:)];
    if (@available(macOS 14.0, *)) {
        self.displayLink.preferredFrameRateRange = CAFrameRateRangeMake(kDisplayLinkMinFps, kDisplayLinkMaxFps, kDisplayLinkDefaultFps);
    }
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stop {
    [self.displayLink invalidate];
    self.displayLink = nil;
}

- (BOOL)isRunning {
    return self.displayLink != nil;
}

- (void)onFrameRefresh:(CADisplayLink*)displayLink {
    [self tick];
}

- (void)tick {
    double dt = g_config.render.frameDt;
    self.currentTime += dt;
    self.tickCount++;

    g_time = self.currentTime;

    if (!g_frameSignpostID) g_frameSignpostID = os_signpost_id_generate(TickLog());
    os_signpost_interval_begin(TickLog(), g_frameSignpostID, "Frame", "dt=%.3f", dt);

    // Tick all actors
    ActorManager::Instance().tickAll(g_world, dt, self.currentTime);
    ActorManager::Instance().cleanup();

    // Phase 3: actors create/update their windows via renderAll
    ActorManager::Instance().renderAll(nullptr);

    // Phase 3: per-goose window position updates (no full-screen overlay)
    for (auto* g : ActorManager::Instance().getGeese()) {
        if (!g || !g->isActive()) continue;
        float winSize = CalculateGooseWindowSize(g);
        float winX = g->pos.x - winSize / 2.0f;
        float winY = g->pos.y - winSize / 2.0f;
        if (!g->m_perGooseWindow) {
            Goose* captured = g;
            BehaviorElementWindow* win = [[BehaviorElementWindow alloc]
                initWithDrawBlock:^(CGContextRef cgCtx) {
                    float ox = captured->pos.x - captured->m_perGooseWindowSize / 2.0f;
                    float oy = captured->pos.y - captured->m_perGooseWindowSize / 2.0f;
                    CGContextTranslateCTM(cgCtx, -ox, -oy);
                    CGRenderer r(cgCtx);
                    captured->draw(&r);
                }
                deviceX:winX deviceY:winY width:winSize height:winSize];
            win.level = NSStatusWindowLevel + 5; // Above memes and all other actors
            g->m_perGooseWindow = (__bridge_retained void*)win;
            g->m_perGooseWindowKey = (__bridge_retained void*)[[BehaviorElementWindowManager shared] registerWindow:win];
            g->m_perGooseWindowSize = winSize;
        } else {
            BehaviorElementWindow* win = (__bridge BehaviorElementWindow*)g->m_perGooseWindow;
            [win updatePosition:winX y:winY width:winSize height:winSize];
            [(BehaviorElementContentView*)win.contentView setNeedsDisplay:YES];
            g->m_perGooseWindowSize = winSize;
        }
    }
    [[ItemWindowManager shared] showPendingWindows];

    if (self.tickCount % kWorldCleanupTickInterval == 0) {
        World_CleanupExpired(self.currentTime);
    }

    auto geese = ActorManager::Instance().getGeese();

    if (g_config.behaviors.fun.autumnLeaves) {
        if (!s_leafPilesInitialized) {
            s_leafPilesInitialized = true;
            for (int i = 0; i < 3; i++) {
                World_SpawnRandomLeafPile(g_world.screenWidth, g_world.screenHeight, self.currentTime);
            }
        } else if (rng_util::RandRange(kLeafSpawnProbabilityDenominator) == 0) {
            World_SpawnRandomLeafPile(g_world.screenWidth, g_world.screenHeight, self.currentTime);
        }
        World_TickLeafPiles(self.currentTime, dt,
                            geese.empty() ? nullptr : geese.front());
    }

    [[EffectWindowManager shared] syncWindows];

    [[ItemWindowManager shared] syncWindows];

    os_signpost_interval_end(TickLog(), g_frameSignpostID, "Frame");
}

@end
