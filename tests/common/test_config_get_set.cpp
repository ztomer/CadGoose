#include <gtest/gtest.h>
#include <string>
#include "config.h"
#include "toml.hpp"
#include "../../src/common/config_helpers.h"

TEST(ConfigGetSet, GetValueByKey_Bool) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (opt && opt->type == CFG_BOOL) {
        std::string value;
        bool result = Config_GetValueByKey("audio_enabled", &value);
        EXPECT_TRUE(result);
        EXPECT_TRUE(value == "0" || value == "1");
    }
}

TEST(ConfigGetSet, GetValueByKey_Int) {
    Config_Init();
    std::string value;
    bool result = Config_GetValueByKey("default_width", &value);
    EXPECT_TRUE(result);
    int parsed = std::stoi(value);
    EXPECT_GE(parsed, 0);
}

TEST(ConfigGetSet, GetValueByKey_Float) {
    Config_Init();
    std::string value;
    bool result = Config_GetValueByKey("global_scale", &value);
    EXPECT_TRUE(result);
    float parsed = std::stof(value);
    EXPECT_GT(parsed, 0.0f);
}

TEST(ConfigGetSet, GetValueByKey_Unknown) {
    std::string value;
    std::string error;
    bool result = Config_GetValueByKey("nonexistent.key", &value, &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
}

TEST(ConfigGetSet, SetValueByKey_Bool_True) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (!opt || opt->type != CFG_BOOL) return;

    *(bool*)opt->ptr = false;
    bool original = *(bool*)opt->ptr;
    bool result = Config_SetValueByKey("audio_enabled", "1");
    EXPECT_TRUE(result);
    EXPECT_NE(*(bool*)opt->ptr, original);

    result = Config_SetValueByKey("audio_enabled", "true");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "yes");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "on");
    EXPECT_TRUE(result);
}

TEST(ConfigGetSet, SetValueByKey_Bool_False) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (!opt || opt->type != CFG_BOOL) return;

    bool result = Config_SetValueByKey("audio_enabled", "0");
    EXPECT_TRUE(result);
    EXPECT_FALSE(*(bool*)opt->ptr);

    result = Config_SetValueByKey("audio_enabled", "false");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "no");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "off");
    EXPECT_TRUE(result);
}

TEST(ConfigGetSet, SetValueByKey_Int) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    bool result = Config_SetValueByKey("default_width", "60");
    EXPECT_TRUE(result);
    EXPECT_EQ(*(int*)opt->ptr, 60);
}

TEST(ConfigGetSet, SetValueByKey_Float) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    bool result = Config_SetValueByKey("global_scale", "1.5");
    EXPECT_TRUE(result);
    EXPECT_NEAR(*(float*)opt->ptr, 1.5f, 0.01f);
}

TEST(ConfigGetSet, SetValueByKey_Unknown) {
    std::string error;
    bool result = Config_SetValueByKey("nonexistent.key", "value", &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
}

TEST(ConfigGetSet, OnConfigChange_Callback) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;
    EXPECT_TRUE(opt->onChange != nullptr);

    float original = *(float*)opt->ptr;
    Config_SetValueByKey("global_scale", "99.0");
    EXPECT_NE(*(float*)opt->ptr, original);
}

TEST(ConfigGetSet, ClampIntToMax) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    int original = *(int*)opt->ptr;
    bool result = Config_SetValueByKey("default_width", "999999");
    EXPECT_TRUE(result);
    EXPECT_LE(*(int*)opt->ptr, static_cast<int>(opt->max));

    *(int*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampIntToMin) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    int original = *(int*)opt->ptr;
    bool result = Config_SetValueByKey("default_width", "-999999");
    EXPECT_TRUE(result);
    EXPECT_GE(*(int*)opt->ptr, static_cast<int>(opt->min));

    *(int*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampFloatToMax) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    bool result = Config_SetValueByKey("global_scale", "999.0");
    EXPECT_TRUE(result);
    EXPECT_LE(*(float*)opt->ptr, opt->max);

    *(float*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampFloatToMin) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    bool result = Config_SetValueByKey("global_scale", "-999.0");
    EXPECT_TRUE(result);
    EXPECT_GE(*(float*)opt->ptr, opt->min);

    *(float*)opt->ptr = original;
}

TEST(ConfigEdgeCase, SetInvalidBoolValue) {
    Config_Init();
    auto opt = Config_FindOptionByKey("audio_enabled");
    if (!opt || opt->type != CFG_BOOL) return;

    float original = *(bool*)opt->ptr;
    std::string error;
    bool result = Config_SetValueByKey("audio_enabled", "not_a_bool", &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    *(bool*)opt->ptr = original;
}

TEST(ConfigEdgeCase, SetInvalidIntValue) {
    Config_Init();
    auto opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    float original = *(int*)opt->ptr;
    std::string error;
    bool result = Config_SetValueByKey("default_width", "abc", &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    *(int*)opt->ptr = (int)original;
}

TEST(ConfigEdgeCase, SetInvalidFloatValue) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    std::string error;
    bool result = Config_SetValueByKey("global_scale", "xyz", &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
    *(float*)opt->ptr = original;
}

TEST(ConfigEdgeCase, ClampFloatToBounds) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;

    Config_SetValueByKey("global_scale", "-5.0");
    EXPECT_FLOAT_EQ(*(float*)opt->ptr, opt->min);

    Config_SetValueByKey("global_scale", "999.0");
    EXPECT_FLOAT_EQ(*(float*)opt->ptr, opt->max);

    *(float*)opt->ptr = original;
}

TEST(ConfigEdgeCase, ClampIntToBounds) {
    Config_Init();
    auto opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    int original = *(int*)opt->ptr;

    Config_SetValueByKey("default_width", "-999999");
    EXPECT_EQ(*(int*)opt->ptr, (int)opt->min);

    Config_SetValueByKey("default_width", "999999");
    EXPECT_EQ(*(int*)opt->ptr, (int)opt->max);

    *(int*)opt->ptr = original;
}
