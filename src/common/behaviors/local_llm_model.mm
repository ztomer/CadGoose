// local_llm_model.mm
// Model discovery and loading for local CoreML LLM
#include "local_llm.h"
#include "config.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

#include <cstdio>
#include <mutex>

static MLModel* s_model = nil;
static LocalLLMState s_state = LocalLLMState::Unavailable;
static std::mutex s_stateMutex;

extern void LocalLLM_LoadTokenizer(NSString* baseDir);

static NSString* FindModelAsset() {
    NSFileManager* fm = [NSFileManager defaultManager];

    if (!g_config.ai.localLlmModelPath.empty()) {
        NSString* path = [NSString stringWithUTF8String:g_config.ai.localLlmModelPath.c_str()];
        if ([fm fileExistsAtPath:path]) {
            fprintf(stderr, "[LOCAL_LLM] Found model via config path: %s\n", path.UTF8String);
            return path;
        }
    }

    NSString* configDir = [NSString stringWithUTF8String:ConfigDirPath().c_str()];
    NSString* modelsDir = [configDir stringByAppendingPathComponent:@"Models"];
    BOOL modelsDirExists = [fm fileExistsAtPath:modelsDir isDirectory:nil];
    fprintf(stderr, "[LOCAL_LLM] Checking ConfigDir/Models: %s (exists=%d)\n", modelsDir.UTF8String, modelsDirExists);
    if (modelsDirExists) {
        NSArray* contents = [fm contentsOfDirectoryAtPath:modelsDir error:nil];
        fprintf(stderr, "[LOCAL_LLM]   Contents: %lu items\n", (unsigned long)contents.count);
        for (NSString* item in contents) {
            NSString* fullPath = [modelsDir stringByAppendingPathComponent:item];
            BOOL isDir = NO;
            if ([fm fileExistsAtPath:fullPath isDirectory:&isDir] && isDir && [item hasSuffix:@".mlmodelc"]) {
                fprintf(stderr, "[LOCAL_LLM] Found model in ConfigDir/Models: %s\n", fullPath.UTF8String);
                return fullPath;
            }
        }
    }

    NSArray* assetBases = @[
        @"/System/Library/AssetsV2/com_apple_MobileAsset_UAF_FM_GenerativeModels/purpose_auto",
        @"/System/Library/AssetsV2/PreinstalledAssetsV2/InstallWithOs/com_apple_MobileAsset_UAF_FM_GenerativeModels",
        @"/System/Library/AssetsV2/com_apple_MobileAsset_MLModels/purpose_auto",
        @"/System/Library/AssetsV2/com_apple_MobileAsset_SiriVocabulary/purpose_auto",
    ];
    for (NSString* base in assetBases) {
        BOOL baseExists = [fm fileExistsAtPath:base isDirectory:nil];
        fprintf(stderr, "[LOCAL_LLM] Checking system path: %s (exists=%d)\n", base.UTF8String, baseExists);
        if (!baseExists) continue;
        NSArray* assets = [fm contentsOfDirectoryAtPath:base error:nil];
        fprintf(stderr, "[LOCAL_LLM]   Assets: %lu items\n", (unsigned long)assets.count);
        for (NSString* asset in assets) {
            if (![asset hasSuffix:@".asset"] && ![asset hasSuffix:@".bundle"]) continue;
            NSString* assetData = [[base stringByAppendingPathComponent:asset] stringByAppendingPathComponent:@"AssetData"];
            BOOL isDir = NO;
            if (![fm fileExistsAtPath:assetData isDirectory:&isDir] || !isDir) continue;
            NSArray* models = [fm contentsOfDirectoryAtPath:assetData error:nil];
            for (NSString* m in models) {
                if ([m hasSuffix:@".mlmodelc"]) {
                    NSString* found = [assetData stringByAppendingPathComponent:m];
                    fprintf(stderr, "[LOCAL_LLM] Found system model: %s\n", found.UTF8String);
                    return found;
                }
            }
        }
    }

    // macOS 26.5+: Check for .mlpackage files (newer format)
    NSArray* packageBases = @[
        @"/System/Library/AssetsV2/com_apple_MobileAsset_UAF_FM_GenerativeModels/purpose_auto",
        @"/System/Library/AssetsV2/PreinstalledAssetsV2/InstallWithOs/com_apple_MobileAsset_UAF_FM_GenerativeModels",
        @"/System/Library/AssetsV2/com_apple_MobileAsset_MLModels/purpose_auto",
    ];
    for (NSString* base in packageBases) {
        BOOL baseExists = [fm fileExistsAtPath:base isDirectory:nil];
        if (!baseExists) continue;
        NSArray* assets = [fm contentsOfDirectoryAtPath:base error:nil];
        for (NSString* asset in assets) {
            if (![asset hasSuffix:@".asset"] && ![asset hasSuffix:@".bundle"]) continue;
            NSString* assetData = [[base stringByAppendingPathComponent:asset] stringByAppendingPathComponent:@"AssetData"];
            BOOL isDir = NO;
            if (![fm fileExistsAtPath:assetData isDirectory:&isDir] || !isDir) continue;
            NSArray* models = [fm contentsOfDirectoryAtPath:assetData error:nil];
            for (NSString* m in models) {
                if ([m hasSuffix:@".mlpackage"]) {
                    NSString* found = [assetData stringByAppendingPathComponent:m];
                    fprintf(stderr, "[LOCAL_LLM] Found .mlpackage model: %s\n", found.UTF8String);
                    return found;
                }
            }
        }
    }

    // Check ~/Library/Caches/com.apple.CoreML/ for cached models
    NSString* homeDir = NSHomeDirectory();
    NSString* coremlCache = [homeDir stringByAppendingPathComponent:@"Library/Caches/com.apple.CoreML"];
    BOOL cacheExists = [fm fileExistsAtPath:coremlCache isDirectory:nil];
    fprintf(stderr, "[LOCAL_LLM] Checking CoreML cache: %s (exists=%d)\n", coremlCache.UTF8String, cacheExists);
    if (cacheExists) {
        NSDirectoryEnumerator* enumerator = [fm enumeratorAtPath:coremlCache];
        for (NSString* item in enumerator) {
            if ([item hasSuffix:@".mlmodelc"] || [item hasSuffix:@".mlpackage"]) {
                NSString* found = [coremlCache stringByAppendingPathComponent:item];
                fprintf(stderr, "[LOCAL_LLM] Found cached model: %s\n", found.UTF8String);
                return found;
            }
        }
    }

    // Scan ConfigDir/Models for .mlpackage files too
    if (modelsDirExists) {
        NSArray* contents = [fm contentsOfDirectoryAtPath:modelsDir error:nil];
        for (NSString* item in contents) {
            NSString* fullPath = [modelsDir stringByAppendingPathComponent:item];
            BOOL isDir = NO;
            if ([fm fileExistsAtPath:fullPath isDirectory:&isDir] && isDir && [item hasSuffix:@".mlpackage"]) {
                fprintf(stderr, "[LOCAL_LLM] Found .mlpackage in ConfigDir/Models: %s\n", fullPath.UTF8String);
                return fullPath;
            }
        }
    }

    NSString* bundled = [[NSBundle mainBundle] pathForResource:@"model" ofType:@"mlmodelc"];
    if (bundled) {
        fprintf(stderr, "[LOCAL_LLM] Found bundled model: %s\n", bundled.UTF8String);
        return bundled;
    }

    fprintf(stderr, "[LOCAL_LLM] No CoreML model found after exhaustive search\n");
    return nil;
}

