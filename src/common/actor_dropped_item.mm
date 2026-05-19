// actor_dropped_item.mm
// DroppedItem actor implementation — owns DroppedItem data, manages window.

#include "actor_dropped_item.h"
#include "actor.h"
#include "item_window.h"
#include "config.h"
#include <cstdio>

DroppedItemActor::DroppedItemActor(const DroppedItem& item)
    : m_item(item)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    position = {item.pos.x, item.pos.y};
    radius = item.data ? std::max(item.data->w, item.data->h) * 0.5f : 0;
    active = true;

    ActorManager::Instance().add(this);

#ifdef __APPLE__
    initWindow();
#endif
}

DroppedItemActor::~DroppedItemActor() {
#ifdef __APPLE__
    if (m_window) {
        ItemWindowManager* manager = [ItemWindowManager shared];
        [manager.windows removeObjectForKey:(__bridge NSNumber*)m_windowKey];
        [(__bridge ItemWindow*)m_window close];
        m_window = nullptr;
        m_windowKey = nullptr;
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

void DroppedItemActor::tick(double dt, double time) {
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
    ItemWindowManager* manager = [ItemWindowManager shared];
    ItemWindow* win = [[ItemWindow alloc] initWithItem:&m_item];
    m_window = (__bridge void*)win;

    static NSInteger s_nextKey = 1000;
    m_windowKey = (__bridge void*)@(s_nextKey++);
    manager.windows[(__bridge NSNumber*)m_windowKey] = win;
}

void DroppedItemActor::updateWindow() {
    if (!m_window || !m_item.data) return;
    ItemWindow* win = (__bridge ItemWindow*)m_window;
    [win updatePosition];
}

#endif
