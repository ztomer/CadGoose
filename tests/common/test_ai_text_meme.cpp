#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "ai_text_meme.h"
#include "config.h"

class AITextMemeTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        AI_TextMeme_Reset();
        g_config.ai.textMemeEnabled = true;
        g_config.ai.textMemeMaxQueue = 5;
    }
    void TearDown() override {
        AI_TextMeme_Reset();
        g_config.ai.textMemeEnabled = false;
    }
};

TEST_F(AITextMemeTest, QueueEmptyInitially) {
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, InjectAndDequeue) {
    AI_TextMeme_Inject("Hello from the goose");
    EXPECT_TRUE(AI_TextMeme_HasAvailable());
    EXPECT_EQ(AI_TextMeme_QueueSize(), 1);

    std::string text = AI_TextMeme_Dequeue();
    EXPECT_EQ(text, "Hello from the goose");
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, DedupSameText) {
    AI_TextMeme_Inject("honk honk");
    AI_TextMeme_Inject("honk honk");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 1);

    std::string text = AI_TextMeme_Dequeue();
    EXPECT_EQ(text, "honk honk");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, MultipleDistinctTexts) {
    AI_TextMeme_Inject("first");
    AI_TextMeme_Inject("second");
    AI_TextMeme_Inject("third");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 3);

    EXPECT_EQ(AI_TextMeme_Dequeue(), "first");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "second");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "third");
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
}

TEST_F(AITextMemeTest, ResetClearsQueue) {
    AI_TextMeme_Inject("alpha");
    AI_TextMeme_Inject("beta");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 2);

    AI_TextMeme_Reset();
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, ResetClearsSeenHashSlot) {
    AI_TextMeme_Inject("unique text");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 1);
    AI_TextMeme_Dequeue();
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);

    AI_TextMeme_Reset();
    AI_TextMeme_Inject("unique text");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 1);
}

TEST_F(AITextMemeTest, InjectBypassesMaxQueue) {
    g_config.ai.textMemeMaxQueue = 3;
    AI_TextMeme_Inject("a");
    AI_TextMeme_Inject("b");
    AI_TextMeme_Inject("c");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 3);

    AI_TextMeme_Inject("d");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 4);
}

TEST_F(AITextMemeTest, DequeueReturnsEmptyWhenEmpty) {
    std::string text = AI_TextMeme_Dequeue();
    EXPECT_TRUE(text.empty());
}

TEST_F(AITextMemeTest, TickDoesNothingWhenDisabled) {
    g_config.ai.textMemeEnabled = false;
    AI_TextMeme_Tick(100.0);
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, DedupPersistsAfterDequeue) {
    AI_TextMeme_Inject("first text");
    AI_TextMeme_Dequeue();
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);

    AI_TextMeme_Inject("first text");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
}

TEST_F(AITextMemeTest, SeenHashRingBufferEvictsOldHashes) {
    for (int i = 0; i < 600; i++) {
        AI_TextMeme_Inject("text_" + std::to_string(i));
        AI_TextMeme_Dequeue();
    }
    EXPECT_TRUE(AI_TextMeme_HasAvailable() || AI_TextMeme_QueueSize() == 0);
}

TEST_F(AITextMemeTest, TickDoesNotExceedMaxQueueFromInject) {
    AI_TextMeme_Inject("one");
    AI_TextMeme_Inject("two");
    AI_TextMeme_Inject("three");
    AI_TextMeme_Inject("four");
    AI_TextMeme_Inject("five");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 5);
}

TEST_F(AITextMemeTest, DequeuePrefersAiOverFile) {
    AI_TextMeme_Inject("ai text");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 1);
    std::string text = AI_TextMeme_Dequeue();
    EXPECT_EQ(text, "ai text");
}

TEST_F(AITextMemeTest, HasAvailableWhenAnyPoolNonEmpty) {
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
    AI_TextMeme_Inject("test");
    EXPECT_TRUE(AI_TextMeme_HasAvailable());
    AI_TextMeme_Dequeue();
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
}

TEST_F(AITextMemeTest, ResetClearsBothPools) {
    AI_TextMeme_Inject("a");
    AI_TextMeme_Inject("b");
    EXPECT_EQ(AI_TextMeme_QueueSize(), 2);
    AI_TextMeme_Reset();
    EXPECT_EQ(AI_TextMeme_QueueSize(), 0);
    EXPECT_FALSE(AI_TextMeme_HasAvailable());
}

TEST_F(AITextMemeTest, DequeueReturnsAiFirst) {
    AI_TextMeme_Inject("ai1");
    AI_TextMeme_Inject("ai2");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "ai1");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "ai2");
}

TEST_F(AITextMemeTest, MultipleAiTextsInOrder) {
    AI_TextMeme_Inject("first");
    AI_TextMeme_Inject("second");
    AI_TextMeme_Inject("third");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "first");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "second");
    EXPECT_EQ(AI_TextMeme_Dequeue(), "third");
}
