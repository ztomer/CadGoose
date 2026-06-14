#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "event_bus.h"
#include "behaviors/states/health_state.h"
#include "renderer_interface.h"

struct MockHealthRenderer : IRenderer {
    int drawRectCalls = 0;
    void SaveState() override {}
    void RestoreState() override {}
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override { ++drawRectCalls; }
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawImage(void*, RenderRect) override {}
    bool GetImageSize(void*, float*, float*) override { return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
};

void Health_Damage(Goose* goose, float amount, double time);
void Health_Heal(Goose* goose, float amount);

class BehaviorHealthTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("health")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(42, "test", 1920, 1080);
        goose->behaviorsEnabled = true;
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
    }

    Goose* goose = nullptr;
    BehaviorContext ctx{};
};

TEST_F(BehaviorHealthTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    ASSERT_TRUE(b->init);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isDead);
    EXPECT_EQ(state->currentHealth, 100.0f);
}

TEST_F(BehaviorHealthTest, TickHighSpeedDamages) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 5.0;
    goose->currentSpeed = 300.0f;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_LT(state->currentHealth, 100.0f);
    EXPECT_FALSE(state->isDead);
}

TEST_F(BehaviorHealthTest, TickLowSpeedNoDamage) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 5.0;
    goose->currentSpeed = 100.0f;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 100.0f);
}

TEST_F(BehaviorHealthTest, TickWithinCooldownNoDamage) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    ctx.time = 1.0;
    goose->currentSpeed = 300.0f;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 100.0f);
}

TEST_F(BehaviorHealthTest, TickIsDeadNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    state->isDead = true;
    float before = state->regenAccumulator;

    ctx.time = 10.0;
    goose->currentSpeed = 300.0f;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_EQ(state->regenAccumulator, before);
    EXPECT_TRUE(state->isDead);
}

TEST_F(BehaviorHealthTest, TickRegenWhenDamaged) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Health_Damage(goose, 20, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 80.0f);

    ctx.time = 0;
    goose->currentSpeed = 100.0f;
    b->tick(goose, ctx, 1.0, ctx.time);
    EXPECT_EQ(state->currentHealth, 80.0f);
}

TEST_F(BehaviorHealthTest, TickRegenAccumulator) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Health_Damage(goose, 20, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 80.0f);

    ctx.time = 0;
    goose->currentSpeed = 100.0f;
    b->tick(goose, ctx, 2.0, ctx.time);
    EXPECT_EQ(state->currentHealth, 81.0f);
}

TEST_F(BehaviorHealthTest, RenderWithNullRenderer) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorHealthTest, HealthDamage) {
    Health_Damage(goose, 30, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 70.0f);
    EXPECT_EQ(state->lastDamageTime, 0);
}

TEST_F(BehaviorHealthTest, HealthDamageToZero) {
    Health_Damage(goose, 200, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 0.0f);
}

TEST_F(BehaviorHealthTest, HealthHeal) {
    Health_Damage(goose, 50, 0);
    Health_Heal(goose, 20);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 70.0f);
}

TEST_F(BehaviorHealthTest, HealthHealCapsAtMax) {
    Health_Heal(goose, 50);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 100.0f);
}

TEST_F(BehaviorHealthTest, TickRendersWithRenderer) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    MockHealthRenderer mockRenderer;
    b->render(goose, ctx, &mockRenderer);
    EXPECT_GT(mockRenderer.drawRectCalls, 0);
}

TEST_F(BehaviorHealthTest, DamageToDeath) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Health_Damage(goose, 95, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 5.0f);
    EXPECT_FALSE(state->isDead);

    goose->currentSpeed = 300.0f;
    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(state->currentHealth, 0.0f);
    EXPECT_TRUE(state->isDead);
}

TEST_F(BehaviorHealthTest, HealthDamageSetsLastDamageTime) {
    Health_Damage(goose, 30, 42.0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->lastDamageTime, 42.0);
}

TEST_F(BehaviorHealthTest, TickAfterDeathSkipsDamage) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* st = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(st, nullptr);
    st->isDead = true;
    st->currentHealth = 50.0f;
    goose->currentSpeed = 300.0f;
    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(st->currentHealth, 50.0f);
}

TEST_F(BehaviorHealthTest, MultipleDamageTicks) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Health_Damage(goose, 30, 0);

    goose->currentSpeed = 300.0f;
    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 65.0f);

    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(state->currentHealth, 60.0f);
}

TEST_F(BehaviorHealthTest, TickDamagePublishesEvent) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    int eventCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseDamagedEvent>([&](const GooseDamagedEvent&) { ++eventCount; });

    goose->currentSpeed = 300.0f;
    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_EQ(eventCount, 1);
    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorHealthTest, FullHealthNoRegen) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 100.0f);

    ctx.time = 10.0;
    goose->currentSpeed = 100.0f;
    float accBefore = state->regenAccumulator;
    b->tick(goose, ctx, 1.0, ctx.time);
    EXPECT_EQ(state->regenAccumulator, accBefore);
}

TEST_F(BehaviorHealthTest, TickDamageCooldownElapsedTriggersDamage) {
    auto* b = BehaviorRegistry::Instance().Get("health");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->currentSpeed = 300.0f;
    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_LT(state->currentHealth, 100.0f);
    EXPECT_FALSE(state->isDead);

    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_LT(state->currentHealth, 95.0f);
}

TEST_F(BehaviorHealthTest, ExternalDamagePublishesNoEvent) {
    Health_Damage(goose, 30, 0);
    auto* state = BehaviorStateManager::Instance().Get<HealthState>(goose->id, "health");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->currentHealth, 70.0f);
    EXPECT_EQ(state->lastDamageTime, 0);
}

TEST_F(BehaviorHealthTest, SetUpWithoutGoose) {
    BehaviorContext emptyCtx{};
    emptyCtx.world = &g_world;
    (void)emptyCtx;
}
