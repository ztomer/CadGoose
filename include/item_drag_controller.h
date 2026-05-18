#pragma once

#include "coordinate_system.h"
#include "items.h"
#include <list>



// ItemDragController handles hit-testing and drag operations for dropped items.
// All coordinates are in DEVICE space (screen pixels, top-left origin, Y-down).
// Used by both AppKit responder chain and NSEvent monitor handlers.
class ItemDragController {
public:
    // Returns true if an item was hit and drag started
    bool OnMouseDown(DevicePoint mousePt);

    // Updates dragged item position. No-op if not dragging.
    void OnMouseDragged(DevicePoint mousePt);

    // Ends drag. No-op if not dragging.
    void OnMouseUp();

    // Returns the item currently being dragged, or nullptr
    DroppedItem* GetDraggedItem() const { return m_draggedItem; }

private:
    DroppedItem* m_draggedItem = nullptr;
    DevicePoint m_dragOffset{0, 0};
};
