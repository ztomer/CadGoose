// actor_dropped_item.mm
// DroppedItem actor — memes, text notes, and toys on the ground.
// Items are passive entities — windows are managed by ItemWindowManager::syncWindows.

#include "actor_dropped_item.h"
#include "config.h"
#include <cstdio>

DroppedItemActor::DroppedItemActor(ItemData* data, const Vector2& pos, float rotation, double timeDropped)
    : m_data(data), m_rotation(rotation), m_timeDropped(timeDropped), m_pinned(false)
{
    position = {pos.x, pos.y};
    radius = data ? std::max(data->w, data->h) * 0.5f : 0;
    active = true;
}

DroppedItemActor::~DroppedItemActor() {
    if (m_data) {
        delete m_data;
        m_data = nullptr;
    }
}

bool DroppedItemActor::isExpired() const {
    if (!m_data || m_pinned) return false;
    double elapsed = g_time - m_timeDropped;
    return elapsed > g_config.item.itemLifetime;
}

void DroppedItemActor::tick(double dt, double time) {
    // Items are passive — no tick logic needed
    (void)dt;
    (void)time;
}

void DroppedItemActor::render(IRenderer* renderer) {
    // Items render via ItemWindowManager on macOS, overlay canvas on Linux
    (void)renderer;
}