void LocalLLM_Init() {
    std::lock_guard<std::mutex> lock(s_stateMutex);
    if (s_state == LocalLLMState::Loading || s_state == LocalLLMState::Ready) return;
    s_state = LocalLLMState::Loading;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        @autoreleasepool {
            NSString* modelPath = FindModelAsset();
            if (!modelPath) {
                fprintf(stderr, "[LOCAL_LLM] No CoreML model found\n");
                s_state = LocalLLMState::Unavailable;
                return;
            }

            NSError* err = nil;
            MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
            config.computeUnits = MLComputeUnitsAll;

            MLModel* model = [MLModel modelWithContentsOfURL:[NSURL fileURLWithPath:modelPath]
                                               configuration:config
                                                       error:&err];
            if (!model) {
                fprintf(stderr, "[LOCAL_LLM] Failed: %s\n", err.localizedDescription.UTF8String);
                s_state = LocalLLMState::Error;
                return;
            }

            s_model = model;

            MLModelDescription* desc = s_model.modelDescription;
            NSString* modelDesc = (NSString*)desc.metadata[MLModelDescriptionKey];
            fprintf(stderr, "[LOCAL_LLM] Model: %s\n", modelDesc.UTF8String);
            for (NSString* name in desc.inputDescriptionsByName) {
                MLFeatureDescription* fd = desc.inputDescriptionsByName[name];
                fprintf(stderr, "[LOCAL_LLM]   Input: %s (type=%d)\n", name.UTF8String, (int)fd.type);
            }
            for (NSString* name in desc.outputDescriptionsByName) {
                MLFeatureDescription* fd = desc.outputDescriptionsByName[name];
                fprintf(stderr, "[LOCAL_LLM]   Output: %s (type=%d)\n", name.UTF8String, (int)fd.type);
            }

            NSString* modelDir = [modelPath stringByDeletingLastPathComponent];
            LocalLLM_LoadTokenizer(modelDir);

            s_state = LocalLLMState::Ready;
            fprintf(stderr, "[LOCAL_LLM] Ready\n");
        }
    });
}

LocalLLMState LocalLLM_GetState() {
    return s_state;
}

MLModel* LocalLLM_GetModel() {
    return s_model;
}

void LocalLLM_Shutdown() {
    std::lock_guard<std::mutex> lock(s_stateMutex);
    s_model = nil;
    s_state = LocalLLMState::Unavailable;
    LocalLLM_ClearTokenizer();
}
