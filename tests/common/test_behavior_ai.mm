// ===========================
// test_behavior_ai.mm
// Unit tests for AI behavior Foundation fallback
// ===========================
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

#import <Cocoa/Cocoa.h>

#pragma mark - Keyword Extraction (C++ replica of Foundation fallback logic)

struct KeywordMatcher {
    std::set<std::string> keywords;

    void extract(const std::string& message) {
        keywords.clear();
        if (message.empty()) return;

        @autoreleasepool {
            NSString* msg = [NSString stringWithUTF8String:message.c_str()];
            NSLinguisticTagger* tagger = [[NSLinguisticTagger alloc] initWithTagSchemes:@[NSLinguisticTagSchemeLexicalClass] options:0];
            [tagger setString:msg];

            [tagger enumerateTagsInRange:NSMakeRange(0, msg.length)
                                  scheme:NSLinguisticTagSchemeLexicalClass
                                 options:NSLinguisticTaggerOmitWhitespace | NSLinguisticTaggerOmitPunctuation | NSLinguisticTaggerOmitOther
                              usingBlock:^(NSString* _Nullable tag, NSRange tokenRange, NSRange sentenceRange, BOOL* _Nonnull stop) {
                NSString* word = [[msg substringWithRange:tokenRange] lowercaseString];
                if ([tag isEqualToString:NSLinguisticTagNoun] || [tag isEqualToString:NSLinguisticTagVerb] || [tag isEqualToString:NSLinguisticTagAdjective] || [tag isEqualToString:NSLinguisticTagAdverb] || [tag isEqualToString:NSLinguisticTagPronoun] || [tag isEqualToString:NSLinguisticTagInterjection] || [tag isEqualToString:@"OtherWord"]) {
                    keywords.insert(std::string([word UTF8String]));
                }
            }];
        }
    }

    bool contains(const std::string& word) const {
        return keywords.find(word) != keywords.end();
    }

    bool intersects(const std::vector<std::string>& words) const {
        for (const auto& w : words) {
            if (keywords.find(w) != keywords.end()) return true;
        }
        return false;
    }
};

static std::string select_response(const KeywordMatcher& km) {
    if (km.contains("honk")) return "honk_response";
    if (km.contains("sad") || km.contains("mad") || km.contains("angry")) return "sad_response";
    if (km.contains("happy") || km.contains("love") || km.contains("great")) return "happy_response";
    if (km.intersects({"food", "bread", "seed", "feed", "eat"})) return "food_response";
    if (km.contains("name")) return "name_response";
    if (km.contains("goose")) return "goose_response";
    if (km.contains("bye") || km.contains("goodbye")) return "bye_response";
    if (km.contains("hello") || km.contains("hi") || km.contains("hey")) return "hello_response";
    if (km.contains("help") || km.contains("what")) return "help_response";
    if (km.contains("dance") || km.contains("spin")) return "dance_response";
    if (km.contains("sing") || km.contains("song")) return "song_response";
    return "default_response";
}

#pragma mark - Tests

TEST(AIChatFallback, HonkKeyword) {
    KeywordMatcher km;
    km.extract("honk honk goose");
    EXPECT_TRUE(km.contains("honk"));
    EXPECT_EQ(select_response(km), "honk_response");
}

TEST(AIChatFallback, SadKeyword) {
    KeywordMatcher km;
    km.extract("I am so sad today");
    EXPECT_TRUE(km.contains("sad"));
    EXPECT_EQ(select_response(km), "sad_response");
}

TEST(AIChatFallback, MadKeyword) {
    KeywordMatcher km;
    km.extract("this makes me mad");
    EXPECT_TRUE(km.contains("mad"));
    EXPECT_EQ(select_response(km), "sad_response");
}

TEST(AIChatFallback, AngryKeyword) {
    KeywordMatcher km;
    km.extract("I am very angry");
    EXPECT_TRUE(km.contains("angry"));
    EXPECT_EQ(select_response(km), "sad_response");
}

TEST(AIChatFallback, HappyKeyword) {
    KeywordMatcher km;
    km.extract("I feel happy today");
    EXPECT_TRUE(km.contains("happy"));
    EXPECT_EQ(select_response(km), "happy_response");
}

TEST(AIChatFallback, LoveKeyword) {
    KeywordMatcher km;
    km.extract("I love this goose");
    EXPECT_TRUE(km.contains("love"));
    EXPECT_EQ(select_response(km), "happy_response");
}

TEST(AIChatFallback, FoodKeyword) {
    KeywordMatcher km;
    km.extract("give me food please");
    EXPECT_TRUE(km.contains("food"));
    EXPECT_TRUE(km.intersects({"food", "bread", "seed", "feed", "eat"}));
    EXPECT_EQ(select_response(km), "food_response");
}

TEST(AIChatFallback, BreadKeyword) {
    KeywordMatcher km;
    km.extract("want some bread");
    EXPECT_TRUE(km.intersects({"food", "bread"}));
    EXPECT_EQ(select_response(km), "food_response");
}

TEST(AIChatFallback, EatKeyword) {
    KeywordMatcher km;
    km.extract("I want to eat");
    EXPECT_TRUE(km.contains("eat"));
    EXPECT_EQ(select_response(km), "food_response");
}

TEST(AIChatFallback, NameKeyword) {
    KeywordMatcher km;
    km.extract("what is your name");
    EXPECT_TRUE(km.contains("name"));
    EXPECT_EQ(select_response(km), "name_response");
}

TEST(AIChatFallback, GooseKeyword) {
    KeywordMatcher km;
    km.extract("you are a goose");
    EXPECT_TRUE(km.contains("goose"));
    EXPECT_EQ(select_response(km), "goose_response");
}

