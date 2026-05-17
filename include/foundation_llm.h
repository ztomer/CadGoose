#pragma once

// C interface for FoundationLLM Swift wrapper (FoundationModels backend, macOS 26+)

#ifdef __cplusplus
extern "C" {
#endif

int FoundationLLM_IsAvailable(void);
int FoundationLLM_ContextSize(void);

typedef void (*FoundationLLM_Callback)(const char* result, void* context);
void FoundationLLM_Generate(const char* prompt, float temperature,
                            FoundationLLM_Callback callback, void* context);

#ifdef __cplusplus
}
#endif
