#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include <thread>

#include "local_llm.h"
#include "config.h"

class LocalLLMTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        LocalLLM_Shutdown();
    }
    void TearDown() override {
        LocalLLM_Shutdown();
    }
};

TEST_F(LocalLLMTest, StateIsUnavailableInitially) {
    EXPECT_EQ(LocalLLM_GetState(), LocalLLMState::Unavailable);
}

TEST_F(LocalLLMTest, QueueEmptyInitially) {
    EXPECT_EQ(LocalLLM_QueueSize(), 0);
    EXPECT_TRUE(LocalLLM_Dequeue().empty());
}

TEST_F(LocalLLMTest, GenerateWithoutModelReturnsEmpty) {
    if (FoundationLLM_IsAvailable()) {
        GTEST_SKIP() << "FoundationModels available — generation succeeds without CoreML model";
    }
    std::promise<std::string> result;
    auto fut = result.get_future();
    LocalLLM_Generate("hello goose", 1.0f, [&](const std::string& text) {
        result.set_value(text);
    });
    auto status = fut.wait_for(std::chrono::seconds(5));
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_TRUE(fut.get().empty());
}

TEST_F(LocalLLMTest, GenerateWhileLoadingReturnsEmpty) {
    if (FoundationLLM_IsAvailable()) {
        GTEST_SKIP() << "FoundationModels available — generation succeeds immediately";
    }
    LocalLLM_Init();

    std::promise<std::string> result;
    auto fut = result.get_future();
    LocalLLM_Generate("test", 1.0f, [&](const std::string& text) {
        result.set_value(text);
    });
    auto status = fut.wait_for(std::chrono::seconds(1));
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_TRUE(fut.get().empty());
}

TEST_F(LocalLLMTest, InitEventuallyLeavesLoadingState) {
    LocalLLM_Init();
    auto start = std::chrono::steady_clock::now();
    while (LocalLLM_GetState() == LocalLLMState::Loading) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        ASSERT_LT(elapsed, std::chrono::seconds(10)) << "Init timed out";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    auto state = LocalLLM_GetState();
    EXPECT_TRUE(state == LocalLLMState::Unavailable || state == LocalLLMState::Ready);
}

TEST_F(LocalLLMTest, ShutdownResetsState) {
    LocalLLM_Init();
    LocalLLM_Shutdown();
    EXPECT_EQ(LocalLLM_GetState(), LocalLLMState::Unavailable);
}

TEST_F(LocalLLMTest, ShutdownClearsQueue) {
    LocalLLM_Shutdown();
    EXPECT_EQ(LocalLLM_QueueSize(), 0);
    EXPECT_TRUE(LocalLLM_Dequeue().empty());
}

TEST_F(LocalLLMTest, DownloadEmptyUrlReturnsError) {
    std::promise<std::pair<bool, std::string>> result;
    auto fut = result.get_future();
    LocalLLM_DownloadModel("", [&](bool success, const std::string& path) {
        result.set_value({success, path});
    });
    auto status = fut.wait_for(std::chrono::seconds(5));
    ASSERT_EQ(status, std::future_status::ready);
    auto res = fut.get();
    EXPECT_FALSE(res.first);
    EXPECT_TRUE(res.second.empty());
}

TEST_F(LocalLLMTest, GenerateWithHighTemperatureDoesNotCrash) {
    std::promise<std::string> result;
    auto fut = result.get_future();
    LocalLLM_Generate("hot test", 1.999f, [&](const std::string& text) {
        result.set_value(text);
    });
    auto status = fut.wait_for(std::chrono::seconds(5));
    ASSERT_EQ(status, std::future_status::ready);
    SUCCEED();
}

TEST_F(LocalLLMTest, ShutdownDuringLoadingIsSafe) {
    LocalLLM_Init();
    LocalLLM_Shutdown();
    EXPECT_EQ(LocalLLM_GetState(), LocalLLMState::Unavailable);
}

TEST_F(LocalLLMTest, RepeatedInitDoesNotCrash) {
    for (int i = 0; i < 5; i++) {
        LocalLLM_Init();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    SUCCEED();
}
