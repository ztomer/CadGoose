#pragma once

#include <string>
#include <functional>
#include <vector>

#ifdef __OBJC__
#import <CoreML/CoreML.h>
#endif

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

// Internal API (exposed for module splitting)
#ifdef __OBJC__
void LocalLLM_LoadTokenizer(NSString* baseDir);
MLModel* LocalLLM_GetModel();
#endif
std::vector<int> LocalLLM_EncodeText(const std::string& text);
std::string LocalLLM_DecodeTokens(const std::vector<int>& tokens);
bool LocalLLM_IsTokenizerReady();
int LocalLLM_VocabSize();
int LocalLLM_GetTokenId(const std::string& token);
void LocalLLM_ClearTokenizer();

// FoundationModels backend (macOS 26+)
#include "foundation_llm.h"
