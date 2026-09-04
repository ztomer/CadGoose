// linux_stubs.cpp
// Minimal Linux implementations for symbols only defined in macOS .mm files.

#include <gtk/gtk.h>
#include "actor_breadcrumb.h"
#include "actor_dropped_item.h"
#include "actor_flower.h"
#include "actor_jail.h"
#include "actor_toy.h"
#include "ai_text_meme.h"
#include "app_actions.h"
#include "assets.h"
#include "config.h"
#include "goose.h"
#include "items.h"
#include "platform_input.h"
#include "renderer_interface.h"
#include "world.h"
#include <cairo.h>
#include <cstdio>
#include <mutex>
#include <queue>
#include <string>

// ---------------------------------------------------------------------------
// Actor constructors/destructors
// ---------------------------------------------------------------------------

BreadcrumbActor::BreadcrumbActor(const Vector2& pos, double spawnTime, float lifetime)
    : Actor(), m_spawnTime(spawnTime), m_lifetime(lifetime), m_image(nullptr) {
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = 5.0f;
}

BreadcrumbActor::~BreadcrumbActor() {}

void BreadcrumbActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx; (void)dt;
    if (!m_active) return;
    if (time - m_spawnTime > m_lifetime) m_active = false;
}

void BreadcrumbActor::render(IRenderer* renderer) { (void)renderer; }

FlowerActor::FlowerActor(const Vector2& pos, float hue, double spawnTime)
    : Actor(), m_hue(hue), m_growth(0), m_stemHeight(0), m_petalSize(0), m_spawnTime(spawnTime) {
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = 25.0f;
}

FlowerActor::~FlowerActor() {}

void FlowerActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx; (void)dt; (void)time;
    if (!m_active) return;
    double age = time - m_spawnTime;
    if (age > 60.0) { m_active = false; return; }
    m_growth = 1.0f;
    m_stemHeight = 20.0f;
    m_petalSize = 10.0f;
}

void FlowerActor::render(IRenderer* renderer) { (void)renderer; }

JailActor::JailActor(const Vector2& pos) : Actor() {
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = 30.0f;
}

JailActor::~JailActor() {}

void JailActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx; (void)dt; (void)time;
}

void JailActor::render(IRenderer* renderer) { (void)renderer; }

ToyActor::ToyActor(Type type, const Vector2& pos, int instanceId)
    : Actor(), m_type(type), m_angle(0), m_spawnTime(0), m_instanceId(instanceId) {
    m_position = {pos.x, pos.y};
    m_active = true;
    m_spawnTime = g_time;
    m_angle = 0;
    m_radius = (type == Stick) ? 12.0f : 15.0f;
}

ToyActor::~ToyActor() {}

void ToyActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx; (void)dt;
    if (!m_active) return;
    if (time - m_spawnTime > 60.0) m_active = false;
}

void ToyActor::render(IRenderer* renderer) { (void)renderer; }

DroppedItemActor::DroppedItemActor(const DroppedItem& item)
    : m_item(item) {
    m_position = {item.pos.x, item.pos.y};
    m_radius = item.data ? std::max(item.data->w, item.data->h) * 0.5f : 0;
    m_active = true;
    ActorManager::Instance().add(this);
}

DroppedItemActor::~DroppedItemActor() {
    if (m_item.data) {
        delete m_item.data;
        m_item.data = nullptr;
    }
}

bool DroppedItemActor::isExpired() const {
    if (!m_item.data || m_item.pinned) return false;
    double elapsed = g_time - m_item.timeDropped;
    return elapsed > g_config.item.itemLifetime;
}

void DroppedItemActor::tick(WorldContext& ctx, double dt, double time) {
    (void)ctx; (void)dt; (void)time;
}

void DroppedItemActor::render(IRenderer* renderer) { (void)renderer; }

// ---------------------------------------------------------------------------
// AssetManager
// ---------------------------------------------------------------------------

void AssetManager::Bite() {
    Honk();
}

ItemData* AssetManager::CreateTestImage(int w, int h) {
    ItemData* item = new ItemData();
    item->type = ItemData::MEME;
    item->w = w;
    item->h = h;
    return item;
}

// ---------------------------------------------------------------------------
// AI_TextMeme (pure C++ logic; the .mm is entirely inside __APPLE__ guard)
// ---------------------------------------------------------------------------

