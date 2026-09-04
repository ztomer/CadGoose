#pragma once
#include <string>
#include <functional>

class ILocalLLM {
public:
    virtual ~ILocalLLM() = default;

    enum class State { Unavailable, Loading, Ready, Error };

    virtual State GetState() const = 0;
    virtual bool IsReady() const = 0;
    virtual void Init() = 0;
    virtual void Generate(const std::string& prompt, float temperature,
                          std::function<void(const std::string&)> callback) = 0;
    virtual int QueueSize() const = 0;
    virtual std::string Dequeue() = 0;
    virtual void Shutdown() = 0;
};
