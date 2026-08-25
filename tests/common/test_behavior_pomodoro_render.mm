#import "test_behavior_pomodoro_fixture.h"

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
