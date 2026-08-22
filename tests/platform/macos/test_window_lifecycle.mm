// Revived window-lifecycle tests.
//
// History: the original file investigated the window-trail bug by capturing
// screen regions with CGWindowListCreateImage (obsoleted in macOS 15) and
// overriding ItemWindow's -drawRect:, which no longer exists. The trail bug
// itself was fixed and has its own harness (WindowTrailTest, requires a
// display). What remains worth testing here — and what nothing else reaches —
// is REAL ItemWindow creation: a live NSWindow sized to the item's rotated
// bounds, ordered front, drawn, and closed without leaking into the manager.
// These tests drive that real path headlessly-safe: they create windows but
// never capture pixels, so they run anywhere AppKit can initialize.
//
// Registered in CMakeLists.txt under TEST_SOURCES_PLATFORM (APPLE only).

#import "item_window.h"
#import "config.h"
#import "items.h"
#import "actor.h"
#import "actor_dropped_item.h"
#import "actor_manager.h"

#import <Cocoa/Cocoa.h>
#include <gtest/gtest.h>

#include <cmath>

namespace {

// Spawns a live DroppedItemActor so ItemWindow's IsItemValid() sees the item
// as owned by a registered actor (a bare DroppedItem would be treated as
// stale and silently null out). The actor's own window creation is bypassed
// by CADGOOSE_HEADLESS_TEST, so the test creates the ItemWindow explicitly.
class ItemFixture {
public:
    ItemFixture(float x, float y, float w, float h, float rotation) {
        m_data = new ItemData();
        m_data->type = ItemData::MEME;
        m_data->w = w;
        m_data->h = h;
        m_data->image = nil;
        m_drop.data = m_data;
        m_drop.pos = {x, y};
        m_drop.rotation = rotation;
        m_drop.timeDropped = g_time;
        m_drop.pinned = false;
        m_actor = new DroppedItemActor(m_drop);
    }

    ~ItemFixture() {
        // The manager OWNS actors: deactivate and let cleanup() delete the
        // actor through the liveSet bookkeeping. Deleting it here would leave
        // a dangling pointer in ActorManager — exactly the class of heap
        // corruption run_tests_ci.sh's header warns about.
        m_actor->setActive(false);
        delete m_actor->item().data;
        m_actor->item().data = nullptr;  // ~DroppedItemActor skips its own delete
        ActorManager::Instance().cleanup();
    }

    DroppedItem* drop() { return &m_actor->item(); }

private:
    ItemData* m_data = nullptr;
    DroppedItem m_drop{};
    DroppedItemActor* m_actor = nullptr;
};

void SpinRunLoop(double seconds) {
    NSDate* until = [NSDate dateWithTimeIntervalSinceNow:seconds];
    while ([NSDate date].timeIntervalSince1970 < until.timeIntervalSince1970 &&
           [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                    beforeDate:until]) {
        // drain
    }
}

TEST(WindowLifecycleTest, WindowSizedToRotatedBounds) {
    ASSERT_TRUE([NSThread isMainThread]);

    float scale = g_config.general.globalScale;

    // Rotation 0: window = unrotated item size * global scale.
    {
        ItemFixture fx(500, 500, 100, 80, 0.0f);
        ItemWindow* win = nil;
        @autoreleasepool {
            win = [[ItemWindow alloc] initWithItem:fx.drop()];
        }
        EXPECT_NEAR(win.frame.size.width, 100.0f * scale, 1.0f);
        EXPECT_NEAR(win.frame.size.height, 80.0f * scale, 1.0f);
        [win closeAndRemove];
    }

    // Quarter turn: dimensions swap (|cos|=|sin|=1 cross-terms).
    {
        ItemFixture fx(500, 500, 100, 80, static_cast<float>(M_PI_2));
        ItemWindow* win = nil;
        @autoreleasepool {
            win = [[ItemWindow alloc] initWithItem:fx.drop()];
        }
        EXPECT_NEAR(win.frame.size.width, 80.0f * scale, 1.0f);
        EXPECT_NEAR(win.frame.size.height, 100.0f * scale, 1.0f);
        [win closeAndRemove];
    }
}

TEST(WindowLifecycleTest, OrderFrontShowsCloseHides) {
    ASSERT_TRUE([NSThread isMainThread]);

    ItemFixture fx(600, 400, 120, 90, 0.0f);
    ItemWindow* win = [[ItemWindow alloc] initWithItem:fx.drop()];
    EXPECT_FALSE(win.isVisible);

    win.shown = YES;
    [win orderFront:nil];

    // Let the window server composite and any deferred display fire.
    SpinRunLoop(0.15);
    EXPECT_TRUE(win.isVisible);

    [win closeAndRemove];
    SpinRunLoop(0.05);
    EXPECT_FALSE(win.isVisible);
}

TEST(WindowLifecycleTest, ManagerRegistrationRoundTrip) {
    ASSERT_TRUE([NSThread isMainThread]);

    ItemFixture fx(300, 300, 64, 64, 0.0f);
    ItemWindowManager* manager = [ItemWindowManager shared];
    const NSUInteger before = manager.windows.count;

    ItemWindow* win = [[ItemWindow alloc] initWithItem:fx.drop()];
    NSNumber* key = @987654;
    manager.windows[key] = win;
    EXPECT_EQ(manager.windows.count, before + 1);

    [win clearItem];  // detach item before removal, as the actor does
    [manager.windows removeObjectForKey:key];
    [win close];
    EXPECT_EQ(manager.windows.count, before);
}

TEST(WindowLifecycleTest, HitTestCenterInsideCornersOutside) {
    ASSERT_TRUE([NSThread isMainThread]);

    ItemFixture fx(500, 500, 200, 100, 0.0f);
    ItemWindow* win = [[ItemWindow alloc] initWithItem:fx.drop()];

    float scale = g_config.general.globalScale;
    CGFloat W = 200.0f * scale;
    CGFloat H = 100.0f * scale;

    // View coords are flipped (top-left origin, Y-down).
    EXPECT_TRUE([win isPointInsideItem:NSMakePoint(W / 2, H / 2)]);
    EXPECT_FALSE([win isPointInsideItem:NSMakePoint(-20, H / 2)]);
    EXPECT_FALSE([win isPointInsideItem:NSMakePoint(W + 20, H / 2)]);
    EXPECT_FALSE([win isPointInsideItem:NSMakePoint(W / 2, -20)]);

    [win closeAndRemove];
}

}  // namespace
