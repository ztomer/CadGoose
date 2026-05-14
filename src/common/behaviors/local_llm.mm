#include "local_llm.h"
#include "config.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

#include <queue>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <random>
#include <cmath>
#include <algorithm>

#pragma mark - Internal State

static MLModel* s_model = nil;
static LocalLLMState s_state = LocalLLMState::Unavailable;
static std::mutex s_stateMutex;

static std::queue<std::string> s_resultQueue;
static std::mutex s_queueMutex;
static bool s_generating = false;
static std::mutex s_genMutex;

static std::unordered_map<std::string, int> s_vocab;
static std::unordered_map<int, std::string> s_idToToken;
static bool s_tokenizerReady = false;

static constexpr int kMaxContext = 512;
static constexpr int kMaxGen = 48;
static constexpr int kEOS = 2;

#pragma mark - Tokenizer

static void LoadTokenizer(NSString* baseDir) {
    NSFileManager* fm = [NSFileManager defaultManager];
    NSArray* candidates = @[
        [baseDir stringByAppendingPathComponent:@"tokenizer.json"],
        [baseDir stringByAppendingPathComponent:@"vocab.json"],
    ];
    NSString* parentDir = [baseDir stringByDeletingLastPathComponent];
    candidates = [candidates arrayByAddingObjectsFromArray:@[
        [parentDir stringByAppendingPathComponent:@"tokenizer.json"],
        [parentDir stringByAppendingPathComponent:@"vocab.json"],
    ]];

    for (NSString* path in candidates) {
        if (![fm fileExistsAtPath:path]) continue;
        NSError* err = nil;
        NSData* data = [NSData dataWithContentsOfURL:[NSURL fileURLWithPath:path] options:0 error:&err];
        if (err || !data) continue;
        id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];
        if (err) continue;

        if ([json isKindOfClass:[NSDictionary class]]) {
            NSDictionary* dict = (NSDictionary*)json;
            id modelDict = dict[@"model"];
            id vocab = modelDict ? ((NSDictionary*)modelDict)[@"vocab"] : dict;
            if ([vocab isKindOfClass:[NSDictionary class]]) {
                s_vocab.clear();
                s_idToToken.clear();
                for (NSString* key in vocab) {
                    int idVal = [vocab[key] intValue];
                    s_vocab[std::string(key.UTF8String)] = idVal;
                    s_idToToken[idVal] = std::string(key.UTF8String);
                }
                s_tokenizerReady = !s_vocab.empty();
                fprintf(stderr, "[LOCAL_LLM] Tokenizer loaded (%zu entries)\n", s_vocab.size());
                return;
            }
        }
    }
}

static std::vector<int> EncodeText(const std::string& text) {
    if (!s_tokenizerReady) return {};
    std::vector<int> tokens;
    auto it = s_vocab.find(text);
    if (it != s_vocab.end()) {
        tokens.push_back(it->second);
        return tokens;
    }
    std::string word;
    int unkId = s_vocab.count("<unk>") ? s_vocab["<unk>"] : 0;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (isspace((unsigned char)c)) {
            if (!word.empty()) {
                auto wit = s_vocab.find(word);
                tokens.push_back(wit != s_vocab.end() ? wit->second : unkId);
                word.clear();
            }
            auto sit = s_vocab.find(" ");
            if (sit != s_vocab.end()) tokens.push_back(sit->second);
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        auto wit = s_vocab.find(word);
        tokens.push_back(wit != s_vocab.end() ? wit->second : unkId);
    }
    return tokens;
}

static std::string DecodeTokens(const std::vector<int>& tokens) {
    std::string result;
    for (int t : tokens) {
        auto it = s_idToToken.find(t);
        if (it != s_idToToken.end()) result += it->second;
    }
    return result;
}

#pragma mark - Model Discovery

