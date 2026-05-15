#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "mcp_server.h"
#include "config.h"

class MCPConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        savedBall = g_config.behaviors.fun.ball;
        savedBreadcrumbs = g_config.behaviors.fun.breadCrumbs;
        savedHats = g_config.behaviors.fun.hats;
        savedRainbow = g_config.behaviors.fun.rainbow;
        savedAcid = g_config.behaviors.fun.acid;
        savedAnger = g_config.behaviors.fun.anger;
        savedAutumnLeaves = g_config.behaviors.fun.autumnLeaves;
        savedHoncker = g_config.behaviors.control.honcker;
        savedJail = g_config.behaviors.control.jail;
        savedPortals = g_config.behaviors.control.portals;
        savedDrag = g_config.behaviors.control.drag;
        savedNametag = g_config.behaviors.info.nametag;
        savedPresence = g_config.behaviors.info.presence;
        savedConfigGUI = g_config.behaviors.info.configGUI;
        savedVisible = g_config.behaviors.info.visible;
        savedHealth = g_config.behaviors.systems.health;
        savedAi = g_config.behaviors.systems.ai;
        savedPomodoro = g_config.behaviors.systems.pomodoro;
        savedHonkerHotkey = g_config.behaviors.honcker.hotkey;
        savedJailHotkeyO = g_config.behaviors.jail.hotkeyO;
        savedJailHotkeyP = g_config.behaviors.jail.hotkeyP;
        savedPortalHotkey1 = g_config.portal.hotkey1;
        savedPortalHotkey2 = g_config.portal.hotkey2;
        savedPortalHotkey0 = g_config.portal.hotkey0;
        savedBreadcrumbsHotkey = g_config.behaviors.breadCrumbs.hotkey;
    }

    void TearDown() override {
        g_config.behaviors.fun.ball = savedBall;
        g_config.behaviors.fun.breadCrumbs = savedBreadcrumbs;
        g_config.behaviors.fun.hats = savedHats;
        g_config.behaviors.fun.rainbow = savedRainbow;
        g_config.behaviors.fun.acid = savedAcid;
        g_config.behaviors.fun.anger = savedAnger;
        g_config.behaviors.fun.autumnLeaves = savedAutumnLeaves;
        g_config.behaviors.control.honcker = savedHoncker;
        g_config.behaviors.control.jail = savedJail;
        g_config.behaviors.control.portals = savedPortals;
        g_config.behaviors.control.drag = savedDrag;
        g_config.behaviors.info.nametag = savedNametag;
        g_config.behaviors.info.presence = savedPresence;
        g_config.behaviors.info.configGUI = savedConfigGUI;
        g_config.behaviors.info.visible = savedVisible;
        g_config.behaviors.systems.health = savedHealth;
        g_config.behaviors.systems.ai = savedAi;
        g_config.behaviors.systems.pomodoro = savedPomodoro;
        g_config.behaviors.honcker.hotkey = savedHonkerHotkey;
        g_config.behaviors.jail.hotkeyO = savedJailHotkeyO;
        g_config.behaviors.jail.hotkeyP = savedJailHotkeyP;
        g_config.portal.hotkey1 = savedPortalHotkey1;
        g_config.portal.hotkey2 = savedPortalHotkey2;
        g_config.portal.hotkey0 = savedPortalHotkey0;
        g_config.behaviors.breadCrumbs.hotkey = savedBreadcrumbsHotkey;
    }

private:
    bool savedBall, savedBreadcrumbs, savedHats, savedRainbow, savedAcid, savedAnger;
    bool savedAutumnLeaves;
    bool savedHoncker, savedJail, savedPortals, savedDrag;
    bool savedNametag, savedPresence, savedConfigGUI, savedVisible;
    bool savedHealth, savedAi, savedPomodoro;
    std::string savedHonkerHotkey, savedJailHotkeyO, savedJailHotkeyP;
    std::string savedPortalHotkey1, savedPortalHotkey2;
    std::string savedPortalHotkey0, savedBreadcrumbsHotkey;
};

