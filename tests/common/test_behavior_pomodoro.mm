#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "event_bus.h"
#include "pomodoro_bed.h"
#include "renderer_interface.h"
#include "behaviors/states/pomodoro_state.h"
#include "items.h"

struct MockPomoRenderer : public IRenderer {
    int saveCount = 0, restoreCount = 0;
    int roundedRectCount = 0;
    int textCount = 0;
    int setAlphaCount = 0;
    float measureTextReturn = 50.0f;

    void SaveState() override { ++saveCount; }
    void RestoreState() override { ++restoreCount; }
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override { ++roundedRectCount; }
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    int imageCount = 0;
    void DrawImage(void*, RenderRect) override { ++imageCount; }
    bool GetImageSize(void* img, float* w, float* h) override { if (img && w && h) { *w = 30; *h = 30; return true; } return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override { ++textCount; }
    float MeasureText(const char*, float) override { return measureTextReturn; }
    void SetAlpha(float) override { ++setAlphaCount; }
};

struct PomoSpyGoose : public Goose {
    int honkCount = 0;
    PomoSpyGoose(int id) : Goose(id, "pomodoro", 1920, 1080) { behaviorsEnabled = true; m_canHonk = true; }
    void onHonk() override { ++honkCount; }
};

class BehaviorPomodoroTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("pomodoro")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedPomoEnabled = g_config.behaviors.systems.pomodoro;
        savedWorkMin = g_config.behaviors.pomodoro.workMinutes;
        savedBreakMin = g_config.behaviors.pomodoro.breakMinutes;
        savedLongBreakMin = g_config.behaviors.pomodoro.longBreakMinutes;
        savedSessions = g_config.behaviors.pomodoro.sessionsBeforeLongBreak;
        savedAggressive = g_config.behaviors.pomodoro.enableAggressiveMode;
        savedAggressiveHonkInterval = g_config.behaviors.pomodoro.aggressiveHonkInterval;
        savedBaseWalkSpeed = g_config.movement.baseWalkSpeed;
        savedBaseRunSpeed = g_config.movement.baseRunSpeed;
        savedP2Width = g_config.portal.p2Width;
        savedP2Height = g_config.portal.p2Height;

        g_config.behaviors.systems.pomodoro = true;
        g_config.behaviors.pomodoro.workMinutes = 1;
        g_config.behaviors.pomodoro.breakMinutes = 1;
        g_config.behaviors.pomodoro.longBreakMinutes = 1;
        g_config.behaviors.pomodoro.sessionsBeforeLongBreak = 2;
        g_config.behaviors.pomodoro.enableAggressiveMode = true;
        g_config.behaviors.pomodoro.aggressiveHonkInterval = 2.0f;
        g_config.movement.baseWalkSpeed = 100.0f;
        g_config.movement.baseRunSpeed = 200.0f;

        g_world.screenWidth = 1920;
        g_world.screenHeight = 1080;

        goose = new PomoSpyGoose(1);
        goose->pos = {500, 500};
        goose->dir = 0;
        goose->vel = {0, 0};
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
        g_config.behaviors.systems.pomodoro = savedPomoEnabled;
        g_config.behaviors.pomodoro.workMinutes = savedWorkMin;
        g_config.behaviors.pomodoro.breakMinutes = savedBreakMin;
        g_config.behaviors.pomodoro.longBreakMinutes = savedLongBreakMin;
        g_config.behaviors.pomodoro.sessionsBeforeLongBreak = savedSessions;
        g_config.behaviors.pomodoro.enableAggressiveMode = savedAggressive;
        g_config.behaviors.pomodoro.aggressiveHonkInterval = savedAggressiveHonkInterval;
        g_config.movement.baseWalkSpeed = savedBaseWalkSpeed;
        g_config.movement.baseRunSpeed = savedBaseRunSpeed;
    }

    PomoSpyGoose* goose;
    BehaviorContext ctx{};

private:
    bool savedPomoEnabled;
    int savedWorkMin, savedBreakMin, savedLongBreakMin, savedSessions;
    bool savedAggressive;
    float savedAggressiveHonkInterval;
    float savedBaseWalkSpeed, savedBaseRunSpeed;
    float savedP2Width, savedP2Height;
};

TEST_F(BehaviorPomodoroTest, InitCreatesStateAndBedPosition) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->bedPosition.x, 1770);
    EXPECT_EQ(state->bedPosition.y, 980);
    EXPECT_EQ(state->phase, PomodoroPhase::Work);
    EXPECT_EQ(state->phaseStartTime, 0);
}

