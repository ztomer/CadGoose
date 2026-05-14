#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "ai_mcp_bridge.h"

TEST(AITokenize, EmptyString) {
    auto tokens = AI_TokenizeMessage("");
    EXPECT_TRUE(tokens.empty());
}

TEST(AITokenize, SingleWord) {
    auto tokens = AI_TokenizeMessage("hello");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "hello");
}

TEST(AITokenize, Lowercases) {
    auto tokens = AI_TokenizeMessage("HELLO World");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
}

TEST(AITokenize, StripsPunctuation) {
    auto tokens = AI_TokenizeMessage("enable ball! please?");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "enable");
    EXPECT_EQ(tokens[1], "ball");
    EXPECT_EQ(tokens[2], "please");
}

TEST(AITokenize, StripsLeadingPunctuation) {
    auto tokens = AI_TokenizeMessage("!honk");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "honk");
}

TEST(AITokenize, MultipleSpaces) {
    auto tokens = AI_TokenizeMessage("enable    ball");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0], "enable");
    EXPECT_EQ(tokens[1], "ball");
}

TEST(AITokenize, PunctuationOnly) {
    auto tokens = AI_TokenizeMessage("!@#$%");
    EXPECT_TRUE(tokens.empty());
}

TEST(AITokenize, PreservesUnderscore) {
    auto tokens = AI_TokenizeMessage("set_config");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "set_config");
}

TEST(AITokenize, PreservesHyphen) {
    auto tokens = AI_TokenizeMessage("right-shift");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "right-shift");
}

TEST(AITokenize, MixedPunctuation) {
    auto tokens = AI_TokenizeMessage("  hello, world!! how are you?  ");
    ASSERT_EQ(tokens.size(), 5u);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "how");
    EXPECT_EQ(tokens[3], "are");
    EXPECT_EQ(tokens[4], "you");
}

TEST(AIMatch, ExactMatch) {
    auto tokens = AI_TokenizeMessage("enable ball");
    EXPECT_TRUE(AI_MatchTokens(tokens, {"enable"}));
    EXPECT_FALSE(AI_MatchTokens(tokens, {"disable"}));
}

TEST(AIMatch, ExactFullMatch) {
    auto tokens = AI_TokenizeMessage("turn on the ball");
    EXPECT_TRUE(AI_MatchTokens(tokens, {"turn", "on"}));
    EXPECT_FALSE(AI_MatchTokens(tokens, {"turn", "off"}));
}

TEST(AIMatch, TooFewTokens) {
    auto tokens = AI_TokenizeMessage("honk");
    EXPECT_FALSE(AI_MatchTokens(tokens, {"enable", "ball"}));
}

TEST(AIMatch, EmptyPattern) {
    auto tokens = AI_TokenizeMessage("anything");
    EXPECT_TRUE(AI_MatchTokens(tokens, {}));
}

TEST(AIMatch, EmptyTokens) {
    std::vector<std::string> empty;
    EXPECT_TRUE(AI_MatchTokens(empty, {}));
    EXPECT_FALSE(AI_MatchTokens(empty, {"anything"}));
}

TEST(AIMatch, CaseInsensitiveMatch) {
    auto tokens = AI_TokenizeMessage("ENABLE BALL");
    EXPECT_TRUE(AI_MatchTokens(tokens, {"enable"}));
}
