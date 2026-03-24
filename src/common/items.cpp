#include "items.h"

ItemData::ItemData() : type(MEME), w(0), h(0) {}

ItemData::~ItemData() {
#ifdef __APPLE__
    if (image) {
        CGImageRelease(image);
        image = nullptr;
    }
#endif
}