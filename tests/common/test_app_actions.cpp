#include <gtest/gtest.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include "app_actions.h"
#include "goose.h"
#include "world.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include "behavior_registry.h"
#include "config.h"
#include "assets.h"
#include "items.h"

namespace {
    // Local test behaviors for enable/disable tests, isolated from real
    // behaviors that get cleared by test_behavior_core.cpp.
    std::unordered_map<std::string, bool> g_appActionsTestEnabled;
    std::vector<std::unique_ptr<Behavior>> g_appActionsTestBehaviors;

    void RegisterAppActionsTestBehavior(const char* id) {
        g_appActionsTestEnabled[id] = false;
        auto bhv = std::make_unique<Behavior>();
        bhv->id = id;
        bhv->name = id;
        bhv->enabledPtr = &g_appActionsTestEnabled[id];
        bhv->configPtr = &g_appActionsTestEnabled[id];
        BehaviorRegistry::Instance().Register(*bhv);
        g_appActionsTestBehaviors.push_back(std::move(bhv));
    }
}

// ============================================================
// AppActions_SpawnGoose
// ============================================================

TEST(AppActions, SpawnGooseWithName) {
    Goose* g = AppActions_SpawnGoose("Alice");
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->name, "Alice");
    EXPECT_EQ(g->state, GooseState::WANDER);

    auto geese = ActorManager::Instance().getGeese();
    EXPECT_EQ(geese.size(), 1);
    EXPECT_EQ(geese[0]->name, "Alice");

    AppActions_ClearGeese();
}

TEST(AppActions, SpawnGooseEmptyNameDefault) {
    g_config.gooseNames.clear();
    EXPECT_TRUE(g_config.gooseNames.empty());
    Goose* g = AppActions_SpawnGoose("");
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->name, "Goose 0");
    AppActions_ClearGeese();
}

TEST(AppActions, SpawnGooseEmptyNameUsesSavedNames) {
    g_config.gooseNames.clear();
    g_config.gooseNames.push("SavedName");

    Goose* g = AppActions_SpawnGoose("");
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->name, "SavedName");
    AppActions_ClearGeese();
}

// ============================================================
// AppActions_EnsureInitialGoose
// ============================================================

TEST(AppActions, EnsureInitialGooseNoGeese) {
    ASSERT_TRUE(ActorManager::Instance().getGeese().empty());
    AppActions_EnsureInitialGoose();
    EXPECT_FALSE(ActorManager::Instance().getGeese().empty());
    AppActions_ClearGeese();
}

TEST(AppActions, EnsureInitialGooseExistingGeese) {
    AppActions_SpawnGoose("Existing");
    size_t before = ActorManager::Instance().getGeese().size();
    ASSERT_GT(before, 0);

    AppActions_EnsureInitialGoose();
    EXPECT_EQ(ActorManager::Instance().getGeese().size(), before);
    AppActions_ClearGeese();
}

// ============================================================
// AppActions_ClearGeese
// ============================================================

TEST(AppActions, ClearGeeseRemovesAll) {
    AppActions_SpawnGoose("G1");
    AppActions_SpawnGoose("G2");
    EXPECT_EQ(ActorManager::Instance().getGeese().size(), 2);

    AppActions_ClearGeese();
    EXPECT_TRUE(ActorManager::Instance().getGeese().empty());
}

// ============================================================
// AppActions_GetStatus
// ============================================================

TEST(AppActions, GetStatusNoGeese) {
    std::string status = AppActions_GetStatus();
    EXPECT_NE(status.find("running=1"), std::string::npos);
    EXPECT_NE(status.find("goose_count=0"), std::string::npos);
    EXPECT_NE(status.find("config_path="), std::string::npos);
    EXPECT_NE(status.find("dropped_items="), std::string::npos);
}

TEST(AppActions, GetStatusWithGeese) {
    AppActions_SpawnGoose("StatusTest");
    g_world.screenWidth = 1920;
    g_world.screenHeight = 1080;

    std::string status = AppActions_GetStatus();
    EXPECT_NE(status.find("goose_count=1"), std::string::npos);
    EXPECT_NE(status.find("goose_state="), std::string::npos);
    EXPECT_NE(status.find("goose_pos="), std::string::npos);
    EXPECT_NE(status.find("goose_heldItem=no"), std::string::npos);
    EXPECT_NE(status.find("goose_dir="), std::string::npos);

    AppActions_ClearGeese();
}

