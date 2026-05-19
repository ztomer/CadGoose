// actor_dropped_item.h
// DroppedItem actor — memes, text notes, and toys on the ground.
// Each item has its own window (macOS) or renders via overlay (Linux).
// Owns the DroppedItem data — managed exclusively by ActorManager.

#pragma once

#include "actor.h"
#include "items.h"

class DroppedItemActor : public Actor {
public:
    explicit DroppedItemActor(const DroppedItem& item);
    ~DroppedItemActor() override;

    const char* type() const override { return "dropped_item"; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return m_active && !isExpired(); }

    DroppedItem& item() { return m_item; }
    const DroppedItem& item() const { return m_item; }
    ItemData* data() const { return m_item.data; }
    float rotation() const { return m_item.rotation; }
    double timeDropped() const { return m_item.timeDropped; }
    bool pinned() const { return m_item.pinned; }
    void setPinned(bool pinned) { m_item.pinned = pinned; }

    bool isExpired() const;

private:
    DroppedItem m_item;

#ifdef __APPLE__
    void* m_window;  // ItemWindow* as opaque pointer
    void* m_windowKey;  // NSNumber* as opaque pointer
    void initWindow();
    void updateWindow();
#endif
};
