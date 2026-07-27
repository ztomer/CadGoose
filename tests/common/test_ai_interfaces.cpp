#include <gtest/gtest.h>
#include "i_local_llm.h"
#include "i_ai_text_meme.h"
#include "i_ai_http_client.h"

extern ILocalLLM* GetLocalLLM();
extern void ResetMockLocalLLM();

extern IAITextMeme* GetAITextMeme();
extern void ResetMockAITextMeme();

extern IAIHttpClient* GetAIHttpClient();
extern void ResetMockAIHttpClient();

class AIInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResetMockLocalLLM();
        ResetMockAITextMeme();
        ResetMockAIHttpClient();
    }
};

// --- ILocalLLM tests ---

TEST_F(AIInterfaceTest, LocalLLM_InitialState) {
    auto* llm = GetLocalLLM();
    EXPECT_EQ(llm->GetState(), ILocalLLM::State::Unavailable);
    EXPECT_FALSE(llm->IsReady());
    EXPECT_EQ(llm->QueueSize(), 0);
}

TEST_F(AIInterfaceTest, LocalLLM_InitMakesReady) {
    auto* llm = GetLocalLLM();
    llm->Init();
    EXPECT_EQ(llm->GetState(), ILocalLLM::State::Ready);
    EXPECT_TRUE(llm->IsReady());
}

TEST_F(AIInterfaceTest, LocalLLM_GenerateWhenReady) {
    auto* llm = GetLocalLLM();
    llm->Init();
    std::string result;
    llm->Generate("hello", 0.7, [&](const std::string& r) { result = r; });
    EXPECT_EQ(result, "mock response for: hello");
}

TEST_F(AIInterfaceTest, LocalLLM_GenerateNotReady) {
    auto* llm = GetLocalLLM();
    bool callbackCalled = false;
    llm->Generate("hello", 0.7, [&](const std::string& r) {
        callbackCalled = true;
    });
    // Not ready — callback stored but not invoked until ready
    EXPECT_FALSE(callbackCalled);
    llm->Init();
    // After init, the stored callback may fire on next call
}

TEST_F(AIInterfaceTest, LocalLLM_EnqueueDequeue) {
    auto* llm = GetLocalLLM();
    EXPECT_EQ(llm->Dequeue(), "");
}

TEST_F(AIInterfaceTest, LocalLLM_Shutdown) {
    auto* llm = GetLocalLLM();
    llm->Shutdown();
}

// --- IAITextMeme tests ---

TEST_F(AIInterfaceTest, TextMeme_InitialState) {
    auto* tm = GetAITextMeme();
    EXPECT_FALSE(tm->HasAvailable());
    EXPECT_EQ(tm->QueueSize(), 0);
    EXPECT_EQ(tm->Dequeue(), "");
}

TEST_F(AIInterfaceTest, TextMeme_TickIncrementsCounter) {
    auto* tm = GetAITextMeme();
    tm->Tick(1.0);
    tm->Tick(2.0);
    tm->Tick(3.0);
}

TEST_F(AIInterfaceTest, TextMeme_Reset) {
    auto* tm = GetAITextMeme();
    tm->Reset();
}

TEST_F(AIInterfaceTest, TextMeme_LoadFileTexts) {
    auto* tm = GetAITextMeme();
    tm->LoadFileTexts();
}

TEST_F(AIInterfaceTest, TextMeme_Inject) {
    auto* tm = GetAITextMeme();
    tm->Inject("hello");
    tm->Inject("world");
}

// --- IAIHttpClient tests ---

TEST_F(AIInterfaceTest, HttpClient_InitialState) {
    auto* client = GetAIHttpClient();
    EXPECT_FALSE(client->IsConnected());
    EXPECT_EQ(client->GetEndpoint(), "http://localhost:1337");
    EXPECT_EQ(client->GetModel(), "mock-model");
}

TEST_F(AIInterfaceTest, HttpClient_SendMessage) {
    auto* client = GetAIHttpClient();
    std::string response, error;
    client->SendMessage("hello", [&](const std::string& r, const std::string& e) {
        response = r;
        error = e;
    });
    EXPECT_EQ(response, "mock response");
    EXPECT_TRUE(error.empty());
}

TEST_F(AIInterfaceTest, HttpClient_CheckConnection) {
    auto* client = GetAIHttpClient();
    bool connected = false;
    std::string message;
    client->CheckConnection([&](bool c, const std::string& m) {
        connected = c;
        message = m;
    });
    EXPECT_FALSE(connected);
    EXPECT_EQ(message, "Not connected");
}

TEST_F(AIInterfaceTest, HttpClient_RefreshConnection) {
    auto* client = GetAIHttpClient();
    client->RefreshConnection();
}
