// Exercises the REAL effect registrations that ship inside the binary.
//
// effect_reg_footprint.mm and effect_reg_pomodorobed.mm register themselves
// from static initializers, so their function pointers are live as soon as
// the test binary links them — no mock registration needed. These tests call
// through EffectGetRegistrations() exactly the way EffectWindowManager does.
//
// configureWindow is deliberately NOT exercised here: it needs a real
// EffectWindow (AppKit window creation), which is GUI-tier and belongs to
// the display-dependent suites.
#import <gtest/gtest.h>
#import "effect_registration.h"
#import "effect_window.h"
#import "world.h"
#import "config.h"
#import "goose.h"
#import "actor_manager.h"
#import "pomodoro_bed.h"
#import "behavior_manager.h"
#import "behaviors/states/pomodoro_state.h"

namespace {

const EffectRegistration* FindReg(EffectType type) {
    for (const auto& reg : EffectGetRegistrations()) {
        if (reg.type == type) return &reg;
    }
    return nullptr;
}

TEST(LiveEffectRegistration, FootprintTypeIsRegistered) {
    EXPECT_NE(FindReg(EffectTypeFootprint), nullptr);
}

TEST(LiveEffectRegistration, FootprintPositionsFilteredByLifetime) {
    const auto* reg = FindReg(EffectTypeFootprint);
    ASSERT_NE(reg, nullptr);

    g_world.footprints.clear();
    const double now = g_time;

    Footprint fresh{};
    fresh.pos = {100.0f, 200.0f};
    fresh.dir = 0;
    fresh.timeSpawned = now;
    fresh.lifetime = 10.0f;
    g_world.footprints.push(fresh);

    Footprint expired{};
    expired.pos = {300.0f, 400.0f};
    expired.dir = 90;
    expired.timeSpawned = now - 1000.0;  // far older than any lifetime
    expired.lifetime = 10.0f;
    g_world.footprints.push(expired);

    auto positions = reg->getPositions();
    ASSERT_EQ(positions.size(), 1u);
    EXPECT_FLOAT_EQ(positions[0].x, 100.0f);
    EXPECT_FLOAT_EQ(positions[0].y, 200.0f);

    // Radius derives from the configured footprint width.
    EXPECT_FLOAT_EQ(reg->getRadius({}), g_config.render.footprintWidth * 0.5f);

    EXPECT_TRUE(reg->existsAt({100.0f, 200.0f}));
    EXPECT_FALSE(reg->existsAt({300.0f, 400.0f}));  // expired
    EXPECT_FALSE(reg->existsAt({999.0f, 999.0f}));  // never existed

    g_world.footprints.clear();
}

TEST(LiveEffectRegistration, PomodoroBedTypeIsRegistered) {
    EXPECT_NE(FindReg(EffectTypePomodoroBed), nullptr);
}

TEST(LiveEffectRegistration, PomodoroBedPositionsFollowSleepingGeese) {
    const auto* reg = FindReg(EffectTypePomodoroBed);
    ASSERT_NE(reg, nullptr);

    Goose* goose = new Goose(777001, "bedreg", 1920, 1080);
    ActorManager::Instance().add(goose);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(goose->id, "pomodoro");
    state->bedPosition = {640.0f, 480.0f};
    state->isSleeping = true;

    auto positions = reg->getPositions();
    bool found = false;
    for (const auto& p : positions) {
        if (std::abs(p.x - 640.0f) < 1.0f && std::abs(p.y - 480.0f) < 1.0f) found = true;
    }
    EXPECT_TRUE(found);

    EXPECT_TRUE(reg->existsAt({640.0f, 480.0f}));
    EXPECT_FALSE(reg->existsAt({10.0f, 10.0f}));

    // Radius: half the bed image's max dimension when the image is loaded,
    // otherwise the documented 30px fallback.
    float radius = reg->getRadius({640.0f, 480.0f});
    EXPECT_GT(radius, 0.0f);

    // Awake geese expose no bed.
    state->isSleeping = false;
    EXPECT_EQ(reg->getPositions().size(), 0u);

    ActorManager::Instance().remove(goose);  // erase without delete; we own it
    delete goose;
}

}  // namespace
