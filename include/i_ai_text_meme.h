#pragma once
#include <string>

class IAITextMeme {
public:
    virtual ~IAITextMeme() = default;

    virtual void Tick(double time) = 0;
    virtual bool HasAvailable() = 0;
    virtual std::string Dequeue() = 0;
    virtual int QueueSize() const = 0;
    virtual void Reset() = 0;
    virtual void Inject(const std::string& text) = 0;
    virtual void LoadFileTexts() = 0;
};
