// actor_dropped_item.h
// DroppedItem actor — memes, text notes, and toys on the ground.
// Each item has its own window (macOS) or renders via overlay (Linux).

#pragma once

#include "actor.h"
#include "items.h"

class DroppedItemActor : public Actor {
public:
    DroppedItemActor(ItemData* data, const Vector2& pos, float rotation, double timeDropped);
    ~DroppedItemActor() override;

    const char* type() const override { return "dropped_item"; }
    void tick(double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return active && !isExpired(); }

    ItemData* data() const { return m_data; }
    float rotation() const { return m_rotation; }
    double timeDropped() const { return m_timeDropped; }
    bool pinned() const { return m_pinned; }
    void setPinned(bool pinned) { m_pinned = pinned; }

    bool isExpired() const;

private:
    ItemData* m_data;
    float m_rotation;
    double m_timeDropped;
    bool m_pinned;

#ifdef __APPLE__
    void* m_window;  // ItemWindow* as opaque pointer
    void* m_windowKey;  // NSNumber* as opaque pointer
    void initWindow();
    void updateWindow();
#endif
};