TEST(AppActions, GetStatusWithDroppedItems) {
    Goose* g = AppActions_SpawnGoose("DropTestG");
    g_world.screenWidth = 1920;
    g_world.screenHeight = 1080;

    // Constructor auto-adds to ActorManager — do not call add() again.
    DroppedItem nullItem;
    nullItem.data = nullptr;
    nullItem.pos = {50, 60};
    nullItem.rotation = 1.5f;
    nullItem.timeDropped = 0;
    nullItem.pinned = false;
    new DroppedItemActor(nullItem);

    std::string status = AppActions_GetStatus();
    EXPECT_NE(status.find("dropped_items=1"), std::string::npos);
    EXPECT_NE(status.find("item_null"), std::string::npos);

    ActorManager::Instance().removeAllDroppedItems();

    // Non-null data path
    ItemData* memeData = g_assets.GetRandomMeme();
    ASSERT_NE(memeData, nullptr);
    ASSERT_GT(memeData->w, 0);
    ASSERT_GT(memeData->h, 0);

    DroppedItem realItem;
    realItem.data = memeData;
    realItem.pos = {100, 200};
    realItem.rotation = 0.5f;
    realItem.timeDropped = 0;
    realItem.pinned = true;
    new DroppedItemActor(realItem);

    status = AppActions_GetStatus();
    EXPECT_NE(status.find("dropped_items=1"), std::string::npos);
    EXPECT_NE(status.find("item_pos=100.0,200.0"), std::string::npos);
    EXPECT_NE(status.find("pinned=1"), std::string::npos);

    ActorManager::Instance().removeAllDroppedItems();
    AppActions_ClearGeese();
}

TEST(AppActions, GetStatusWithFetchingState) {
    Goose* g = AppActions_SpawnGoose("FetchState");
    g->state = GooseState::FETCHING;
    std::string status = AppActions_GetStatus();
    EXPECT_NE(status.find("goose_state=fetching"), std::string::npos);

    g->state = GooseState::RETURNING;
    status = AppActions_GetStatus();
    EXPECT_NE(status.find("goose_state=returning"), std::string::npos);

    g->state = GooseState::CHASE_CURSOR;
    status = AppActions_GetStatus();
    EXPECT_NE(status.find("goose_state=chase_cursor"), std::string::npos);

    g->state = GooseState::SNATCH_CURSOR;
    status = AppActions_GetStatus();
    EXPECT_NE(status.find("goose_state=snatch_cursor"), std::string::npos);

    AppActions_ClearGeese();
}

// ============================================================
// AppActions_HandleCommand
// ============================================================

TEST(AppActions, HandleCommandEmpty) {
    std::string r = AppActions_HandleCommand({});
    EXPECT_EQ(r, "error missing command\n");
}

TEST(AppActions, HandleCommandUnknown) {
    std::string r = AppActions_HandleCommand({"nonexistent"});
    EXPECT_EQ(r, "error unknown command: nonexistent\n");
}