TEST_F(MCPConfigTest, GetConfigReturnsAllSections) {
    std::string resp = MCP_CallTool("get_config", "{}");
    EXPECT_GT(resp.size(), 100u) << "Response should contain substantial config JSON";
    EXPECT_NE(resp.find("fun"), std::string::npos);
    EXPECT_NE(resp.find("control"), std::string::npos);
    EXPECT_NE(resp.find("info"), std::string::npos);
    EXPECT_NE(resp.find("systems"), std::string::npos);
    EXPECT_NE(resp.find("honcker_hotkey"), std::string::npos);
    EXPECT_NE(resp.find("breadcrumbs_hotkey"), std::string::npos);
    EXPECT_NE(resp.find("jail_hotkey_o"), std::string::npos);
    EXPECT_NE(resp.find("jail_hotkey_p"), std::string::npos);
    EXPECT_NE(resp.find("portal_hotkey_1"), std::string::npos);
    EXPECT_NE(resp.find("portal_hotkey_2"), std::string::npos);
    EXPECT_NE(resp.find("portal_hotkey_0"), std::string::npos);
}

TEST_F(MCPConfigTest, GetConfigWithKeyReturnsInfo) {
    std::string resp = MCP_CallTool("get_config", "{\"key\":\"behaviors.fun.ball\"}");
    EXPECT_NE(resp.find("config for"), std::string::npos);
}

