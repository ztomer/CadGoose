#include "assets.h"
#include <vector>
#include <string>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

std::vector<AssetImage> g_memeAssets;
std::vector<std::string> g_noteAssets;

#if defined(__APPLE__)
static std::string GetBundlePath() {
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle) return "";
    CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
    if (!url) return "";
    char path[1024];
    if (CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, sizeof(path))) {
        CFRelease(url);
        return std::string(path);
    }
    CFRelease(url);
    return "";
}
#endif

void Assets_Init() {
#if defined(__APPLE__)
    std::string basePath = GetBundlePath();
    if (!basePath.empty()) {
        // Load from bundle Resources
    }
#endif
}

AssetImage* Assets_LoadImage(const char* name) {
    return nullptr;
}

const char* Assets_GetRandomHonkSound() {
    return "honk.wav";
}