static std::queue<std::string> s_fileQueue;

void AI_TextMeme_LoadFileTexts() {}
void AI_TextMeme_Tick(double time) { (void)time; }
bool AI_TextMeme_HasAvailable() { return !s_fileQueue.empty(); }

std::string AI_TextMeme_Dequeue() {
    if (s_fileQueue.empty()) return "";
    std::string result = s_fileQueue.front();
    s_fileQueue.pop();
    return result;
}

int AI_TextMeme_QueueSize() { return (int)s_fileQueue.size(); }
void AI_TextMeme_Reset() { while (!s_fileQueue.empty()) s_fileQueue.pop(); }
void AI_TextMeme_Inject(const std::string& text) { s_fileQueue.push(text); }

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

bool Config_IsSystemDarkTheme() { return false; }

// ---------------------------------------------------------------------------
// Presence (extern "C" — must match the declaration in behavior_presence.cpp)
// ---------------------------------------------------------------------------

extern "C" void Presence_UpdateStatusFromBehavior(const char* status) {
    (void)status;
}

extern "C" void Presence_SetGooseWindowVisible(bool visible) {
    (void)visible;
}

// ---------------------------------------------------------------------------
// AppActions
// ---------------------------------------------------------------------------

void AppActions_SetApplication(GtkApplication* app) { (void)app; }

// ---------------------------------------------------------------------------
// UI utilities
// ---------------------------------------------------------------------------

void UiLogTrim() {}

const char* GetGooseStateStr(GooseState s) {
    switch (s) {
        case GooseState::WANDER:       return "WANDER";
        case GooseState::FETCHING:     return "FETCHING";
        case GooseState::RETURNING:    return "RETURNING";
        case GooseState::CHASE_CURSOR: return "CHASE_CURSOR";
        case GooseState::SNATCH_CURSOR: return "SNATCH_CURSOR";
        default:                       return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Goose Cairo drawing
// ---------------------------------------------------------------------------

void Goose::Draw(cairo_t* cr) {
    if (!cr) return;
    cairo_save(cr);
    cairo_translate(cr, pos.x, pos.y);
    cairo_scale(cr, g_config.general.globalScale, g_config.general.globalScale);
    cairo_translate(cr, -pos.x, -pos.y);

    cairo_arc(cr, pos.x, pos.y, 20.0, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.9);
    cairo_fill(cr);

    DrawEyes(cr, {1, 0});
    cairo_restore(cr);
}

void Goose::DrawHeldItem(cairo_t* cr) { (void)cr; }

void Goose::DrawEyes(cairo_t* cr, Vector2 fwd) {
    (void)fwd;
    cairo_save(cr);
    float cy = pos.y - 5;
    cairo_arc(cr, pos.x - 6, cy, 3, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_fill(cr);
    cairo_arc(cr, pos.x + 6, cy, 3, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_fill(cr);
    cairo_restore(cr);
}

void Goose::DrawEllipse(cairo_t* cr, Vector2 p, int rx, int ry, float r, float g, float b, float a) {
    cairo_save(cr);
    cairo_translate(cr, p.x, p.y);
    cairo_scale(cr, (double)rx / ry, 1.0);
    cairo_arc(cr, 0, 0, ry, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_fill(cr);
    cairo_restore(cr);
}

void Goose::DrawLine(cairo_t* cr, Vector2 a, Vector2 b, float w, const float color[]) {
    cairo_save(cr);
    cairo_move_to(cr, a.x, a.y);
    cairo_line_to(cr, b.x, b.y);
    cairo_set_line_width(cr, w);
    cairo_set_source_rgba(cr, color[0], color[1], color[2], color[3]);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void Goose::DrawLine(cairo_t* cr, Vector2 a, Vector2 b, float w, float r, float g, float bl) {
    float color[] = {r, g, bl, 1.0f};
    DrawLine(cr, a, b, w, color);
}

// ---------------------------------------------------------------------------
// Platform input stubs (overridden by test mocks on macOS)
// ---------------------------------------------------------------------------

bool Platform_IsKeyPressed(int keyCode) {
    (void)keyCode;
    return false;
}

bool Platform_IsMouseButtonDown(int button) {
    (void)button;
    return false;
}

bool Platform_GetMousePosition(double* outX, double* outY) {
    (void)outX;
    (void)outY;
    return false;
}
