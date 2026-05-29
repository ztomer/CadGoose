#pragma once

// C interface for FoundationLLM Swift wrapper (FoundationModels backend, macOS 26+)

#ifdef __cplusplus
extern "C" {
#endif

int FoundationLLM_IsAvailable(void);

// Reason code for (un)availability of the FoundationModels backend:
//   0 = available
//   1 = framework not present (built without macOS 26 SDK, or OS < 26)
//   2 = device not eligible for Apple Intelligence
//   3 = Apple Intelligence not enabled in System Settings
//   4 = model still downloading / not ready
//   5 = unavailable, unknown reason
int FoundationLLM_AvailabilityCode(void);

int FoundationLLM_ContextSize(void);

typedef void (*FoundationLLM_Callback)(const char* result, void* context);
void FoundationLLM_Generate(const char* prompt, float temperature,
                            FoundationLLM_Callback callback, void* context);

#ifdef __cplusplus
}
#endif
