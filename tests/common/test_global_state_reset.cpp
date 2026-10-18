// GlobalState_Restore() puts back every piece of process-wide state a test can
// leave behind (ROADMAP G0.13). The listener in tests/global_state_reset.h runs
// it after EVERY test; this drives it directly, so the restore is proven here
// rather than inferred from a green shuffle.
#include <gtest/gtest.h>

#include <memory>

#include "actor.h"
#include "behavior.h"
#include "behavior_manager.h"
#include "config.h"
#include "cursor_io.h"
#include "event_bus.h"
#include "../global_state_reset.h"
#include "goose.h"
#include "world.h"

namespace {

bool s_scratchEnabled = true;

struct NullCursor : ICursorProvider {
    CursorState Read() override { return {}; }
    void Execute(const CursorAction&) override {}
};

}  // namespace

TEST(GlobalStateReset, RestoresTheEngineSingletons) {
    auto& reg = BehaviorRegistry::Instance();
    const size_t behaviors = reg.GetBehaviorCount();
    const bool visible = g_config.behaviors.info.visible;
    const double time = g_time;
    const int width = g_world.screenWidth;
    const size_t footprints = g_world.footprints.size();
    ICursorProvider* const provider = g_cursorProvider;
    ASSERT_EQ(ActorManager::Instance().totalCount(), 0);
    ASSERT_EQ(BehaviorStateManager::Instance().GetStateCount(), 0u);

    // A behavior whose callbacks capture this frame: left registered, the next
    // test to tick or render the registry calls into a dead stack (the
    // GooseRender SIGSEGV/SIGBUS seeds).
    auto scratch = std::make_unique<Behavior>();
    scratch->id = "global_state_reset_scratch";
    scratch->name = scratch->id;
    scratch->enabledPtr = &s_scratchEnabled;
    reg.Register(*scratch);
    g_config.behaviors.info.visible = !visible;
    g_time = time + 1000.0;
    g_world.screenWidth = width + 7;
    g_world.footprints.push(Footprint{});
    NullCursor cursor;
    g_cursorProvider = &cursor;
    auto* goose = new Goose(4242, "leaked", 1920, 1080);
    ActorManager::Instance().add(goose);
    BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(4242, "scratch");
    int heard = 0;
    EventBus::Instance().Subscribe<GooseHonkedEvent>(
        [&heard](const GooseHonkedEvent&) { ++heard; });

    GlobalState_Restore();

    EXPECT_EQ(reg.GetBehaviorCount(), behaviors);
    EXPECT_EQ(reg.Get("global_state_reset_scratch"), nullptr);
    EXPECT_EQ(g_config.behaviors.info.visible, visible);
    EXPECT_EQ(g_time, time);
    EXPECT_EQ(g_world.screenWidth, width);
    EXPECT_EQ(g_world.footprints.size(), footprints);
    EXPECT_EQ(g_cursorProvider, provider);
    EXPECT_EQ(ActorManager::Instance().totalCount(), 0);
    EXPECT_EQ(BehaviorStateManager::Instance().GetStateCount(), 0u);
    EventBus::Instance().Publish(GooseHonkedEvent{});
    EXPECT_EQ(heard, 0) << "a subscription outlived the restore";
}