TEST_F(MCPConfigTest, SetConfigAllBoolsRoundTrip) {
    struct BoolField { const char* key; bool* ptr; };
    BoolField fields[] = {
        {"behaviors.fun.ball", &g_config.behaviors.fun.ball},
        {"behaviors.fun.breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
        {"behaviors.fun.hats", &g_config.behaviors.fun.hats},
        {"behaviors.fun.rainbow", &g_config.behaviors.fun.rainbow},
        {"behaviors.fun.acid", &g_config.behaviors.fun.acid},
        {"behaviors.fun.anger", &g_config.behaviors.fun.anger},
        {"behaviors.control.honcker", &g_config.behaviors.control.honcker},
        {"behaviors.control.jail", &g_config.behaviors.control.jail},
        {"behaviors.control.portals", &g_config.behaviors.control.portals},
        {"behaviors.control.drag", &g_config.behaviors.control.drag},
        {"behaviors.info.nametag", &g_config.behaviors.info.nametag},
        {"behaviors.info.presence", &g_config.behaviors.info.presence},
        {"behaviors.info.configGUI", &g_config.behaviors.info.configGUI},
        {"behaviors.info.visible", &g_config.behaviors.info.visible},
        {"behaviors.systems.health", &g_config.behaviors.systems.health},
        {"behaviors.systems.ai", &g_config.behaviors.systems.ai},
        {"behaviors.systems.pomodoro", &g_config.behaviors.systems.pomodoro},
    };
    for (auto& f : fields) {
        bool orig = *f.ptr;
        std::string args = "{\"key\":\"" + std::string(f.key) + "\",\"value\":true}";
        std::string resp = MCP_CallTool("set_config", args);
        EXPECT_NE(resp.find("ok"), std::string::npos) << "set " << f.key;
        EXPECT_TRUE(*f.ptr) << "config bool " << f.key << " should be true";
        args = "{\"key\":\"" + std::string(f.key) + "\",\"value\":false}";
        resp = MCP_CallTool("set_config", args);
        EXPECT_NE(resp.find("ok"), std::string::npos) << "unset " << f.key;
        EXPECT_FALSE(*f.ptr) << "config bool " << f.key << " should be false";
        *f.ptr = orig;
    }
}

TEST_F(MCPConfigTest, SetConfigViaHandleRequest) {
    bool orig = g_config.behaviors.fun.ball;
    std::string req = "{\"jsonrpc\":\"2.0\",\"id\":5,"
        "\"method\":\"tools/call\","
        "\"params\":{\"name\":\"set_config\","
        "\"arguments\":{\"key\":\"behaviors.fun.ball\",\"value\":true}}}";
    std::string resp = MCP_HandleRequest(req);
    EXPECT_TRUE(g_config.behaviors.fun.ball);
    EXPECT_NE(resp.find("\"id\":5"), std::string::npos);
    EXPECT_NE(resp.find("ok"), std::string::npos);
    g_config.behaviors.fun.ball = orig;
}

TEST_F(MCPConfigTest, SetConfigUnknownKey) {
    std::string resp = MCP_CallTool("set_config",
        "{\"key\":\"nonexistent\",\"value\":true}");
    EXPECT_NE(resp.find("error"), std::string::npos);
    EXPECT_NE(resp.find("unknown"), std::string::npos);
}

TEST_F(MCPConfigTest, SetConfigMissingArgs) {
    std::string resp = MCP_CallTool("set_config", "{}");
    EXPECT_NE(resp.find("error"), std::string::npos);
}

TEST_F(MCPConfigTest, SetConfigMissingValue) {
    std::string resp = MCP_CallTool("set_config", "{\"key\":\"behaviors.fun.ball\"}");
    EXPECT_NE(resp.find("error"), std::string::npos);
}

TEST_F(MCPConfigTest, SetHotkeyAllFields) {
    struct HotkeyField { const char* name; std::string* ptr; };
    HotkeyField fields[] = {
        {"honcker_hotkey", &g_config.behaviors.honcker.hotkey},
        {"jail_hotkey_o", &g_config.behaviors.jail.hotkeyO},
        {"jail_hotkey_p", &g_config.behaviors.jail.hotkeyP},
        {"portal_hotkey_1", &g_config.portal.hotkey1},
        {"portal_hotkey_2", &g_config.portal.hotkey2},
        {"portal_hotkey_0", &g_config.portal.hotkey0},
        {"breadcrumbs_hotkey", &g_config.behaviors.breadCrumbs.hotkey},
    };
    for (auto& f : fields) {
        std::string orig = *f.ptr;
        std::string args = "{\"hotkey\":\"" + std::string(f.name) + "\",\"value\":\"x\"}";
        std::string resp = MCP_CallTool("set_hotkey", args);
        EXPECT_NE(resp.find("ok"), std::string::npos) << "set_hotkey " << f.name;
        EXPECT_EQ(*f.ptr, "x") << "hotkey " << f.name << " should be \"x\"";
        *f.ptr = orig;
    }
}

TEST_F(MCPConfigTest, SetHotkeyUnknownField) {
    std::string resp = MCP_CallTool("set_hotkey",
        "{\"hotkey\":\"nonexistent\",\"value\":\"x\"}");
    EXPECT_NE(resp.find("error"), std::string::npos);
    EXPECT_NE(resp.find("unknown"), std::string::npos);
}

TEST_F(MCPConfigTest, SetHotkeyMissingArgs) {
    std::string resp = MCP_CallTool("set_hotkey", "{}");
    EXPECT_NE(resp.find("error"), std::string::npos);
}

TEST_F(MCPConfigTest, SetHotkeyMissingValue) {
    std::string resp = MCP_CallTool("set_hotkey",
        "{\"hotkey\":\"honcker_hotkey\"}");
    EXPECT_NE(resp.find("error"), std::string::npos);
}

TEST_F(MCPConfigTest, SetConfigTogglesReflectedInGetConfig) {
    bool origBall = g_config.behaviors.fun.ball;
    bool origHoncker = g_config.behaviors.control.honcker;

    g_config.behaviors.fun.ball = true;
    g_config.behaviors.control.honcker = false;
    std::string resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find(R"(\"ball\":true)"), std::string::npos);

    g_config.behaviors.fun.ball = false;
    g_config.behaviors.control.honcker = true;
    resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find(R"(\"honcker\":true)"), std::string::npos);

    g_config.behaviors.fun.ball = origBall;
    g_config.behaviors.control.honcker = origHoncker;
}

TEST_F(MCPConfigTest, SetHotkeyReflectedInGetConfig) {
    std::string orig = g_config.behaviors.honcker.hotkey;
    g_config.behaviors.honcker.hotkey = "cmd+shift+h";
    std::string resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find(R"(cmd+shift+h)"), std::string::npos);
    g_config.behaviors.honcker.hotkey = orig;
}

TEST_F(MCPConfigTest, RepeatedSetConfigStress) {
    std::string key = "behaviors.fun.ball";
    bool orig = g_config.behaviors.fun.ball;
    for (int i = 0; i < 10; i++) {
        MCP_CallTool("set_config",
            "{\"key\":\"" + key + "\",\"value\":true}");
        EXPECT_TRUE(g_config.behaviors.fun.ball);
        MCP_CallTool("set_config",
            "{\"key\":\"" + key + "\",\"value\":false}");
        EXPECT_FALSE(g_config.behaviors.fun.ball);
    }
    g_config.behaviors.fun.ball = orig;
}
