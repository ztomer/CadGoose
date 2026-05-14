// ===========================
// world.cpp
// ===========================
#include "world.h"

std::deque<std::string> g_uiLog;
static const size_t UI_LOG_MAX = 12;

void UiLogPush(const std::string& s) {
    g_uiLog.push_back(s);
    if (g_uiLog.size() > UI_LOG_MAX) g_uiLog.pop_front();
}

std::list<Goose> g_geese;
std::list<MonitorInfo> g_monitors;
std::list<DroppedItem> g_droppedItems;
RingBuffer<Footprint, kMaxFootprints> g_footprints;
std::list<LeafPile> g_leafPiles;
int g_nextId = 0;
int g_screenWidth = 0;
int g_screenHeight = 0;
int g_selectedGooseId = 0;
int g_cursorGrabberId = -1;
int g_frameId = 0;

void LeafPile::Init(Vector2 position, float radius, float height, double currentTime) {
    pos = position;
    rad = radius;
    timeSinceKicked = -1.0f;
    timeCreated = currentTime;
    leaves.resize(128);
    for (int i = 0; i < 128; i++) {
        float angle = (rand() % 360) * M_PI / 180.0f;
        float r = ((rand() % 1000) / 1000.0f);
        Vector2 val = Vector2{r * cosf(angle), r * sinf(angle)};
        float num = (rand() % 1000) / 1000.0f;
        num *= num;
        leaves[i].curPosZ = num * height;
        leaves[i].curPosPlanar = val * radius * (1.0f - num);
        leaves[i].curPosPlanar.y *= 0.6f;
        leaves[i].velPlanar = Vector2{0.0f, 0.0f};
        leaves[i].velZ = 0.0f;
        leaves[i].colorIndex = rand() % 4;
    }
}

void LeafPile::Kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage) {
    timeSinceKicked = currentTime;
    float num = Lerp(0.6f, 1.1f, gooseSpeedPercentage);
    for (int i = 0; i < leaves.size(); i++) {
        Vector2 val = Vector2::Normalize(leaves[i].curPosPlanar);
        float dot = Dot(val, Vector2::Normalize(kickVelocity));
        float num2 = 1.0f - std::abs(dot);
        float randSpeed = (rand() % 200);
        Vector2 val2 = val * randSpeed;
        val2 = val2 + val * num2 * 200.0f * 0.2f;
        val2 = val2 + (kickVelocity - val2) * 0.3f;
        float num3 = 10.0f + (rand() % 490);
        num3 *= Lerp(0.9f, 1.1f, num2);
        leaves[i].velPlanar = val2 * num;
        leaves[i].velZ = num3 * num;
    }
}

void LeafPile::Tick(Goose* g, double currentTime, float dt) {
    if (timeSinceKicked > 0.0f) {
        for (int i = 0; i < leaves.size(); i++) {
            leaves[i].curPosPlanar = leaves[i].curPosPlanar + leaves[i].velPlanar * dt;
            leaves[i].curPosZ += leaves[i].velZ * dt;
            leaves[i].velZ += -900.0f * dt;
            if (leaves[i].curPosZ < 0.0f) {
                leaves[i].curPosZ = 0.0f;
                leaves[i].velZ *= -0.3f;
                leaves[i].velPlanar = leaves[i].velPlanar * 0.2f;
            }
        }
    } else if (g && Vector2::Distance(g->pos, pos) < rad + 4.0f) {
        float walkSpeed = g_config.movement.baseWalkSpeed;
        float chargeSpeed = g_config.movement.baseRunSpeed;
        float currentSpeed = g->currentSpeed;
        float gooseSpeedPercentage = (currentSpeed - walkSpeed) / (chargeSpeed - walkSpeed);
        if (gooseSpeedPercentage < 0.0f) gooseSpeedPercentage = 0.0f;
        if (gooseSpeedPercentage > 1.0f) gooseSpeedPercentage = 1.0f;
        Kick(g->vel, currentTime, gooseSpeedPercentage);
    }
}

#ifdef __linux__
#include <gtk/gtk.h>
GtkWidget* g_entryNote = nullptr;
std::vector<GtkWidget*> g_overlayCanvases;
#endif

Goose* GetGooseById(int id) {
    for (auto& g : g_geese) {
        if (g.id == id) return &g;
    }
    return nullptr;
}