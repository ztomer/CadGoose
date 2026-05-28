#include "items.h"

ItemData::ItemData() : type(MEME), w(0), h(0) {}

ItemData::~ItemData() {
#if defined(__APPLE__)
    if (image) {
        CGImageRelease(image);
        image = nullptr;
    }
#elif defined(__linux__)
    if (pixbuf) {
        g_object_unref(pixbuf);
        pixbuf = nullptr;
    }
#endif
}