TEST_F(BehaviorPomodoroTest, InitNoScreenDims) {
    g_world.screenWidth = 0;
    g_world.screenHeight = 0;
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->bedPosition.x, -1);
    EXPECT_EQ(state->bedPosition.y, -1);
}

TEST_F(BehaviorPomodoroTest, DisabledBehaviorResetsState) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);

    g_config.behaviors.systems.pomodoro = false;
    goose->isResting = true;
    state->accumulatedRotation = 90;
    state->speedMultiplierApplied = true;
    goose->currentSpeed = 999;

    b->tick(goose, ctx, 0.016, 1.0);

    EXPECT_FALSE(goose->isResting);
    EXPECT_FLOAT_EQ(goose->currentSpeed, g_config.movement.baseWalkSpeed);
}

TEST_F(BehaviorPomodoroTest, WorkPhaseWalksToBed) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    Vector2 expectedBed{1770, 980};
    EXPECT_FALSE(goose->isResting);
    EXPECT_FLOAT_EQ(goose->target.x, expectedBed.x);
    EXPECT_FLOAT_EQ(goose->target.y, expectedBed.y);
    EXPECT_GT(Vector2::Length(goose->vel), 0);
}

TEST_F(BehaviorPomodoroTest, WorkToBreakTransition) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    int eventCount = 0;
    auto sub = EventBus::Instance().Subscribe<PomodoroPhaseChangedEvent>(
        [&](const PomodoroPhaseChangedEvent&) { ++eventCount; });

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Break);
    EXPECT_EQ(state->completedSessions, 1);
    EXPECT_TRUE(state->isAggressive);
    EXPECT_EQ(eventCount, 1);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorPomodoroTest, BreakToWorkTransition) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    ctx.time = 130.0;
    b->tick(goose, ctx, 0.016, 130.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Work);
    EXPECT_FALSE(state->isAggressive);
}

TEST_F(BehaviorPomodoroTest, SessionsReachLongBreak) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    ctx.time = 130.0;
    b->tick(goose, ctx, 0.016, 130.0);

    ctx.time = 195.0;
    b->tick(goose, ctx, 0.016, 195.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::LongBreak);
    EXPECT_EQ(state->completedSessions, 0);
}

TEST_F(BehaviorPomodoroTest, LongBreakToWork) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);
    ctx.time = 130.0;
    b->tick(goose, ctx, 0.016, 130.0);
    ctx.time = 195.0;
    b->tick(goose, ctx, 0.016, 195.0);
    ctx.time = 260.0;
    b->tick(goose, ctx, 0.016, 260.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Work);
}

TEST_F(BehaviorPomodoroTest, AggressiveBreakRotatesAndHonks) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Break);
    EXPECT_TRUE(state->isAggressive);

    float dirBefore = goose->dir;
    EXPECT_GE(goose->honkCount, 1);

    ctx.time = 67.0;
    b->tick(goose, ctx, 2.0, 67.0);

    EXPECT_NE(goose->dir, dirBefore);
    EXPECT_GE(goose->honkCount, 2);
}

TEST_F(BehaviorPomodoroTest, SleepStateAtBed) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {1765, 975};

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isSleeping);
    EXPECT_TRUE(goose->isResting);
    EXPECT_FLOAT_EQ(goose->vel.x, 0);
    EXPECT_FLOAT_EQ(goose->vel.y, 0);

    ctx.time = 2.1;
    b->tick(goose, ctx, 2.0, 2.1);

    EXPECT_GT(state->slowRotateTimer, 0);
}

TEST_F(BehaviorPomodoroTest, BedPositionUpdatesOnScreenResize) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->bedPosition.x, 1770);

    g_world.screenWidth = 2560;
    g_world.screenHeight = 1440;

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    EXPECT_FLOAT_EQ(state->bedPosition.x, 2410);
    EXPECT_FLOAT_EQ(state->bedPosition.y, 1340);
}

TEST_F(BehaviorPomodoroTest, RenderTimer) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->rig.neckHead = {100, 100};
    MockPomoRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_EQ(renderer.textCount, 1);
}

TEST_F(BehaviorPomodoroTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorPomodoroTest, RenderSleepingZZZ) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {1765, 975};
    goose->rig.neckHead = {100, 100};

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    MockPomoRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_EQ(renderer.saveCount, renderer.restoreCount);
    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_TRUE(renderer.textCount >= 1);
    // If zzz images loaded from disk, image-draw path is taken; otherwise text fallback
    EXPECT_TRUE(renderer.imageCount > 0 || renderer.textCount >= 2);
}

