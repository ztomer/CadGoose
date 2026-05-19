#include "item_drag_controller.h"
#include "config.h"
#include "coordinate_system.h"
#include "world.h"
#include "actor.h"
#include "actor_dropped_item.h"

bool ItemDragController::OnMouseDown(DevicePoint mousePt) {
    auto items = ActorManager::Instance().getDroppedItems();
    // Iterate in reverse for z-order (last added = on top)
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        DroppedItemActor* actor = *it;
        DroppedItem& item = actor->item();
        DevicePoint itemCenter = ItemCoords::Center({item.pos.x, item.pos.y}, item.data->w, item.data->h, g_config.general.globalScale);

        if (HitTest::PointInItem(mousePt, itemCenter, item.data->w, item.data->h, item.rotation, g_config.general.globalScale)) {
            if (item.data->type != ItemData::TOY) {
                if (HitTest::PointInCloseButton(mousePt, itemCenter, item.data->w, item.data->h,
                                                item.rotation, g_config.render.closeButtonSize, g_config.general.globalScale)) {
                    ActorManager::Instance().remove(actor);
                    return true;
                }
            }

            m_draggedItem = &item;
            m_draggedItem->pinned = true;
            m_dragOffset = {item.pos.x - mousePt.x, item.pos.y - mousePt.y};
            // With per-item windows, z-order is controlled by window level, not list order
            return true;
        }
    }
    return false;
}

void ItemDragController::OnMouseDragged(DevicePoint mousePt) {
    if (m_draggedItem) {
        m_draggedItem->pos.x = mousePt.x + m_dragOffset.x;
        m_draggedItem->pos.y = mousePt.y + m_dragOffset.y;
    }
}

void ItemDragController::OnMouseUp() {
    m_draggedItem = nullptr;
}
