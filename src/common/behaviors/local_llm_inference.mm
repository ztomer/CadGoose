// local_llm_inference.mm
// Sampling, input creation, generation, and download for local CoreML LLM
#include "local_llm.h"
#include "config.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

#include <queue>
#include <mutex>
#include <cstdio>
#include <random>
#include <cmath>
#include <algorithm>
#include <filesystem>

// --- Inference constants ---
static constexpr int kMaxContext = 512;
static constexpr int kMaxGen = 48;
static constexpr int kEOS = 2;
static constexpr int kTopK = 40;
static constexpr float kLogitFloor = -1e30f;

static std::queue<std::string> s_resultQueue;
static std::mutex s_queueMutex;
static bool s_generating = false;
static std::mutex s_genMutex;

extern MLModel* LocalLLM_GetModel();
extern std::vector<int> LocalLLM_EncodeText(const std::string& text);
extern std::string LocalLLM_DecodeTokens(const std::vector<int>& tokens);
extern bool LocalLLM_IsTokenizerReady();
extern int LocalLLM_GetTokenId(const std::string& token);

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

static int SampleLogits(id<MLFeatureProvider> output, float temperature) {
    MLModel* model = LocalLLM_GetModel();
    if (!model) return kEOS;

    MLModelDescription* desc = model.modelDescription;
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
    int64_t vocabSize = MIN(total, (int64_t)LocalLLM_VocabSize());
    int64_t lastPos = [arr.shape[0] longValue] - 1;
    int64_t offset = lastPos * total;

    float maxL = kLogitFloor;
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

    if (vocabSize > kTopK) {
        std::vector<std::pair<float, int>> idx;
        for (int64_t i = 0; i < vocabSize; i++) idx.push_back({probs[(int)i], (int)i});
        std::partial_sort(idx.begin(), idx.begin() + kTopK, idx.end(),
                         [](auto& a, auto& b) { return a.first > b.first; });
        float topSum = 0;
        for (int i = 0; i < kTopK; i++) topSum += idx[i].first;
        std::fill(probs.begin(), probs.end(), 0.0f);
        for (int i = 0; i < kTopK; i++) probs[idx[i].second] = idx[i].first / topSum;
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
    MLModel* model = LocalLLM_GetModel();
    if (!model) return nil;
    MLModelDescription* desc = model.modelDescription;
    NSArray* names = @[@"input_ids", @"input", @"inputIds", @"tokens", @"token_ids", @"x"];
    for (NSString* n in names) {
        if (desc.inputDescriptionsByName[n]) return n;
    }
    return [[desc.inputDescriptionsByName allKeys] firstObject];
}

static id CreateInputProvider(const std::vector<int>& tokens) {
    MLModel* model = LocalLLM_GetModel();
    if (!model) return nil;

    NSString* inputName = FindInputName();
    if (!inputName) return nil;

    MLFeatureDescription* fd = [model.modelDescription.inputDescriptionsByName objectForKey:inputName];
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

    if (LocalLLM_GetState() != LocalLLMState::Ready || !LocalLLM_GetModel()) {
        LocalLLM_Init();
        std::lock_guard<std::mutex> lock(s_genMutex);
        s_generating = false;
        if (callback) callback("");
        return;
    }

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        @autoreleasepool {
            NSDate* start = [NSDate date];

            std::vector<int> tokens = LocalLLM_EncodeText(prompt);
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
                    id<MLFeatureProvider> output = [LocalLLM_GetModel() predictionFromFeatures:provider
                                                                                       error:&predErr];
                    if (!output) break;

                    int next = SampleLogits(output, temperature);
                    if (next == kEOS || next == 0) break;

                    generated.push_back(next);

                    int dotId = LocalLLM_GetTokenId(".");
                    int exclId = LocalLLM_GetTokenId("!");
                    int qmarkId = LocalLLM_GetTokenId("?");
                    if (next == dotId && generated.size() > 5) break;
                    if (next == exclId && generated.size() > 3) break;
                    if (next == qmarkId && generated.size() > 3) break;
                }
            }

            std::string result = LocalLLM_DecodeTokens(generated);
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

            NSString* fileName = [downloadURL lastPathComponent];
            NSString* destPath = [destDir stringByAppendingPathComponent:fileName];

            if ([data writeToFile:destPath atomically:YES]) {
                fprintf(stderr, "[LOCAL_LLM] Downloaded to %s\n", destPath.UTF8String);

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
                LocalLLM_Init();
                if (callback) callback(true, std::string(destPath.UTF8String));
            } else {
                fprintf(stderr, "[LOCAL_LLM] Failed to save model\n");
                if (callback) callback(false, "");
            }
        }
    });
}