TEST_F(BehaviorPomodoroTest, PomodoroGetBedInfo) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PomodoroBedInfo info = Pomodoro_GetBedInfo(1);

    EXPECT_FLOAT_EQ(info.position.x, 1770);
    EXPECT_FLOAT_EQ(info.position.y, 980);
    EXPECT_FALSE(info.visible);
}

TEST_F(BehaviorPomodoroTest, PomodoroGetBedInfoNoState) {
    PomodoroBedInfo info = Pomodoro_GetBedInfo(999);
    EXPECT_FLOAT_EQ(info.position.x, 0);
    EXPECT_FLOAT_EQ(info.position.y, 0);
    EXPECT_FALSE(info.visible);
    EXPECT_EQ(info.bedImage, nullptr);
}

TEST_F(BehaviorPomodoroTest, WorkPhaseForceItemFetchNegative) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->forceItemFetch = static_cast<FetchType>(5);

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    EXPECT_EQ(goose->forceItemFetch, FetchType::Random);
}

TEST_F(BehaviorPomodoroTest, WorkPhaseCancelsFetchingWithHeldItem) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->state = GooseState::FETCHING;
    goose->heldItem = new ItemData();

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    EXPECT_EQ(goose->state, GooseState::WANDER);
    EXPECT_EQ(goose->heldItem, nullptr);
}

TEST_F(BehaviorPomodoroTest, BedPositionUpdatesFromNegative) {
    g_world.screenWidth = 0;
    g_world.screenHeight = 0;
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->bedPosition.x, -1);

    g_world.screenWidth = 1920;
    g_world.screenHeight = 1080;

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    EXPECT_FLOAT_EQ(state->bedPosition.x, 1770);
}

TEST_F(BehaviorPomodoroTest, NonAggressiveBreakNoRotation) {
    g_config.behaviors.pomodoro.enableAggressiveMode = false;
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Break);
    EXPECT_FALSE(state->isAggressive);

    EXPECT_FALSE(goose->isResting);
}

TEST_F(BehaviorPomodoroTest, CleanupFunction) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->cleanup(ctx);
}

TEST_F(BehaviorPomodoroTest, RenderBreakLabel) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    state->phase = PomodoroPhase::Break;
    state->isAggressive = false;
    state->phaseStartTime = ctx.time;

    goose->rig.neckHead = {100, 100};

    MockPomoRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_EQ(renderer.textCount, 1);
}

TEST_F(BehaviorPomodoroTest, RenderLongBreakLabel) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    state->phase = PomodoroPhase::LongBreak;
    state->isAggressive = false;
    state->phaseStartTime = ctx.time;

    goose->rig.neckHead = {100, 100};

    MockPomoRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_EQ(renderer.textCount, 1);
}

TEST_F(BehaviorPomodoroTest, NonAggressiveBreakResetsAccumulatedRotation) {
    g_config.behaviors.pomodoro.enableAggressiveMode = false;
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 65.0;
    b->tick(goose, ctx, 0.016, 65.0);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->phase, PomodoroPhase::Break);
    EXPECT_FALSE(state->isAggressive);

    state->accumulatedRotation = 90.0f;
    state->speedMultiplierApplied = true;
    float dirBefore = goose->dir;

    ctx.time = 66.0;
    b->tick(goose, ctx, 1.0, 66.0);

    EXPECT_NE(goose->dir, dirBefore);
    EXPECT_FLOAT_EQ(state->accumulatedRotation, 0);
    EXPECT_FALSE(state->speedMultiplierApplied);
    EXPECT_FLOAT_EQ(goose->currentSpeed, g_config.movement.baseWalkSpeed);
}

TEST_F(BehaviorPomodoroTest, SleepSlowRotationFlips) {
    auto* b = BehaviorRegistry::Instance().Get("pomodoro");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {1765, 975};

    ctx.time = 0.1;
    b->tick(goose, ctx, 0.016, 0.1);

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(1, "pomodoro");
    ASSERT_NE(state, nullptr);
    ASSERT_TRUE(state->isSleeping);

    float dir1 = goose->dir;
    ctx.time = 3.0;
    b->tick(goose, ctx, 2.9, 3.0);
    float dir2 = goose->dir;
    EXPECT_NE(dir1, dir2);

    ctx.time = 6.0;
    b->tick(goose, ctx, 3.0, 6.0);
    float dir3 = goose->dir;
    EXPECT_NE(dir3, dir2);
}
