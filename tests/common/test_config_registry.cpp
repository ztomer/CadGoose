#include <gtest/gtest.h>
#include <string>
#include <chrono>
#include "config.h"
#include "toml.hpp"
#include "../../src/common/config_helpers.h"

TEST(ConfigRegistry, FindOptionByKey) {
    Config_Init();
    ASSERT_FALSE(g_configRegistry.empty());

    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    ASSERT_NE(opt, nullptr) << "Could not find global_scale";
    EXPECT_STREQ(opt->key, "global_scale");
    EXPECT_STREQ(opt->section, "General");
    EXPECT_EQ(opt->type, CFG_FLOAT);

    const ConfigOption* notFound = Config_FindOptionByKey("nonexistent_key");
    EXPECT_EQ(notFound, nullptr);
}

TEST(ConfigRegistry, CaseInsensitiveLookup) {
    Config_Init();
    EXPECT_NE(Config_FindOptionByKey("global_scale"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("Global_Scale"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
}

TEST(ConfigRegistry, AllEntriesHaveValidTypes) {
    Config_Init();
    for (const auto& opt : g_configRegistry) {
        EXPECT_GE(opt.type, CFG_BOOL) << "Invalid type for key: " << opt.key;
        EXPECT_LE(opt.type, CFG_STRING) << "Invalid type for key: " << opt.key;
    }
}

TEST(ConfigRegistry, AllFloatEntriesHaveValidPtr) {
    Config_Init();
    for (const auto& opt : g_configRegistry) {
        if (opt.type == CFG_FLOAT) {
            EXPECT_NE(opt.ptr, nullptr) << "Null ptr for key: " << opt.key;
        }
    }
}

TEST(ConfigRegistry, O1LookupPerformance) {
    Config_Init();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        Config_FindOptionByKey("global_scale");
        Config_FindOptionByKey("default_width");
        Config_FindOptionByKey("base_walk_speed");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(duration, 50);
}
