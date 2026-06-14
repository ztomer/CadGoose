#include <gtest/gtest.h>
#include <string>
#include <cstring>
#import <Foundation/Foundation.h>
#include "behaviors/ai_prompt_builder.h"
#include "behaviors/ai_think_block_stripper.h"
#include "behaviors/ai_model_profiles.h"
#include "config.h"
#include "config_helpers.h"

@interface AIHelpersTest : NSObject
@end

@implementation AIHelpersTest
@end

TEST(AIHelpers, CapEvilForFoundationClampsHighValues) {
    EXPECT_FLOAT_EQ(CapEvilForFoundation(1.0f), 0.72f);
    EXPECT_FLOAT_EQ(CapEvilForFoundation(0.9f), 0.72f);
}

TEST(AIHelpers, CapEvilForFoundationPassesLowValues) {
    EXPECT_FLOAT_EQ(CapEvilForFoundation(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(CapEvilForFoundation(0.0f), 0.0f);
}

TEST(AIHelpers, FoundationPersonaCapNoteNonEmpty) {
    NSString* note = FoundationPersonaCapNote();
    EXPECT_GT(note.length, 0u);
    EXPECT_TRUE([note containsString:@"72"]);
}

TEST(AIHelpers, StripThinkBlocksRemovesThinkBlock) {
    NSString* result = stripThinkBlocks(@"Hello<think>remove</think>world");
    EXPECT_TRUE([result isEqualToString:@"Helloworld"]) << [result UTF8String];
}

TEST(AIHelpers, StripThinkBlocksHandlesNoThinkBlock) {
    NSString* input = @"Hello world";
    NSString* result = stripThinkBlocks(input);
    EXPECT_TRUE([result isEqualToString:input]);
}

TEST(AIHelpers, StripThinkBlocksHandlesEmptyString) {
    NSString* input = @"";
    NSString* result = stripThinkBlocks(input);
    EXPECT_TRUE([result isEqualToString:input]);
}

TEST(AIHelpers, StripThinkBlocksHandlesNil) {
    NSString* result = stripThinkBlocks(nil);
    EXPECT_EQ(result, nil);
}

TEST(AIHelpers, StripThinkBlocksHandlesOnlyThinkBlock) {
    NSString* result = stripThinkBlocks(@"<think>thinking text</think>");
    EXPECT_TRUE([result isEqualToString:@"<think>thinking text</think>"]);
}

TEST(AIHelpers, StripThinkBlocksHandlesMultipleThinkBlocks) {
    NSString* result = stripThinkBlocks(@"a<think>1</think>b<think>2</think>c");
    EXPECT_TRUE([result isEqualToString:@"abc"]);
}

TEST(AIHelpers, StripThinkBlocksHandlesNewlines) {
    NSString* result = stripThinkBlocks(@"Hello<think>remove\nthis</think>world");
    EXPECT_TRUE([result isEqualToString:@"Helloworld"]) << [result UTF8String];
}

TEST(AIHelpers, MatchProfileReturnsDefaultForNull) {
    const BuiltinProfile* p = MatchProfile(nullptr);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->pattern, nullptr);
}

TEST(AIHelpers, MatchProfileMatchesLlamaPrefix) {
    const BuiltinProfile* p = MatchProfile("llama-3.2-3b");
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->pattern, "llama*");
    EXPECT_FLOAT_EQ(p->temperature, 0.7f);
}

TEST(AIHelpers, MatchProfileMatchesQwenPrefix) {
    const BuiltinProfile* p = MatchProfile("qwen2.5-coder-7b");
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->pattern, "qwen*");
    EXPECT_FLOAT_EQ(p->temperature, 0.9f);
}

TEST(AIHelpers, MatchProfileReturnsDefaultForUnknown) {
    const BuiltinProfile* p = MatchProfile("unknown-model-v1");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->pattern, nullptr);
    EXPECT_FLOAT_EQ(p->temperature, 0.8f);
}

TEST(AIHelpers, MatchProfileMatchesFoundationExact) {
    const BuiltinProfile* p = MatchProfile("foundation-models-1");
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->pattern, "foundation*");
    EXPECT_EQ(p->hasReasoningContent, false);
}

TEST(AIHelpers, MatchProfileMatchesExactModelName) {
    const BuiltinProfile* p = MatchProfile("gemma-2-2b");
    ASSERT_NE(p, nullptr);
    EXPECT_STREQ(p->pattern, "gemma*");
}

TEST(AIHelpers, SystemPromptForEvilLevelContainsPersonality) {
    Config_Init();
    NSString* prompt = systemPromptForEvilLevel(0.5f);
    EXPECT_TRUE([prompt length] > 0u);
    EXPECT_TRUE([prompt containsString:@"You are"]);
}

TEST(AIHelpers, SystemPromptForEvilLevelHighValue) {
    Config_Init();
    NSString* prompt = systemPromptForEvilLevel(0.9f);
    EXPECT_TRUE([prompt length] > 0u);
    EXPECT_TRUE([prompt containsString:@"You are"]);
}

TEST(AIHelpers, SystemPromptStalinModeReplacesHonkWithGulag) {
    Config_Init();
    int saved = g_config.general.appearanceMode;
    g_config.general.appearanceMode = APPEARANCE_STALIN;
    NSString* prompt = systemPromptForEvilLevel(0.5f);
    EXPECT_TRUE([prompt containsString:@"GULAG"]) << [prompt UTF8String];
    EXPECT_TRUE([prompt containsString:@"Comrade"]) << [prompt UTF8String];
    EXPECT_FALSE([prompt containsString:@"Honk Goose"]) << [prompt UTF8String];
    g_config.general.appearanceMode = saved;
}
