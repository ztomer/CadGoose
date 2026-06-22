// local_llm_model.mm
// Model discovery and loading for local CoreML LLM
#include "local_llm.h"
#include "log.h"
#include "config.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

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
            CG_DEBUG("LOCAL_LLM", "Found model via config path: %s", path.UTF8String);
            return path;
        }
    }

    NSString* configDir = [NSString stringWithUTF8String:ConfigDirPath().c_str()];
    NSString* modelsDir = [configDir stringByAppendingPathComponent:@"Models"];
    BOOL modelsDirExists = [fm fileExistsAtPath:modelsDir isDirectory:nil];
    CG_DEBUG("LOCAL_LLM", "Checking ConfigDir/Models: %s (exists=%d)", modelsDir.UTF8String, modelsDirExists);
    if (modelsDirExists) {
        NSArray* contents = [fm contentsOfDirectoryAtPath:modelsDir error:nil];
        CG_DEBUG("LOCAL_LLM", "  Contents: %lu items", (unsigned long)contents.count);
        for (NSString* item in contents) {
            NSString* fullPath = [modelsDir stringByAppendingPathComponent:item];
            BOOL isDir = NO;
            if ([fm fileExistsAtPath:fullPath isDirectory:&isDir] && isDir && [item hasSuffix:@".mlmodelc"]) {
                CG_DEBUG("LOCAL_LLM", "Found model in ConfigDir/Models: %s", fullPath.UTF8String);
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
        CG_DEBUG("LOCAL_LLM", "Checking system path: %s (exists=%d)", base.UTF8String, baseExists);
        if (!baseExists) continue;
        NSArray* assets = [fm contentsOfDirectoryAtPath:base error:nil];
        CG_DEBUG("LOCAL_LLM", "  Assets: %lu items", (unsigned long)assets.count);
        for (NSString* asset in assets) {
            if (![asset hasSuffix:@".asset"] && ![asset hasSuffix:@".bundle"]) continue;
            NSString* assetData = [[base stringByAppendingPathComponent:asset] stringByAppendingPathComponent:@"AssetData"];
            BOOL isDir = NO;
            if (![fm fileExistsAtPath:assetData isDirectory:&isDir] || !isDir) continue;
            NSArray* models = [fm contentsOfDirectoryAtPath:assetData error:nil];
            for (NSString* m in models) {
                if ([m hasSuffix:@".mlmodelc"]) {
                    NSString* found = [assetData stringByAppendingPathComponent:m];
                    CG_DEBUG("LOCAL_LLM", "Found system model: %s", found.UTF8String);
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
                    CG_DEBUG("LOCAL_LLM", "Found .mlpackage model: %s", found.UTF8String);
                    return found;
                }
            }
        }
    }

    // Check custom search paths from config
    for (const auto& searchPath : g_config.ai.localLlmSearchPaths) {
        NSString* path = [NSString stringWithUTF8String:searchPath.c_str()];
        BOOL isDir = NO;
        if ([fm fileExistsAtPath:path isDirectory:&isDir] && isDir) {
            NSArray* contents = [fm contentsOfDirectoryAtPath:path error:nil];
            for (NSString* item in contents) {
                NSString* fullPath = [path stringByAppendingPathComponent:item];
                BOOL itemIsDir = NO;
                if ([fm fileExistsAtPath:fullPath isDirectory:&itemIsDir] && itemIsDir &&
                    ([item hasSuffix:@".mlmodelc"] || [item hasSuffix:@".mlpackage"])) {
                    CG_DEBUG("LOCAL_LLM", "Found model in custom path: %s", fullPath.UTF8String);
                    return fullPath;
                }
            }
        } else if ([fm fileExistsAtPath:path]) {
            if ([path hasSuffix:@".mlmodelc"] || [path hasSuffix:@".mlpackage"]) {
                CG_DEBUG("LOCAL_LLM", "Found model via custom path: %s", path.UTF8String);
                return path;
            }
        }
    }

    // Check ~/Library/Caches/com.apple.CoreML/ for cached models
    NSString* homeDir = NSHomeDirectory();
    NSString* coremlCache = [homeDir stringByAppendingPathComponent:@"Library/Caches/com.apple.CoreML"];
    BOOL cacheExists = [fm fileExistsAtPath:coremlCache isDirectory:nil];
    CG_DEBUG("LOCAL_LLM", "Checking CoreML cache: %s (exists=%d)", coremlCache.UTF8String, cacheExists);
    if (cacheExists) {
        NSDirectoryEnumerator* enumerator = [fm enumeratorAtPath:coremlCache];
        for (NSString* item in enumerator) {
            if ([item hasSuffix:@".mlmodelc"] || [item hasSuffix:@".mlpackage"]) {
                NSString* found = [coremlCache stringByAppendingPathComponent:item];
                CG_DEBUG("LOCAL_LLM", "Found cached model: %s", found.UTF8String);
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
                CG_DEBUG("LOCAL_LLM", "Found .mlpackage in ConfigDir/Models: %s", fullPath.UTF8String);
                return fullPath;
            }
        }
    }

    NSString* bundled = [[NSBundle mainBundle] pathForResource:@"model" ofType:@"mlmodelc"];
    if (bundled) {
        CG_DEBUG("LOCAL_LLM", "Found bundled model: %s", bundled.UTF8String);
        return bundled;
    }

    CG_ERROR("LOCAL_LLM", "No CoreML model found after exhaustive search");
    return nil;
}

void LocalLLM_Init() {
    std::lock_guard<std::mutex> lock(s_stateMutex);
    if (s_state == LocalLLMState::Loading || s_state == LocalLLMState::Ready) return;
    s_state = LocalLLMState::Loading;

    // Check for FoundationModels first (macOS 26+)
    if (FoundationLLM_IsAvailable()) {
        CG_DEBUG("LOCAL_LLM", "Using FoundationModels backend (macOS 26+)");
        s_state = LocalLLMState::Ready;
        CG_DEBUG("LOCAL_LLM", "Ready (FoundationModels)");
        return;
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        @autoreleasepool {
            NSString* modelPath = FindModelAsset();
            if (!modelPath) {
                CG_ERROR("LOCAL_LLM", "No CoreML model found");
                { std::lock_guard<std::mutex> lock(s_stateMutex); s_state = LocalLLMState::Unavailable; }
                return;
            }

            NSError* err = nil;
            MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
            config.computeUnits = MLComputeUnitsAll;

            MLModel* model = [MLModel modelWithContentsOfURL:[NSURL fileURLWithPath:modelPath]
                                               configuration:config
                                                       error:&err];
            if (!model) {
                CG_ERROR("LOCAL_LLM", "Failed: %s", err.localizedDescription.UTF8String);
                { std::lock_guard<std::mutex> lock(s_stateMutex); s_state = LocalLLMState::Error; }
                return;
            }

            s_model = model;

            MLModelDescription* desc = s_model.modelDescription;
            NSString* modelDesc = (NSString*)desc.metadata[MLModelDescriptionKey];
            CG_DEBUG("LOCAL_LLM", "Model: %s", modelDesc.UTF8String);
            for (NSString* name in desc.inputDescriptionsByName) {
                MLFeatureDescription* fd = desc.inputDescriptionsByName[name];
                CG_DEBUG("LOCAL_LLM", "  Input: %s (type=%d)", name.UTF8String, (int)fd.type);
            }
            for (NSString* name in desc.outputDescriptionsByName) {
                MLFeatureDescription* fd = desc.outputDescriptionsByName[name];
                CG_DEBUG("LOCAL_LLM", "  Output: %s (type=%d)", name.UTF8String, (int)fd.type);
            }

            NSString* modelDir = [modelPath stringByDeletingLastPathComponent];
            LocalLLM_LoadTokenizer(modelDir);

            { std::lock_guard<std::mutex> lock(s_stateMutex); s_state = LocalLLMState::Ready; }
            CG_DEBUG("LOCAL_LLM", "Ready");
        }
    });
}

LocalLLMState LocalLLM_GetState() {
    std::lock_guard<std::mutex> lock(s_stateMutex);
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
