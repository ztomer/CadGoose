#include "item_drag_controller.h"
#include "config.h"

bool ItemDragController::OnMouseDown(DevicePoint mousePt) {
    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        DevicePoint itemPos = {item.pos.x, item.pos.y};

        if (HitTest::PointInItem(mousePt, itemPos, item.data->w, item.data->h, item.rotation, g_config.general.globalScale)) {
            // Check close button (top-left corner, only for non-toys)
            if (item.data->type != ItemData::TOY) {
                if (HitTest::PointInCloseButton(mousePt, itemPos, item.data->w, item.data->h,
                                                item.rotation, g_config.render.closeButtonSize, g_config.general.globalScale)) {
                    delete item.data;
                    auto forward_it = std::prev(it.base());
                    g_droppedItems.erase(forward_it);
                    return true; // Item deleted, no drag
                }
            }

            // Start drag — compute offset in DEVICE coords
            m_draggedItem = &item;
            m_draggedItem->pinned = true;
            m_dragOffset = {item.pos.x - mousePt.x, item.pos.y - mousePt.y};
            auto forward_it = std::prev(it.base());
            g_droppedItems.splice(g_droppedItems.end(), g_droppedItems, forward_it);
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
