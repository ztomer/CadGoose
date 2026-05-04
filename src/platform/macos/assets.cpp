#include "assets.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

void Assets_Init() {
#if defined(__APPLE__)
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
        // Load from bundle Resources
    }
#endif
}

ItemData* Assets_LoadImage(const char* name) {
    return nullptr;
}

const char* Assets_GetRandomHonkSound() {
    return "honk.wav";
}