static NSString* FindModelAsset() {
    NSFileManager* fm = [NSFileManager defaultManager];

    if (!g_config.ai.localLlmModelPath.empty()) {
        NSString* path = [NSString stringWithUTF8String:g_config.ai.localLlmModelPath.c_str()];
        if ([fm fileExistsAtPath:path]) return path;
    }

    NSString* configDir = [NSString stringWithUTF8String:ConfigDirPath().c_str()];
    NSString* modelsDir = [configDir stringByAppendingPathComponent:@"Models"];
    NSArray* contents = [fm contentsOfDirectoryAtPath:modelsDir error:nil];
    for (NSString* item in contents) {
        NSString* fullPath = [modelsDir stringByAppendingPathComponent:item];
        BOOL isDir = NO;
        if ([fm fileExistsAtPath:fullPath isDirectory:&isDir] && isDir && [item hasSuffix:@".mlmodelc"]) {
            return fullPath;
        }
    }

    NSArray* assetBases = @[
        @"/System/Library/AssetsV2/com_apple_MobileAsset_UAF_FM_GenerativeModels/purpose_auto",
        @"/System/Library/AssetsV2/PreinstalledAssetsV2/InstallWithOs/com_apple_MobileAsset_UAF_FM_GenerativeModels",
    ];
    for (NSString* base in assetBases) {
        NSArray* assets = [fm contentsOfDirectoryAtPath:base error:nil];
        for (NSString* asset in assets) {
            if (![asset hasSuffix:@".asset"]) continue;
            NSString* assetData = [[base stringByAppendingPathComponent:asset] stringByAppendingPathComponent:@"AssetData"];
            BOOL isDir = NO;
            if (![fm fileExistsAtPath:assetData isDirectory:&isDir] || !isDir) continue;
            NSArray* models = [fm contentsOfDirectoryAtPath:assetData error:nil];
            for (NSString* m in models) {
                if ([m hasSuffix:@".mlmodelc"]) return [assetData stringByAppendingPathComponent:m];
            }
        }
    }

    NSString* bundled = [[NSBundle mainBundle] pathForResource:@"model" ofType:@"mlmodelc"];
    if (bundled) return bundled;

    return nil;
}

#pragma mark - Public API

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
            LoadTokenizer(modelDir);

            s_state = LocalLLMState::Ready;
            fprintf(stderr, "[LOCAL_LLM] Ready\n");
        }
    });
}

LocalLLMState LocalLLM_GetState() {
    return s_state;
}

int LocalLLM_QueueSize() {
    std::lock_guard<std::mutex> lock(s_queueMutex);
    return (int)s_resultQueue.size();
}

std::string LocalLLM_Dequeue() {
    std::lock_guard<std::mutex> lock(s_queueMutex);
    if (s_resultQueue.empty()) return "";
    std::string r = s_resultQueue.front();
    s_resultQueue.pop();
    return r;
}

void LocalLLM_Shutdown() {
    std::lock_guard<std::mutex> lock(s_stateMutex);
    s_model = nil;
    s_state = LocalLLMState::Unavailable;
    s_vocab.clear();
    s_idToToken.clear();
    s_tokenizerReady = false;
}

#pragma mark - Inference

static int SampleLogits(id<MLFeatureProvider> output, float temperature) {
    MLModelDescription* desc = s_model.modelDescription;
    NSArray* outNames = @[@"output", @"logits", @"probs", @"prediction", @"y"];
    NSString* outName = nil;
    for (NSString* n in outNames) {
        if (desc.outputDescriptionsByName[n]) { outName = n; break; }
    }
    if (!outName) outName = [[desc.outputDescriptionsByName allKeys] firstObject];
    if (!outName) return kEOS;

    MLFeatureValue* val = [output featureValueForName:outName];
    if (val.type != MLFeatureTypeMultiArray) return kEOS;

    MLMultiArray* arr = val.multiArrayValue;
    if (!arr || arr.count == 0) return kEOS;

    float* data = (float*)arr.dataPointer;
    if (!data) return kEOS;

    int64_t total = arr.count / [arr.shape[0] longValue];
    int64_t vocabSize = MIN(total, s_vocab.empty() ? 50000 : (int64_t)s_vocab.size());
    int64_t lastPos = [arr.shape[0] longValue] - 1;
    int64_t offset = lastPos * total;

    float maxL = -1e30f;
    for (int64_t i = 0; i < vocabSize; i++) {
        if (data[offset + i] > maxL) maxL = data[offset + i];
    }

    std::vector<float> probs(vocabSize);
    float sum = 0;
    for (int64_t i = 0; i < vocabSize; i++) {
        float p = expf((data[offset + i] - maxL) / temperature);
        probs[(int)i] = p;
        sum += p;
    }
    if (sum <= 0) return kEOS;

    int K = 40;
    if (vocabSize > K) {
        std::vector<std::pair<float, int>> idx;
        for (int64_t i = 0; i < vocabSize; i++) idx.push_back({probs[(int)i], (int)i});
        std::partial_sort(idx.begin(), idx.begin() + K, idx.end(),
                         [](auto& a, auto& b) { return a.first > b.first; });
        float topSum = 0;
        for (int i = 0; i < K; i++) topSum += idx[i].first;
        std::fill(probs.begin(), probs.end(), 0.0f);
        for (int i = 0; i < K; i++) probs[idx[i].second] = idx[i].first / topSum;
        sum = 1.0f;
    }

    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(0, sum);
    float r = dist(gen);
    float cum = 0;
    for (int64_t i = 0; i < vocabSize; i++) {
        cum += probs[(int)i];
        if (r < cum) return (int)i;
    }
    return (int)(vocabSize - 1);
}

