#pragma once
#include <string>
#include <functional>

class IAIHttpClient {
public:
    virtual ~IAIHttpClient() = default;

    virtual bool IsConnected() const = 0;
    virtual std::string GetEndpoint() const = 0;
    virtual std::string GetModel() const = 0;
    virtual void SendMessage(const std::string& message,
                             std::function<void(const std::string&, const std::string&)> completion) = 0;
    virtual void CheckConnection(std::function<void(bool, const std::string&)> completion) = 0;
    virtual void RefreshConnection() = 0;
};
