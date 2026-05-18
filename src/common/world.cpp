// ===========================
// world.cpp
// ===========================
#include "world.h"
#include "actor.h"

static constexpr int kLeavesPerPile = 128;
static constexpr int kLeafRandomSpeed = 200;
static constexpr float kLeafSpeedMultiplier = 200.0f;
static constexpr float kLeafSpeedFactor = 0.2f;
static constexpr float kLeafVelZMin = 10.0f;
static constexpr float kLeafVelZRange = 490;
static constexpr float kLeafGravity = -900.0f;

WorldContext g_world;
static const size_t UI_LOG_MAX = 12;

void UiLogPush(const std::string& s) {
    g_world.uiLog.push_back(s);
    if (g_world.uiLog.size() > UI_LOG_MAX) g_world.uiLog.pop_front();
}

void LeafPile::Init(Vector2 position, float radius, float height, double currentTime) {
    pos = position;
    rad = radius;
    timeSinceKicked = -1.0f;
    timeCreated = currentTime;
    leaves.resize(kLeavesPerPile);
    for (int i = 0; i < kLeavesPerPile; i++) {
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
        float randSpeed = (rand() % kLeafRandomSpeed);
        Vector2 val2 = val * randSpeed;
        val2 = val2 + val * num2 * kLeafSpeedMultiplier * kLeafSpeedFactor;
        val2 = val2 + (kickVelocity - val2) * 0.3f;
        float num3 = kLeafVelZMin + (rand() % (int)kLeafVelZRange);
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
            leaves[i].velZ += kLeafGravity * dt;
            if (leaves[i].curPosZ < 0.0f) {
                leaves[i].curPosZ = 0.0f;
                leaves[i].velZ *= -0.3f;
                leaves[i].velPlanar = leaves[i].velPlanar * 0.2f;
            }
        }
    } else if (g && Vector2::Distance(g->pos, pos) < rad + g_config.render.footSize) {
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
#endif

Goose* GetGooseById(int id) {
    for (auto* g : ActorManager::Instance().getGeese()) {
        if (g->id == id) return g;
    }
    return nullptr;
}