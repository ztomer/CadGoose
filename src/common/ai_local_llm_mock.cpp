#include "i_local_llm.h"
#include <string>
#include <functional>
#include <vector>
#include <algorithm>

class MockLocalLLM : public ILocalLLM {
public:
    State state = State::Unavailable;
    bool ready = false;
    bool initCalled = false;
    bool shutdownCalled = false;
    int generateCallCount = 0;
    int queueSize = 0;
    std::vector<std::string> dequeuedItems;
    std::vector<std::string> generatedPrompts;
    std::function<void(const std::string&)> lastGenerateCallback;

    State GetState() const override { return state; }
    bool IsReady() const override { return ready; }

    void Init() override {
        initCalled = true;
        state = State::Ready;
        ready = true;
    }

    void Generate(const std::string& prompt, float temperature,
                  std::function<void(const std::string&)> callback) override {
        generateCallCount++;
        generatedPrompts.push_back(prompt);
        lastGenerateCallback = callback;
        if (ready && callback) {
            callback("mock response for: " + prompt);
        }
    }

    int QueueSize() const override { return queueSize; }

    std::string Dequeue() override {
        if (!dequeuedItems.empty()) {
            std::string item = dequeuedItems.front();
            dequeuedItems.erase(dequeuedItems.begin());
            queueSize = (int)dequeuedItems.size();
            return item;
        }
        return {};
    }

    void Shutdown() override { shutdownCalled = true; }
};

static MockLocalLLM* g_mock = nullptr;

MockLocalLLM* GetMockLocalLLM() {
    if (!g_mock) g_mock = new MockLocalLLM();
    return g_mock;
}

void ResetMockLocalLLM() {
    delete g_mock;
    g_mock = nullptr;
}

ILocalLLM* GetLocalLLM() {
    return GetMockLocalLLM();
}
