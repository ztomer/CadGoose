// actor_dropped_item.mm
// DroppedItem actor implementation — owns DroppedItem data, manages window.

#include "actor_dropped_item.h"
#include "actor.h"
#include "item_window.h"
#include "config.h"
#include <cstdio>
#include <mach/mach_time.h>
#include <TargetConditionals.h>

static double GetTimeMs() {
    static mach_timebase_info_data_t info = {0};
    if (info.denom == 0) mach_timebase_info(&info);
    uint64_t now = mach_absolute_time();
    return (double)now * (double)info.numer / (double)info.denom / 1e6;
}

DroppedItemActor::DroppedItemActor(const DroppedItem& item)
    : m_item(item)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    m_position = {item.pos.x, item.pos.y};
    m_radius = item.data ? std::max(item.data->w, item.data->h) * 0.5f : 0;
    m_active = true;

    ActorManager::Instance().add(this);

#ifdef __APPLE__
    initWindow();
#endif
}

DroppedItemActor::~DroppedItemActor() {
#ifdef __APPLE__
    if (m_window) {
        ItemWindow* win = (__bridge ItemWindow*)m_window;
        [win clearItem];

        void* windowPtr = m_window;
        void* keyPtr = m_windowKey;
        m_window = nullptr;
        m_windowKey = nullptr;
        closeWindowOnMainThread(^{
            ItemWindow* w = (__bridge_transfer ItemWindow*)windowPtr;
            NSNumber* key = (__bridge_transfer NSNumber*)keyPtr;
            [[ItemWindowManager shared].windows removeObjectForKey:key];
            [w close];
        });
    }
#endif
    if (m_item.data) {
        delete m_item.data;
        m_item.data = nullptr;
    }
}

bool DroppedItemActor::isExpired() const {
    if (!m_item.data || m_item.pinned) return false;
    double elapsed = g_time - m_item.timeDropped;
    return elapsed > g_config.item.itemLifetime;
}

void DroppedItemActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx;
    (void)dt;
    (void)time;

#ifdef __APPLE__
    updateWindow();
#endif
}

void DroppedItemActor::render(IRenderer* renderer) {
    (void)renderer;
}

#ifdef __APPLE__

void DroppedItemActor::initWindow() {
    if (getenv("CADGOOSE_HEADLESS_TEST")) {
        return;
    }
    double t0 = GetTimeMs();
    ItemWindowManager* manager = [ItemWindowManager shared];
    ItemWindow* win = [[ItemWindow alloc] initWithItem:&m_item];
    double t1 = GetTimeMs();
    m_window = (__bridge_retained void*)win;

    static NSInteger s_nextKey = 1000;
    m_windowKey = (__bridge_retained void*)@(s_nextKey++);
    manager.windows[(__bridge NSNumber*)m_windowKey] = win;
    double t2 = GetTimeMs();
    fprintf(stderr, "[DROP_TIMING] initWindow item=(%.1f,%.1f) allocInit=%.3fms storeKey=%.3fms total=%.3fms\n",
        m_item.pos.x, m_item.pos.y,
        (t1 - t0), (t2 - t1), (t2 - t0));
}

void DroppedItemActor::updateWindow() {
    if (!m_window || !m_item.data) return;
    ItemWindow* win = (__bridge ItemWindow*)m_window;
    [win updatePosition];
}

#endif
