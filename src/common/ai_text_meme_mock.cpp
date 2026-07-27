#include "i_ai_text_meme.h"
#include <string>
#include <vector>
#include <algorithm>

class MockAITextMeme : public IAITextMeme {
public:
    bool hasAvailable = false;
    int queueSize = 0;
    int tickCount = 0;
    bool resetCalled = false;
    bool loadFileTextsCalled = false;
    std::vector<std::string> injectedTexts;
    std::vector<std::string> dequeuedItems;

    void Tick(double time) override { tickCount++; }

    bool HasAvailable() override { return hasAvailable; }

    std::string Dequeue() override {
        if (!dequeuedItems.empty()) {
            std::string item = dequeuedItems.front();
            dequeuedItems.erase(dequeuedItems.begin());
            queueSize = (int)dequeuedItems.size();
            return item;
        }
        return {};
    }

    int QueueSize() const override { return queueSize; }

    void Reset() override { resetCalled = true; }

    void Inject(const std::string& text) override {
        injectedTexts.push_back(text);
    }

    void LoadFileTexts() override { loadFileTextsCalled = true; }
};

static MockAITextMeme* g_mock = nullptr;

MockAITextMeme* GetMockAITextMeme() {
    if (!g_mock) g_mock = new MockAITextMeme();
    return g_mock;
}

void ResetMockAITextMeme() {
    delete g_mock;
    g_mock = nullptr;
}

IAITextMeme* GetAITextMeme() {
    return GetMockAITextMeme();
}
