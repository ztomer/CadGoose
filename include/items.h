#ifndef ITEMS_H
#define ITEMS_H

#include <memory>
#include <string>

#ifdef __linux__
#include <gdk-pixbuf/gdk-pixbuf.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#endif

#include "goose_math.h"
#include "config.h"

struct ItemData {
    enum Type { MEME, TEXT } type;

#ifdef __linux__
    GdkPixbuf* pixbuf = nullptr;
#elif defined(__APPLE__)
    CGImageRef image = nullptr;
#endif

    std::shared_ptr<const std::string> textContent;
    int w = 0, h = 0;
    bool isAIGenerated = false;

    ItemData();
    ~ItemData();
    const std::string& Text() const {
        static const std::string empty;
        return textContent ? *textContent : empty;
    }
};

struct DroppedItem {
    ItemData* data;
    Vector2 pos;
    float rotation;
    double timeDropped;
    bool pinned = false;
    bool isExpired(double time) { return !pinned && ((time - timeDropped) > g_config.item.itemLifetime); } // Disappear after itemLifetime unless pinned
};

#endif // ITEMS_H
