#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <list>
#include "items.h"

#ifdef __linux__
#include <gtk/gtk.h>
#include <SDL_mixer.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace fs = std::filesystem;

extern const std::string ASSET_ROOT_NAME;
extern fs::path ASSET_ROOT;


class AssetManager {
public:
#ifdef __linux__
    std::vector<Mix_Chunk*> honks, pats;
    std::vector<std::string> memePaths;
    std::vector<std::string> textPaths;
    std::unordered_map<std::string, GdkPixbuf*> memeCache;
#elif defined(__APPLE__)
    std::vector<std::string> honkFiles;
    std::vector<std::string> memePaths;
    std::vector<std::string> textPaths;
    std::unordered_map<std::string, CGImageRef> memeCache;
#endif
    std::unordered_map<std::string, std::shared_ptr<const std::string>> textCache;

    void Init();
    ~AssetManager();
    ItemData* GetRandomMeme(int screenWidth = 1920, int screenHeight = 1080, float maxSizeFraction = 0.1f);
    ItemData* GetRandomText();
    ItemData* CreateTextItem(const std::string& text);
    ItemData* CreateToyItem(bool isStick);
    void Honk();
    void Pat();
    void Bite();
    void MudSquish();

#ifdef __APPLE__
    CGImageRef GetBehaviorImage(const std::string& name);
    void PreloadBehaviorAssets();
#elif defined(__linux__)
    void* GetBehaviorImage(const std::string& name); // returns GdkPixbuf*
    void PreloadBehaviorAssets();
#endif

private:
#ifdef __linux__
    void LoadAudio(std::vector<Mix_Chunk*>& v, std::string p);
#endif
    void ScanFolder(std::string rel, std::vector<std::string>& out, std::vector<std::string> exts);
};

extern AssetManager g_assets;