TEST(AIChatFallback, ByeKeyword) {
    KeywordMatcher km;
    km.extract("bye now");
    EXPECT_TRUE(km.contains("bye"));
    EXPECT_EQ(select_response(km), "bye_response");
}

TEST(AIChatFallback, GoodbyeKeyword) {
    KeywordMatcher km;
    km.extract("goodbye see you later");
    EXPECT_TRUE(km.contains("goodbye"));
    EXPECT_EQ(select_response(km), "bye_response");
}

TEST(AIChatFallback, HelloKeyword) {
    KeywordMatcher km;
    km.extract("hello there");
    EXPECT_TRUE(km.contains("hello"));
    EXPECT_EQ(select_response(km), "hello_response");
}

TEST(AIChatFallback, HiKeyword) {
    KeywordMatcher km;
    km.extract("hi there");
    EXPECT_TRUE(km.contains("hi"));
    EXPECT_EQ(select_response(km), "hello_response");
}

TEST(AIChatFallback, HeyKeyword) {
    KeywordMatcher km;
    km.extract("hey what's up");
    EXPECT_TRUE(km.contains("hey"));
    EXPECT_EQ(select_response(km), "hello_response");
}

TEST(AIChatFallback, HelpKeyword) {
    KeywordMatcher km;
    km.extract("help me understand");
    EXPECT_TRUE(km.contains("help"));
    EXPECT_EQ(select_response(km), "help_response");
}

TEST(AIChatFallback, WhatKeyword) {
    KeywordMatcher km;
    km.extract("what is this");
    EXPECT_TRUE(km.contains("what"));
    EXPECT_EQ(select_response(km), "help_response");
}

TEST(AIChatFallback, DanceKeyword) {
    KeywordMatcher km;
    km.extract("do a dance");
    EXPECT_TRUE(km.contains("dance"));
    EXPECT_EQ(select_response(km), "dance_response");
}

TEST(AIChatFallback, SpinKeyword) {
    KeywordMatcher km;
    km.extract("spin around");
    EXPECT_TRUE(km.contains("spin"));
    EXPECT_EQ(select_response(km), "dance_response");
}

TEST(AIChatFallback, SingKeyword) {
    KeywordMatcher km;
    km.extract("sing me a song");
    EXPECT_TRUE(km.contains("sing"));
    EXPECT_EQ(select_response(km), "song_response");
}

TEST(AIChatFallback, SongKeyword) {
    KeywordMatcher km;
    km.extract("sing a song for me");
    EXPECT_TRUE(km.contains("song"));
    EXPECT_EQ(select_response(km), "song_response");
}

TEST(AIChatFallback, DefaultResponse) {
    KeywordMatcher km;
    km.extract("the sky is blue");
    EXPECT_FALSE(km.contains("honk"));
    EXPECT_FALSE(km.contains("sad"));
    EXPECT_EQ(select_response(km), "default_response");
}

TEST(AIChatFallback, HonkTakesPriority) {
    KeywordMatcher km;
    km.extract("honk I am sad");
    EXPECT_TRUE(km.contains("honk"));
    EXPECT_TRUE(km.contains("sad"));
    EXPECT_EQ(select_response(km), "honk_response") << "honk should match before sad";
}

TEST(AIChatFallback, FoodTakesPriorityOverName) {
    KeywordMatcher km;
    km.extract("food name");
    EXPECT_TRUE(km.contains("food"));
    EXPECT_EQ(select_response(km), "food_response") << "food should match before name";
}

TEST(AIChatFallback, EmptyMessage) {
    KeywordMatcher km;
    km.extract("");
    EXPECT_TRUE(km.keywords.empty());
    EXPECT_EQ(select_response(km), "default_response");
}

TEST(AIChatFallback, CaseInsensitive) {
    KeywordMatcher km;
    km.extract("HONK HONK HONK");
    EXPECT_TRUE(km.contains("honk")) << "Should be case insensitive";
    EXPECT_EQ(select_response(km), "honk_response");
}

TEST(AIChatFallback, MultipleKeywords) {
    KeywordMatcher km;
    km.extract("hello happy goose dance");
    EXPECT_TRUE(km.contains("hello"));
    EXPECT_TRUE(km.contains("happy"));
    EXPECT_TRUE(km.contains("goose"));
    EXPECT_TRUE(km.contains("dance"));
    EXPECT_EQ(select_response(km), "happy_response") << "happy takes priority by position in if-chain";
}

TEST(AIChatFallback, NumericInput) {
    KeywordMatcher km;
    km.extract("12345");
    EXPECT_EQ(select_response(km), "default_response");
}

TEST(AIChatFallback, PunctuationInput) {
    KeywordMatcher km;
    km.extract("!!! ??? ...");
    EXPECT_TRUE(km.keywords.empty());
    EXPECT_EQ(select_response(km), "default_response");
}

TEST(AIChatFallback, LongInput) {
    KeywordMatcher km;
    km.extract("I have been thinking about this for a long time and I really want to know what the goose thinks about bread and food in general");
    EXPECT_TRUE(km.intersects({"food", "bread"}));
    EXPECT_EQ(select_response(km), "food_response");
}

TEST(AIChatFallback, IntersectsHelper) {
    KeywordMatcher km;
    km.extract("feed me");
    std::vector<std::string> foodWords = {"food", "bread", "seed", "feed", "eat"};
    EXPECT_TRUE(km.intersects(foodWords));
}

TEST(AIChatFallback, NoFalsePositive) {
    KeywordMatcher km;
    km.extract("what is a good name for a cat");
    EXPECT_TRUE(km.contains("name"));
    EXPECT_TRUE(km.contains("cat"));
    EXPECT_FALSE(km.contains("food"));
    EXPECT_FALSE(km.contains("goose"));
    EXPECT_EQ(select_response(km), "name_response");
}
