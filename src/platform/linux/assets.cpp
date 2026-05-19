#include "assets.h"
#include "random_util.h"
#include "config.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

const std::string ASSET_ROOT_NAME = "Assets";
fs::path ASSET_ROOT;

ItemData::ItemData() : pixbuf(nullptr), w(0), h(0) {}
ItemData::~ItemData() { if(pixbuf) g_object_unref(pixbuf); }

AssetManager g_assets;

AssetManager::~AssetManager() {
    for (auto& [_, pixbuf] : memeCache) {
        if (pixbuf) g_object_unref(pixbuf);
    }
    memeCache.clear();
    textCache.clear();

    for (Mix_Chunk* chunk : honks) {
        if (chunk) Mix_FreeChunk(chunk);
    }
    for (Mix_Chunk* chunk : pats) {
        if (chunk) Mix_FreeChunk(chunk);
    }
    honks.clear();
    pats.clear();

    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

void AssetManager::Init() {
    // Audio
    if (SDL_Init(SDL_INIT_AUDIO) == 0) {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        LoadAudio(honks, "Sound/NotEmbedded/Honk1.mp3");
        LoadAudio(honks, "Sound/NotEmbedded/Honk2.mp3");
        LoadAudio(pats, "Sound/NotEmbedded/Pat1.mp3");
        LoadAudio(pats, "Sound/NotEmbedded/Pat2.mp3");
    }

    // Memes
    ScanFolder("Images/Memes", memePaths, {".jpg", ".png", ".jpeg"});
    // Text
    ScanFolder("Text/NotepadMessages", textPaths, {".txt"});
}

ItemData* AssetManager::GetRandomMeme(int screenWidth, int screenHeight, float maxSizeFraction) {
    if(memePaths.empty()) return nullptr;
    std::string p = memePaths[rng_util::RandRange((int)memePaths.size())];
    GdkPixbuf* pb = nullptr;

    auto cached = memeCache.find(p);
    if (cached != memeCache.end()) {
        pb = cached->second;
    } else {
        GError* err = nullptr;
        pb = gdk_pixbuf_new_from_file(p.c_str(), &err);
        if (!pb) {
            if (err) g_error_free(err);
            return nullptr;
        }
        
        int maxW = (int)(screenWidth * maxSizeFraction);
        int maxH = (int)(screenHeight * maxSizeFraction);
        int w = gdk_pixbuf_get_width(pb);
        int h = gdk_pixbuf_get_height(pb);
        
        if(w > maxW || h > maxH) {
            float ratioW = (float)maxW / w;
            float ratioH = (float)maxH / h;
            float ratio = std::min(ratioW, ratioH);
            int newW = (int)(w * ratio);
            int newH = (int)(h * ratio);
            GdkPixbuf* scaled = gdk_pixbuf_scale_simple(pb, newW, newH, GDK_INTERP_BILINEAR);
            g_object_unref(pb);
            pb = scaled;
        }

        if (!gdk_pixbuf_get_has_alpha(pb)) {
            GdkPixbuf* withAlpha = gdk_pixbuf_add_alpha(pb, FALSE, 0, 0, 0);
            g_object_unref(pb);
            pb = withAlpha;
        }

        memeCache[p] = pb;
    }

    ItemData* item = new ItemData();
    item->type = ItemData::MEME;
    item->pixbuf = (GdkPixbuf*)g_object_ref(pb);
    item->w = gdk_pixbuf_get_width(pb);
    item->h = gdk_pixbuf_get_height(pb);
    return item;
}

ItemData* AssetManager::GetRandomText() {
    if(textPaths.empty()) return nullptr;
    std::string p = textPaths[rng_util::RandRange((int)textPaths.size())];
    std::shared_ptr<const std::string> text;

    auto cached = textCache.find(p);
    if (cached != textCache.end()) {
        text = cached->second;
    } else {
        std::ifstream f(p);
        std::stringstream buffer;
        buffer << f.rdbuf();
        text = std::make_shared<const std::string>(buffer.str());
        textCache[p] = text;
    }

    ItemData* item = new ItemData();
    item->type = ItemData::TEXT;
    item->textContent = std::move(text);
    item->w = 200; // Fixed width for notepad
    item->h = 150;
    return item;
}

ItemData* AssetManager::CreateTextItem(const std::string& text) {
    ItemData* item = new ItemData();
    item->type = ItemData::TEXT;
    item->textContent = std::make_shared<const std::string>(text);
    item->w = 200;
    item->h = 150;
    return item;
}

ItemData* AssetManager::CreateToyItem(bool isStick) {
    ItemData* item = new ItemData();
    item->type = ItemData::TOY;

    int toyW = isStick ? 32 : 20;
    int toyH = isStick ? 8 : 20;
    item->w = toyW;
    item->h = toyH;

    GdkPixbuf* pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, toyW, toyH);
    if (pb) {
        guchar* pixels = gdk_pixbuf_get_pixels(pb);
        int stride = gdk_pixbuf_get_rowstride(pb);
        int channels = gdk_pixbuf_get_n_channels(pb);

        for (int y = 0; y < toyH; y++) {
            for (int x = 0; x < toyW; x++) {
                guchar* p = pixels + y * stride + x * channels;
                if (isStick) {
                    p[0] = 140; p[1] = 90; p[2] = 38; p[3] = 255;
                } else {
                    p[0] = 204; p[1] = 51; p[2] = 51; p[3] = 255;
                }
            }
        }
        item->pixbuf = pb;
    }

    return item;
}

void AssetManager::Honk() { if(g_config.audioEnabled && !honks.empty()) Mix_PlayChannel(-1, honks[rng_util::RandRange((int)honks.size())], 0); }
void AssetManager::Pat()  { if(g_config.audioEnabled && !pats.empty())  Mix_PlayChannel(-1, pats[rng_util::RandRange((int)pats.size())], 0); }

void* AssetManager::GetBehaviorImage(const std::string& name) {
    fs::path path = ASSET_ROOT / name;
    if (!fs::exists(path)) return nullptr;
    GError* err = nullptr;
    GdkPixbuf* pb = gdk_pixbuf_new_from_file(path.string().c_str(), &err);
    if (!pb) {
        if (err) g_error_free(err);
        return nullptr;
    }
    return pb;
}

void AssetManager::PreloadBehaviorAssets() {
}

void AssetManager::LoadAudio(std::vector<Mix_Chunk*>& v, std::string p) {
    fs::path path = ASSET_ROOT / p;
    if(fs::exists(path)) v.push_back(Mix_LoadWAV(path.string().c_str()));
    // Use absolute path string.
} 

void AssetManager::ScanFolder(std::string rel, std::vector<std::string>& out, std::vector<std::string> exts) {
    fs::path p = ASSET_ROOT / rel;
    if(!fs::exists(p)) return;
    for(const auto& entry : fs::directory_iterator(p)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        for(const auto& e : exts) {
            if(ext == e) {
                out.push_back(entry.path().string());
                break;
            }
        }
    }
}