static NSString* FindInputName() {
    MLModelDescription* desc = s_model.modelDescription;
    NSArray* names = @[@"input_ids", @"input", @"inputIds", @"tokens", @"token_ids", @"x"];
    for (NSString* n in names) {
        if (desc.inputDescriptionsByName[n]) return n;
    }
    return [[desc.inputDescriptionsByName allKeys] firstObject];
}

static id CreateInputProvider(const std::vector<int>& tokens) {
    NSString* inputName = FindInputName();
    if (!inputName) return nil;

    MLFeatureDescription* fd = [s_model.modelDescription.inputDescriptionsByName objectForKey:inputName];
    if (!fd) return nil;

    if (fd.type == MLFeatureTypeMultiArray) {
        NSError* err = nil;
        NSArray* shape = @[@1, @((NSInteger)tokens.size())];
        MLMultiArray* arr = [[MLMultiArray alloc] initWithShape:shape
                                                       dataType:MLMultiArrayDataTypeInt32
                                                          error:&err];
        if (err) return nil;
        for (size_t i = 0; i < tokens.size(); i++) {
            arr[i] = @(tokens[i]);
        }
        return [[MLDictionaryFeatureProvider alloc]
                 initWithDictionary:@{inputName: [MLFeatureValue featureValueWithMultiArray:arr]}
                             error:nil];
    }

    if (fd.type == MLFeatureTypeSequence) {
        NSMutableArray* seqArr = [NSMutableArray arrayWithCapacity:tokens.size()];
        for (int t : tokens) [seqArr addObject:@(t)];
        MLSequence* seq = [MLSequence sequenceWithInt64Array:seqArr];
        return [[MLDictionaryFeatureProvider alloc]
                 initWithDictionary:@{inputName: [MLFeatureValue featureValueWithSequence:seq]}
                             error:nil];
    }

    return nil;
}

