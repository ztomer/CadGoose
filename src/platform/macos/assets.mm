#include "assets.h"
#include "items.h"
#include "audio.h"
#include "config.h"
#include "world.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(__APPLE__)
#import <AppKit/AppKit.h>
#endif

namespace fs = std::filesystem;

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
    std::string bundlePath = GetBundlePath();
    if (!bundlePath.empty()) {
        ASSET_ROOT = bundlePath;
    } else {
        ASSET_ROOT = ".";
    }
#else
    ASSET_ROOT = ".";
#endif

    // If Assets folder doesn't exist in ASSET_ROOT, search parent dirs relative to ASSET_ROOT
    fs::path assetsPath = ASSET_ROOT / "Assets";
    if (!fs::exists(assetsPath)) {
        fs::path parentPath = ASSET_ROOT / ".." / "Assets";
        if (fs::exists(parentPath)) {
            ASSET_ROOT = ASSET_ROOT.parent_path();
        } else {
            fs::path grandparentPath = ASSET_ROOT / ".." / ".." / "Assets";
            if (fs::exists(grandparentPath)) {
                ASSET_ROOT = ASSET_ROOT.parent_path().parent_path();
            }
        }
    }

    std::cout << "Asset root: " << ASSET_ROOT << std::endl;

#if defined(__APPLE__)
    std::string fontPath = ASSET_ROOT / "Assets/Text/MapleMono";
    NSURL *fontDirURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:fontPath.c_str()] isDirectory:YES];
    CTFontManagerRegisterFontsForURL((__bridge CFURLRef)fontDirURL, kCTFontManagerScopeProcess, NULL);
#endif

    ScanFolder("Assets/Images/Memes", memePaths, {".png", ".jpg", ".jpeg"});
    ScanFolder("Assets/Text/NotepadMessages", textPaths, {".txt"});

    std::cout << "Assets: " << memePaths.size() << " memes, " << textPaths.size() << " texts" << std::endl;

    PreloadBehaviorAssets();
}

void AssetManager::PreloadBehaviorAssets() {
    std::vector<std::string> behaviorImages = {
        "Assets/Images/OtherGfx/ball.png",
        "Assets/Images/OtherGfx/ball2.png",
        "Assets/Images/OtherGfx/ball3.png",
        "Assets/Images/OtherGfx/p1.png",
        "Assets/Images/OtherGfx/p2.png",
        "Assets/Images/OtherGfx/crumbs.png",
        "Assets/Images/OtherGfx/heart.png",
        "Assets/Images/OtherGfx/hat_default.png",
        "Assets/Images/OtherGfx/honk.png",
        "Assets/Items/Bed/bed.png",
        "Assets/Items/Bed/z1.png",
        "Assets/Items/Bed/z2.png",
        "Assets/Items/Bed/z3.png"
    };

    for (const auto& path : behaviorImages) {
        if (memeCache.find(path) != memeCache.end()) continue;
        NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
        NSImage* img = [[NSImage alloc] initWithContentsOfFile:nsPath];
        if (img) {
            CGImageRef cg = [img CGImageForProposedRect:NULL context:nil hints:nil];
            if (cg) {
                memeCache[path] = CGImageRetain(cg);
            }
        }
    }
}

CGImageRef AssetManager::GetBehaviorImage(const std::string& name) {
    auto it = memeCache.find(name);
    if (it != memeCache.end()) {
        return it->second;
    }

    NSString* nsPath = [NSString stringWithUTF8String:name.c_str()];
    NSImage* img = [[NSImage alloc] initWithContentsOfFile:nsPath];
    if (img) {
        CGImageRef cg = [img CGImageForProposedRect:NULL context:nil hints:nil];
        if (cg) {
            CGImageRef retained = CGImageRetain(cg);
            memeCache[name] = retained;
            return retained;
        }
    }
    return nullptr;
}

