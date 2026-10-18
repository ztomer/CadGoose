// LeafPileActor's own behavior: lifetime and fade, a kick that scatters the
// leaves and lets them settle, the drawing that the pile's window shows, and
// the window it keeps.
//
// Before G0.13 none of this had a test of its own. It was "covered" by piles
// AutumnLeaves.InitialSpawnOnEnable leaked into the actor manager, which later
// tests then ticked and rendered without asserting anything about them; once
// every test started from an empty manager, 116 of the file's lines dropped
// out of the coverage measure (ROADMAP G0.16).
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#import <Cocoa/Cocoa.h>

#include "actor.h"
#include "actor_leafpile.h"
#include "behavior_element_window.h"
#include "config.h"
#include "goose.h"
#include "renderer_interface.h"
#include "world.h"
#include "world_utils.h"

void GlobalState_DrainMainRunLoop();

namespace {

struct Ellipse {
    RenderPoint center;
    RenderColor color;
};

class RecordingRenderer : public IRenderer {
public:
    std::vector<Ellipse> ellipses;
    float tx = 0.0f, ty = 0.0f;
    int saves = 0, restores = 0;

    void SaveState() override { ++saves; }
    void RestoreState() override { ++restores; }
    void Translate(float x, float y) override { tx += x; ty += y; }
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint c, float, float, RenderColor fill) override {
        ellipses.push_back({c, fill});
    }
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawImage(void*, RenderRect) override {}
    bool GetImageSize(void*, float*, float*) override { return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
};

constexpr float kRadius = 60.0f;
constexpr float kHeight = 100.0f;
constexpr int kLeaves = 128;

// Every leaf the pile draws lands inside the window it sized for them.
void ExpectEveryLeafInsideItsWindow(const LeafPileActor& pile) {
    RecordingRenderer r;
    pile.drawLeaves(r);
    const float win = pile.leafBounds().windowSize();
    ASSERT_EQ(r.ellipses.size(), static_cast<size_t>(kLeaves));
    EXPECT_FLOAT_EQ(r.tx, win * 0.5f);
    EXPECT_FLOAT_EQ(r.ty, win * 0.5f);
    for (const auto& e : r.ellipses) {
        const float x = e.center.x + r.tx;
        const float y = e.center.y + r.ty;
        EXPECT_GE(x, 0.0f);
        EXPECT_LE(x, win);
        EXPECT_GE(y, 0.0f);
        EXPECT_LE(y, win);
    }
}

// How far apart the outermost leaves are drawn, horizontally.
float DrawnSpreadX(const LeafPileActor& pile) {
    RecordingRenderer r;
    pile.drawLeaves(r);
    float lo = 1e9f, hi = -1e9f;
    for (const auto& e : r.ellipses) {
        lo = std::min(lo, e.center.x);
        hi = std::max(hi, e.center.x);
    }
    return hi - lo;
}

int LeafWindowCount() {
    int n = 0;
    for (NSWindow* w in [NSApplication sharedApplication].windows) {
        if ([w isKindOfClass:[BehaviorElementWindow class]] && w.isVisible) ++n;
    }
    return n;
}

TEST(LeafPileActor, DrawsEveryLeafOnceWithinItsWindow) {
    LeafPileActor pile({500, 400}, kRadius, kHeight, 0.0);
    RecordingRenderer r;
    pile.drawLeaves(r);

    EXPECT_EQ(r.ellipses.size(), static_cast<size_t>(kLeaves));
    EXPECT_EQ(r.saves, 1);
    EXPECT_EQ(r.restores, 1);
    for (const auto& e : r.ellipses) {
        // Higher leaves are fainter, never below the floor, never above opaque.
        EXPECT_GE(e.color.a, 0.2f);
        EXPECT_LE(e.color.a, 1.0f);
    }
    ExpectEveryLeafInsideItsWindow(pile);
}

TEST(LeafPileActor, WindowAndDrawingAgreeAtEveryScale) {
    // The window's draw block once captured globalScale at creation while
    // render() re-read it every frame, so a scale change drew leaves for one
    // scale into a window sized for another.
    LeafPileActor pile({500, 400}, kRadius, kHeight, 0.0);
    g_config.general.globalScale = 1.0f;
    const float small = pile.leafBounds().windowSize();
    const float spread1 = DrawnSpreadX(pile);
    ExpectEveryLeafInsideItsWindow(pile);

    g_config.general.globalScale = 2.0f;
    const float large = pile.leafBounds().windowSize();
    ExpectEveryLeafInsideItsWindow(pile);
    EXPECT_GT(large, small * 1.5f);
    // ...and the leaves themselves are drawn at the new scale, not the old.
    EXPECT_NEAR(DrawnSpreadX(pile), 2.0f * spread1, 1e-3f);
}

TEST(LeafPileActor, FadesOverItsLastTwentySecondsThenDies) {
    WorldContext world = g_world;
    LeafPileActor pile({500, 400}, kRadius, kHeight, 0.0);

    pile.tick(world, 1.0 / 60.0, 100.0);
    EXPECT_TRUE(pile.isAlive());
    EXPECT_FLOAT_EQ(pile.alphaMult(), 1.0f);

    pile.tick(world, 1.0 / 60.0, 170.0);  // halfway through the fade
    EXPECT_TRUE(pile.isAlive());
    EXPECT_NEAR(pile.alphaMult(), 0.5f, 1e-4f);
    RecordingRenderer r;
    pile.drawLeaves(r);
    for (const auto& e : r.ellipses) EXPECT_LE(e.color.a, 0.5f + 1e-4f);

    pile.tick(world, 1.0 / 60.0, 181.0);
    EXPECT_FALSE(pile.isAlive());
    pile.tick(world, 1.0 / 60.0, 182.0);  // a dead pile stays dead
    EXPECT_FALSE(pile.isAlive());
}

TEST(LeafPileActor, AKickScattersTheLeavesAndTheySettle) {
    WorldContext world = g_world;
    LeafPileActor pile({500, 400}, kRadius, kHeight, 0.0);
    const float restingWindow = pile.leafBounds().windowSize();

    pile.kick({400.0f, 0.0f}, 1.0, 1.0f);
    for (const auto& leaf : pile.leaves()) EXPECT_GT(leaf.velZ, 0.0f);

    double t = 1.0;
    for (int i = 0; i < 30; ++i) pile.tick(world, 1.0 / 60.0, t += 1.0 / 60.0);
    EXPECT_GT(pile.leafBounds().windowSize(), restingWindow);
    ExpectEveryLeafInsideItsWindow(pile);

    // Gravity and the bounce damping bring every leaf back to the ground.
    for (int i = 0; i < 60 * 30; ++i) pile.tick(world, 1.0 / 60.0, t += 1.0 / 60.0);
    std::vector<LeafParticle> settled = pile.leaves();
    for (const auto& leaf : settled) EXPECT_FLOAT_EQ(leaf.curPosZ, 0.0f);

    // Settled means settled: further frames move nothing.
    pile.tick(world, 1.0 / 60.0, t += 1.0 / 60.0);
    for (size_t i = 0; i < settled.size(); ++i) {
        EXPECT_FLOAT_EQ(pile.leaves()[i].curPosPlanar.x, settled[i].curPosPlanar.x);
        EXPECT_FLOAT_EQ(pile.leaves()[i].curPosPlanar.y, settled[i].curPosPlanar.y);
    }
}

TEST(LeafPileActor, KeepsOneWindowAndClosesItWhenDestroyed) {
    ASSERT_TRUE([NSThread isMainThread]);
    const int before = LeafWindowCount();

    auto* pile = new LeafPileActor({500, 400}, kRadius, kHeight, 0.0);
    pile->render(nullptr);
    EXPECT_EQ(LeafWindowCount(), before + 1);

    // A resting pile redraws nothing, a moving one reuses its window.
    pile->render(nullptr);
    WorldContext world = g_world;
    pile->kick({300.0f, 0.0f}, 1.0, 0.5f);
    pile->tick(world, 1.0 / 60.0, 1.1);
    pile->render(nullptr);
    EXPECT_EQ(LeafWindowCount(), before + 1);

    delete pile;
    GlobalState_DrainMainRunLoop();
    EXPECT_EQ(LeafWindowCount(), before);
}

TEST(LeafPileActor, ADeadPileDrawsNoWindow) {
    WorldContext world = g_world;
    const int before = LeafWindowCount();
    LeafPileActor pile({500, 400}, kRadius, kHeight, 0.0);
    pile.tick(world, 1.0 / 60.0, 200.0);
    pile.render(nullptr);
    EXPECT_EQ(LeafWindowCount(), before);
}

TEST(LeafPileActor, AGooseWalkingThroughThePileKicksIt) {
    g_config.behaviors.fun.autumnLeaves = true;
    auto* pile = new LeafPileActor({500, 400}, kRadius, kHeight, 0.0);
    auto* far = new LeafPileActor({1500, 900}, kRadius, kHeight, 0.0);
    ActorManager::Instance().add(pile);
    ActorManager::Instance().add(far);

    Goose goose(1, "goose", 1920, 1080);
    goose.pos = {500, 400};
    goose.vel = {300, 0};
    goose.currentSpeed = g_config.movement.baseRunSpeed;

    World_TickLeafPiles(1.0, 1.0f / 60.0f, &goose);

    for (const auto& leaf : pile->leaves()) EXPECT_GT(leaf.velZ, 0.0f);
    for (const auto& leaf : far->leaves()) EXPECT_FLOAT_EQ(leaf.velZ, 0.0f);
}

TEST(LeafPileActor, NoGooseOrLeavesOffKicksNothing) {
    auto* pile = new LeafPileActor({500, 400}, kRadius, kHeight, 0.0);
    ActorManager::Instance().add(pile);
    Goose goose(1, "goose", 1920, 1080);
    goose.pos = {500, 400};

    g_config.behaviors.fun.autumnLeaves = true;
    World_TickLeafPiles(1.0, 1.0f / 60.0f, nullptr);
    g_config.behaviors.fun.autumnLeaves = false;
    World_TickLeafPiles(1.0, 1.0f / 60.0f, &goose);

    for (const auto& leaf : pile->leaves()) EXPECT_FLOAT_EQ(leaf.velZ, 0.0f);
}

}  // namespace
