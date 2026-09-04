#include "i_ai_http_client.h"
#include <string>
#include <functional>

class MockAIHttpClient : public IAIHttpClient {
public:
    bool connected = false;
    std::string endpoint = "http://localhost:1337";
    std::string model = "mock-model";
    int sendMessageCallCount = 0;
    int checkConnectionCallCount = 0;
    int refreshConnectionCallCount = 0;
    std::string lastSentMessage;
    std::string nextResponse = "mock response";
    std::string nextError;

    bool IsConnected() const override { return connected; }

    std::string GetEndpoint() const override { return endpoint; }

    std::string GetModel() const override { return model; }

    void SendMessage(const std::string& message,
                     std::function<void(const std::string&, const std::string&)> completion) override {
        sendMessageCallCount++;
        lastSentMessage = message;
        if (completion) {
            completion(nextResponse, nextError);
        }
    }

    void CheckConnection(std::function<void(bool, const std::string&)> completion) override {
        checkConnectionCallCount++;
        if (completion) {
            completion(connected, connected ? "Connected" : "Not connected");
        }
    }

    void RefreshConnection() override { refreshConnectionCallCount++; }
};

static MockAIHttpClient* g_mock = nullptr;

MockAIHttpClient* GetMockAIHttpClient() {
    if (!g_mock) g_mock = new MockAIHttpClient();
    return g_mock;
}

void ResetMockAIHttpClient() {
    delete g_mock;
    g_mock = nullptr;
}

IAIHttpClient* GetAIHttpClient() {
    return GetMockAIHttpClient();
}
