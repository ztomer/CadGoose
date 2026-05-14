#pragma once

#include <string>
#include <functional>

enum class LocalLLMState {
    Unavailable,
    Loading,
    Ready,
    Error
};

LocalLLMState LocalLLM_GetState();
void LocalLLM_Init();
void LocalLLM_Generate(const std::string& prompt, float temperature,
                       std::function<void(const std::string&)> callback);
int LocalLLM_QueueSize();
std::string LocalLLM_Dequeue();
void LocalLLM_DownloadModel(const std::string& url,
                            std::function<void(bool success, const std::string& path)> callback);
void LocalLLM_Shutdown();