void AssetManager::ScanFolder(std::string rel, std::vector<std::string>& out, std::vector<std::string> exts) {
    fs::path scanPath = ASSET_ROOT / rel;
    if (!fs::exists(scanPath)) return;

    for (const auto& entry : fs::directory_iterator(scanPath)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for (const auto& allowed : exts) {
            if (ext == allowed) {
                out.push_back(entry.path().string());
                break;
            }
        }
    }
}

ItemData* AssetManager::GetRandomMeme(int screenWidth, int screenHeight, float maxSizeFraction) {
    if (memePaths.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)memePaths.size() - 1);
    std::string path = memePaths[dis(gen)];

    ItemData* data = new ItemData();
    data->type = ItemData::MEME;

    int maxW = (int)(screenWidth * maxSizeFraction);
    int maxH = (int)(screenHeight * maxSizeFraction);

    auto it = memeCache.find(path);
    if (it != memeCache.end()) {
        data->image = CGImageRetain(it->second);
        data->w = CGImageGetWidth(data->image);
        data->h = CGImageGetHeight(data->image);
        if (data->w > maxW || data->h > maxH) {
            float scaleW = (float)maxW / data->w;
            float scaleH = (float)maxH / data->h;
            float scale = std::min(scaleW, scaleH);
            data->w = (int)(data->w * scale);
            data->h = (int)(data->h * scale);
        }
        return data;
    }

    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSImage* img = [[NSImage alloc] initWithContentsOfFile:nsPath];
    if (img) {
        NSSize size = img.size;
        int origW = (int)size.width;
        int origH = (int)size.height;
        
        int finalW = origW;
        int finalH = origH;
        
        if (origW > maxW || origH > maxH) {
            float scaleW = (float)maxW / origW;
            float scaleH = (float)maxH / origH;
            float scale = std::min(scaleW, scaleH);
            finalW = (int)(origW * scale);
            finalH = (int)(origH * scale);
            
            NSImage* resized = [[NSImage alloc] initWithSize:NSMakeSize(finalW, finalH)];
            [resized lockFocus];
            [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
            [img drawInRect:NSMakeRect(0, 0, finalW, finalH) fromRect:NSMakeRect(0, 0, origW, origH) operation:NSCompositeSourceOver fraction:1.0];
            [resized unlockFocus];
            img = resized;
        }
        
        data->w = finalW;
        data->h = finalH;
        CGImageRef cgImage = [img CGImageForProposedRect:NULL context:nil hints:nil];
        if (cgImage) {
            data->image = CGImageRetain(cgImage);
            memeCache[path] = CGImageRetain(cgImage);
        } else {
            data->w = g_config.asset.memePlaceholderW;
            data->h = g_config.asset.memePlaceholderH;
        }
    } else {
        data->w = g_config.asset.memePlaceholderW;
        data->h = g_config.asset.memePlaceholderH;
    }

    return data;
}
ItemData* AssetManager::GetRandomText() {
    if (textPaths.empty()) return nullptr;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, (int)textPaths.size() - 1);
    std::string path = textPaths[dis(gen)];

    ItemData* data = new ItemData();
    data->type = ItemData::TEXT;
    data->w = g_config.asset.textPlaceholderW;
    data->h = g_config.asset.textPlaceholderH;
    
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        data->textContent = std::make_shared<std::string>(buffer.str());
    } else {
        data->textContent = std::make_shared<std::string>("Note");
    }
    
    return data;
}

ItemData* AssetManager::CreateTextItem(const std::string& text) {
    ItemData* data = new ItemData();
    data->type = ItemData::TEXT;
    data->w = g_config.asset.textPlaceholderW;
    data->h = g_config.asset.textPlaceholderH;
    data->textContent = std::make_shared<std::string>(text);
    data->isAIGenerated = true;
    return data;
}

void AssetManager::Honk() {
    Audio_PlayHonk();
}

void AssetManager::Pat() {
    Audio_PlayPat();
}

void AssetManager::Bite() {
    Audio_PlayBite();
}

void AssetManager::MudSquish() {
    Audio_PlayMudSquish();
}

AssetManager g_assets;
