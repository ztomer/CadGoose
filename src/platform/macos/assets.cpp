#include "assets.h"
#include "items.h"
#include <iostream>
#include <random>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

const std::string ASSET_ROOT_NAME = "Assets";
fs::path ASSET_ROOT;

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

AssetManager::~AssetManager() {
    for (auto& pair : memeCache) {
        CGImageRelease(pair.second);
    }
}

void AssetManager::Init() {
#if defined(__APPLE__)
    std::string basePath = GetBundlePath();
    if (!basePath.empty()) {
        ASSET_ROOT = basePath;
    } else {
        ASSET_ROOT = ".";
    }
#endif
    
    ScanFolder("Images/Memes", memePaths, {".png", ".jpg", ".jpeg"});
    ScanFolder("Text/NotepadMessages", textPaths, {".txt"});
    
    std::cout << "Assets: " << memePaths.size() << " memes, " << textPaths.size() << " texts" << std::endl;
}

void AssetManager::ScanFolder(std::string rel, std::vector<std::string>& out, std::vector<std::string> exts) {
    (void)rel; (void)out; (void)exts;
}

ItemData* AssetManager::GetRandomMeme() {
    if (memePaths.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)memePaths.size() - 1);
    
    ItemData* data = new ItemData();
    data->type = ItemData::MEME;
    data->w = 64;
    data->h = 64;
    return data;
}

ItemData* AssetManager::GetRandomText() {
    if (textPaths.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)textPaths.size() - 1);
    
    ItemData* data = new ItemData();
    data->type = ItemData::TEXT;
    data->w = 100;
    data->h = 60;
    data->textContent = std::make_shared<std::string>("Sample text");
    return data;
}

ItemData* AssetManager::CreateTextItem(const std::string& text) {
    ItemData* data = new ItemData();
    data->type = ItemData::TEXT;
    data->w = 100;
    data->h = 60;
    data->textContent = std::make_shared<std::string>(text);
    return data;
}

void AssetManager::Honk() {
    // Placeholder
}

void AssetManager::Pat() {
    // Placeholder
}

AssetManager g_assets;