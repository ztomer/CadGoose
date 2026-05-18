#include "item_drag_controller.h"
#include "config.h"
#include "coordinate_system.h"
#include "world.h"

bool ItemDragController::OnMouseDown(DevicePoint mousePt) {
    for (auto it = g_world.droppedItems.rbegin(); it != g_world.droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        DevicePoint itemCenter = ItemCoords::Center({item.pos.x, item.pos.y}, item.data->w, item.data->h, g_config.general.globalScale);

        if (HitTest::PointInItem(mousePt, itemCenter, item.data->w, item.data->h, item.rotation, g_config.general.globalScale)) {
            if (item.data->type != ItemData::TOY) {
                if (HitTest::PointInCloseButton(mousePt, itemCenter, item.data->w, item.data->h,
                                                item.rotation, g_config.render.closeButtonSize, g_config.general.globalScale)) {
                    delete item.data;
                    auto forward_it = std::prev(it.base());
                    g_world.droppedItems.erase(forward_it);
                    return true;
                }
            }

            m_draggedItem = &item;
            m_draggedItem->pinned = true;
            m_dragOffset = {item.pos.x - mousePt.x, item.pos.y - mousePt.y};
            auto forward_it = std::prev(it.base());
            g_world.droppedItems.splice(g_world.droppedItems.end(), g_world.droppedItems, forward_it);
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