void LocalLLM_Generate(const std::string& prompt, float temperature,
                       std::function<void(const std::string&)> callback) {
    {
        std::lock_guard<std::mutex> lock(s_genMutex);
        if (s_generating) {
            if (callback) callback("");
            return;
        }
        s_generating = true;
    }

    if (s_state != LocalLLMState::Ready || !s_model) {
        LocalLLM_Init();
        std::lock_guard<std::mutex> lock(s_genMutex);
        s_generating = false;
        if (callback) callback("");
        return;
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        @autoreleasepool {
            NSDate* start = [NSDate date];

            std::vector<int> tokens = EncodeText(prompt);
            if (tokens.empty()) {
                std::lock_guard<std::mutex> lock(s_genMutex);
                s_generating = false;
                if (callback) callback("");
                return;
            }

            std::vector<int> generated;
            int maxNew = MIN(kMaxGen, kMaxContext - (int)tokens.size());

            for (int step = 0; step < maxNew; step++) {
                @autoreleasepool {
                    auto ctx = tokens;
                    ctx.insert(ctx.end(), generated.begin(), generated.end());
                    if ((int)ctx.size() > kMaxContext)
                        ctx.erase(ctx.begin(), ctx.begin() + ((int)ctx.size() - kMaxContext));

                    id provider = CreateInputProvider(ctx);
                    if (!provider) break;

                    NSError* predErr = nil;
                    id<MLFeatureProvider> output = [s_model predictionFromFeatures:provider
                                                                             error:&predErr];
                    if (!output) break;

                    int next = SampleLogits(output, temperature);
                    if (next == kEOS || next == 0) break;

                    generated.push_back(next);

                    if (next == s_vocab["."] && generated.size() > 5) break;
                    if (next == s_vocab["!"] && generated.size() > 3) break;
                    if (next == s_vocab["?"] && generated.size() > 3) break;
                }
            }

            std::string result = DecodeTokens(generated);
            auto trim = [](std::string& s) {
                while (!s.empty() && (s[0]==' '||s[0]=='\n')) s.erase(0,1);
                while (!s.empty() && (s.back()==' '||s.back()=='\n')) s.pop_back();
            };
            trim(result);

            double elapsed = [[NSDate date] timeIntervalSinceDate:start];
            if (!result.empty()) {
                std::lock_guard<std::mutex> lock(s_queueMutex);
                s_resultQueue.push(result);
            }
            fprintf(stderr, "[LOCAL_LLM] Generated %zu tok (%.1fs): %s\n",
                   generated.size(), elapsed, result.c_str());

            std::lock_guard<std::mutex> lock(s_genMutex);
            s_generating = false;
            if (callback) callback(result);
        }
    });
}

void LocalLLM_DownloadModel(const std::string& url,
                            std::function<void(bool success, const std::string& path)> callback) {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        @autoreleasepool {
            NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
            NSURL* downloadURL = [NSURL URLWithString:nsUrl];
            if (!downloadURL) {
                fprintf(stderr, "[LOCAL_LLM] Invalid download URL\n");
                if (callback) callback(false, "");
                return;
            }

            std::filesystem::path modelsDir = ConfigDirPath() / "Models";
            std::filesystem::create_directories(modelsDir);
            NSString* destDir = [NSString stringWithUTF8String:modelsDir.c_str()];

            NSError* err = nil;
            NSData* data = [NSData dataWithContentsOfURL:downloadURL
                                                 options:NSDataReadingUncached
                                                   error:&err];
            if (err || !data) {
                fprintf(stderr, "[LOCAL_LLM] Download failed: %s\n",
                       err.localizedDescription.UTF8String);
                if (callback) callback(false, "");
                return;
            }

            // Save as .zip or directly as .mlmodelc
            NSString* fileName = [downloadURL lastPathComponent];
            NSString* destPath = [destDir stringByAppendingPathComponent:fileName];

            if ([data writeToFile:destPath atomically:YES]) {
                fprintf(stderr, "[LOCAL_LLM] Downloaded to %s\n", destPath.UTF8String);

                // If it's a zip, unzip it
                if ([fileName hasSuffix:@".zip"]) {
                    NSTask* unzip = [[NSTask alloc] init];
                    unzip.executableURL = [NSURL fileURLWithPath:@"/usr/bin/unzip"];
                    unzip.arguments = @[@"-o", destPath, @"-d", destDir];
                    NSError* taskErr = nil;
                    if (![unzip launchAndReturnError:&taskErr]) {
                        fprintf(stderr, "[LOCAL_LLM] Unzip failed: %s\n",
                               taskErr.localizedDescription.UTF8String);
                        if (callback) callback(false, "");
                        return;
                    }
                    [unzip waitUntilExit];
                    [[NSFileManager defaultManager] removeItemAtPath:destPath error:nil];
                }

                // Reload model
                s_state = LocalLLMState::Unavailable;
                LocalLLM_Init();
                if (callback) callback(true, std::string(destPath.UTF8String));
            } else {
                fprintf(stderr, "[LOCAL_LLM] Failed to save model\n");
                if (callback) callback(false, "");
            }
        }
    });
}