TEST(AppActions, HandleCommandSpawn) {
    std::string r = AppActions_HandleCommand({"spawn", "CmdSpawn"});
    EXPECT_NE(r.find("ok id="), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandClear) {
    AppActions_SpawnGoose("ToClear");
    EXPECT_FALSE(ActorManager::Instance().getGeese().empty());

    std::string r = AppActions_HandleCommand({"clear"});
    EXPECT_EQ(r, "ok\n");
    EXPECT_TRUE(ActorManager::Instance().getGeese().empty());
}

TEST(AppActions, HandleCommandStatus) {
    AppActions_SpawnGoose("StatusCmd");
    g_world.screenWidth = 1920;
    g_world.screenHeight = 1080;

    std::string r = AppActions_HandleCommand({"status"});
    EXPECT_NE(r.find("running=1"), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandClearDropped) {
    std::string r = AppActions_HandleCommand({"clear_dropped"});
    EXPECT_EQ(r, "ok\n");
}

TEST(AppActions, HandleCommandRam) {
    std::string r = AppActions_HandleCommand({"ram"});
    EXPECT_EQ(r, "");
}

TEST(AppActions, HandleCommandFetchNoGoose) {
    std::string r = AppActions_HandleCommand({"fetch"});
    EXPECT_EQ(r, "error no goose\n");
}

TEST(AppActions, HandleCommandFetchNumericIndex) {
    AppActions_SpawnGoose("FetchG");
    std::string r = AppActions_HandleCommand({"fetch", "0", "test"});
    EXPECT_NE(r.find("ok force_fetch goose=0 type=2"), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandFetchAlphaType) {
    AppActions_SpawnGoose("FetchG2");
    std::string r = AppActions_HandleCommand({"fetch", "test"});
    EXPECT_NE(r.find("ok force_fetch goose=0 type=2"), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandFetchAlphaText) {
    AppActions_SpawnGoose("FetchText");
    std::string r = AppActions_HandleCommand({"fetch", "text"});
    EXPECT_NE(r.find("ok force_fetch goose=0 type=1"), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandFetchAlphaMeme) {
    AppActions_SpawnGoose("FetchMeme");
    std::string r = AppActions_HandleCommand({"fetch", "meme"});
    EXPECT_NE(r.find("ok force_fetch goose=0 type=0"), std::string::npos);
    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandFetchWithIndexAndType) {
    AppActions_SpawnGoose("F1");
    AppActions_SpawnGoose("F2");

    std::string r = AppActions_HandleCommand({"fetch", "1", "text"});
    EXPECT_NE(r.find("ok force_fetch goose=1 type=1"), std::string::npos);

    AppActions_ClearGeese();
}

TEST(AppActions, HandleCommandEnable) {
    RegisterAppActionsTestBehavior("app_test_beh");
    auto* h = BehaviorRegistry::Instance().Get("app_test_beh");
    ASSERT_NE(h, nullptr);
    ASSERT_NE(h->enabledPtr, nullptr);
    *h->enabledPtr = false;
    if (h->configPtr) *h->configPtr = false;

    std::string r = AppActions_HandleCommand({"enable", "app_test_beh"});
    EXPECT_EQ(r, "ok enabled app_test_beh\n");
    EXPECT_TRUE(*h->enabledPtr);
}

TEST(AppActions, HandleCommandDisable) {
    RegisterAppActionsTestBehavior("app_test_bd");
    auto* h = BehaviorRegistry::Instance().Get("app_test_bd");
    ASSERT_NE(h, nullptr);
    *h->enabledPtr = true;
    if (h->configPtr) *h->configPtr = true;

    std::string r = AppActions_HandleCommand({"disable", "app_test_bd"});
    EXPECT_EQ(r, "ok disabled app_test_bd\n");
    EXPECT_FALSE(*h->enabledPtr);
}

TEST(AppActions, HandleCommandEnableNoEnabledPtr) {
    auto bhv = std::make_unique<Behavior>();
    bhv->id = "no_enable_ptr_beh";
    bhv->name = "no_enable_ptr_beh";
    bhv->enabledPtr = nullptr;
    bhv->configPtr = nullptr;
    BehaviorRegistry::Instance().Register(*bhv);
    g_appActionsTestBehaviors.push_back(std::move(bhv));

    std::string r = AppActions_HandleCommand({"enable", "no_enable_ptr_beh"});
    EXPECT_EQ(r, "error behavior has no enabledPtr\n");
}

TEST(AppActions, HandleCommandDisableNoEnabledPtr) {
    auto bhv = std::make_unique<Behavior>();
    bhv->id = "no_disable_ptr_beh";
    bhv->name = "no_disable_ptr_beh";
    bhv->enabledPtr = nullptr;
    bhv->configPtr = nullptr;
    BehaviorRegistry::Instance().Register(*bhv);
    g_appActionsTestBehaviors.push_back(std::move(bhv));

    std::string r = AppActions_HandleCommand({"disable", "no_disable_ptr_beh"});
    EXPECT_EQ(r, "error behavior has no enabledPtr\n");
}

TEST(AppActions, HandleCommandEnableUnknown) {
    std::string r = AppActions_HandleCommand({"enable", "bogus"});
    EXPECT_EQ(r, "error unknown behavior: bogus\n");
}

TEST(AppActions, HandleCommandDisableUnknown) {
    std::string r = AppActions_HandleCommand({"disable", "bogus"});
    EXPECT_EQ(r, "error unknown behavior: bogus\n");
}

TEST(AppActions, HandleCommandEnableMissingBehaviorId) {
    std::string r = AppActions_HandleCommand({"enable"});
    EXPECT_EQ(r, "error missing behavior id\n");
}

TEST(AppActions, HandleCommandDisableMissingBehaviorId) {
    std::string r = AppActions_HandleCommand({"disable"});
    EXPECT_EQ(r, "error missing behavior id\n");
}

TEST(AppActions, HandleCommandQuit) {
    AppActions_SpawnGoose("QuitG");
    std::string r = AppActions_HandleCommand({"quit"});
    EXPECT_EQ(r, "ok cleared and quitting\n");
    EXPECT_TRUE(ActorManager::Instance().getGeese().empty());
}
