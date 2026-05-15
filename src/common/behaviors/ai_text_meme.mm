#include "ai_text_meme.h"
#include "config.h"
#include "world.h"
#include "local_llm.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

#include <queue>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <functional>
#include <fstream>
#include <random>

#include "ring_buffer.h"
#include "assets.h"

#pragma mark - Internal State

static std::queue<std::string> s_aiQueue;          // AI-generated texts (expensive to replenish)
static std::queue<std::string> s_fileQueue;         // File-based texts (always available)
static constexpr size_t kSeenHashCapacity = 500;
static RingBuffer<size_t, kSeenHashCapacity> s_seenHashes;
static std::mutex s_mutex;

static double s_nextGenTime = 0;
static double s_responseTime = 0;

#pragma mark - Simple Hash

static size_t SimpleHash(const std::string& s) {
    return std::hash<std::string>{}(s);
}

static bool SeenHash(size_t hash) {
    for (size_t i = 0; i < s_seenHashes.size(); ++i) {
        if (s_seenHashes[i] == hash) return true;
    }
    return false;
}

#pragma mark - Prompt Builder

static std::string GetActiveBehaviorsStr() {
    static const char* kBehaviorKeys[] = {
        "behaviors.fun.ball", "behaviors.fun.breadCrumbs", "behaviors.fun.hats",
        "behaviors.fun.rainbow", "behaviors.fun.acid", "behaviors.fun.anger",
        "behaviors.fun.autumnLeaves", "behaviors.control.honcker", "behaviors.control.jail",
        "behaviors.control.portals", "behaviors.control.drag", "behaviors.info.nametag",
        "behaviors.systems.health", "behaviors.systems.pomodoro",
    };
    std::vector<std::string> active;
    for (const char* key : kBehaviorKeys) {
        const ConfigOption* opt = Config_FindOptionByKey(key);
        if (opt && opt->type == CFG_BOOL && *(bool*)opt->ptr) {
            // Extract short name from key (last component after '.')
            std::string s(key);
            size_t dot = s.rfind('.');
            active.push_back(dot != std::string::npos ? s.substr(dot + 1) : s);
        }
    }
    if (active.empty()) return "none";
    std::string result;
    for (size_t i = 0; i < active.size(); i++) {
        if (i > 0) result += ", ";
        result += active[i];
    }
    return result;
}

static std::string GetColorModeStr() {
    switch (g_config.general.appearanceMode) {
        case 0: return "light";
        case 1: return "dark";
        case 2: return "system";
        case 3: return "custom";
        default: return "unknown";
    }
}

static std::string GetEvilPersonality(float level) {
    return Config_EvilPersonality(level);
}

static std::string BuildPrompt() {
    int seed = (rand() << 16) ^ rand();
    float evilLevel = g_config.ai.evilLevel;
    std::string personality = GetEvilPersonality(evilLevel);
    std::string behaviors = GetActiveBehaviorsStr();
    std::string colorMode = GetColorModeStr();

    std::ostringstream prompt;
    prompt << "You are " << personality
           << ". Generate ONE short, funny text message that a goose like you would leave behind."
           << " Current active behaviors: " << behaviors
           << ". Color theme: " << colorMode
           << ". Random seed: " << seed
           << ". Be creative and absurd. Output ONLY the message text, nothing else. No quotes. Max 120 characters.";
    return prompt.str();
}

#pragma mark - HTTP Request

// Forward declaration
static void TryLocalGeneration(const std::string& prompt, float temperature);

static void SendGenerateRequest(const std::string& prompt, float temperature) {
    s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 2.0;
    NSString* nsPrompt = [NSString stringWithUTF8String:prompt.c_str()];

    NSString* endpoint = @"";
    NSString* model = @"";
    switch (g_config.ai.providerType) {
        case 0:
            // Foundation: use local CoreML LLM if available, otherwise fall back to file texts
            LocalLLM_Init();
            if (LocalLLM_GetState() == LocalLLMState::Ready) {
                fprintf(stderr, "[AITEXT] Foundation provider: routing to local CoreML LLM\n");
                TryLocalGeneration(prompt, temperature);
            } else {
                fprintf(stderr, "[AITEXT] Foundation provider: no local model available, using file texts\n");
                // Don't set s_nextGenTime here - let file queue handle it
                // File texts are always available via Dequeue fallback
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 5.0; // Gentle cooldown
            }
            return;
        case 1:
            endpoint = [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.osaurusPort];
            model = [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
            break;
        case 2:
            endpoint = [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.ollamaPort];
            model = [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
            break;
        case 3:
            endpoint = [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
            model = [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
            break;
        default:
            endpoint = @"http://localhost:1337/v1/chat/completions";
            model = @"foundation";
            break;
    }

    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) return;

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    request.timeoutInterval = 60;

    NSDictionary* body = @{
        @"model": model,
        @"messages": @[
            @{@"role": @"system", @"content": @"You generate short, funny text messages that a goose would leave behind. Output ONLY the message text, no quotes, no explanations. Max 120 characters."},
            @{@"role": @"user", @"content": nsPrompt}
        ],
        @"max_tokens": @(150),
        @"temperature": @(temperature)
    };

    NSError* jsonErr;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:body options:0 error:&jsonErr];
    if (jsonErr) return;
    [request setHTTPBody:jsonData];

    NSDate* startTime = [NSDate date];

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        double elapsed = [[NSDate date] timeIntervalSinceDate:startTime];
        fprintf(stderr, "[AITEXT] Response in %.1fs\n", elapsed);
        double cooldown = elapsed * 1.5;
        if (cooldown < 2.0) cooldown = 2.0;

        dispatch_async(dispatch_get_main_queue(), ^{
            s_responseTime = elapsed;

            if (error) {
                fprintf(stderr, "[AITEXT] Request failed: %s\n", error.localizedDescription.UTF8String);
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;
                return;
            }

            NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
            if (httpResp.statusCode != 200) {
                fprintf(stderr, "[AITEXT] HTTP %ld\n", (long)httpResp.statusCode);
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;
                return;
            }

            NSError* parseErr;
            NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseErr];
            if (parseErr || !json) {
                fprintf(stderr, "[AITEXT] Parse error\n");
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;
                return;
            }

            NSString* content = json[@"choices"][0][@"message"][@"content"];
            if (!content || content.length == 0) {
                fprintf(stderr, "[AITEXT] Empty response\n");
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;
                return;
            }

            content = [content stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
            content = [content stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\"'"]];

            std::string text = std::string([content UTF8String]);
            size_t hash = SimpleHash(text);

            std::lock_guard<std::mutex> lock(s_mutex);
            if (SeenHash(hash)) {
                fprintf(stderr, "[AITEXT] Duplicate text, skipping\n");
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;
                return;
            }

            s_seenHashes.push(hash);
            s_aiQueue.push(text);
            int queueSize = (int)s_aiQueue.size();
            s_nextGenTime = [[NSDate date] timeIntervalSince1970] + cooldown;

            fprintf(stderr, "[AITEXT] Generated: \"%s\" (ai_queue=%d, next=%.1fs)\n", text.c_str(), queueSize, cooldown);

            if (g_config.ai.textMemeAutoSave) {
                @autoreleasepool {
                    std::filesystem::path saveDir = ConfigDirPath() / "TextMemes";
                    std::filesystem::create_directories(saveDir);

                    time_t now = std::time(nullptr);
                    int seed = (rand() << 16) ^ rand();
                    char hashStr[9];
                    snprintf(hashStr, sizeof(hashStr), "%08zx", hash);
                    char filename[128];
                    snprintf(filename, sizeof(filename), "%lld_%d_%s.txt", (long long)now, seed, hashStr);

                    std::string fullPath = (saveDir / filename).string();
                    FILE* f = fopen(fullPath.c_str(), "w");
                    if (f) {
                        fprintf(f, "%s", text.c_str());
                        fclose(f);
                        fprintf(stderr, "[AITEXT] Saved to %s\n", fullPath.c_str());
                    }
                }
            }
        });
    }];
    [task resume];
}

#pragma mark - Local LLM Fallback

static void TryLocalGeneration(const std::string& prompt, float temperature) {
    s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 2.0;

    LocalLLM_Generate(prompt, temperature, ^(const std::string& result) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result.empty()) {
                fprintf(stderr, "[AITEXT] Local LLM returned empty, falling back to file texts\n");
                // Fall back to file texts - they're always available via Dequeue
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 5.0;
                return;
            }

            size_t hash = SimpleHash(result);
            std::lock_guard<std::mutex> lock(s_mutex);
            if (SeenHash(hash)) {
                fprintf(stderr, "[AITEXT] Duplicate local text, skipping\n");
                s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 5.0;
                return;
            }

            s_seenHashes.push(hash);
            s_aiQueue.push(result);
            s_nextGenTime = [[NSDate date] timeIntervalSince1970] + 10.0;
            fprintf(stderr, "[AITEXT] Local LLM: \"%s\" (ai_queue=%d)\n", result.c_str(), (int)s_aiQueue.size());
        });
    });
}

#pragma mark - File Text Loading

static void LoadTextsFromDirectory(const std::filesystem::path& dir, std::queue<std::string>& queue) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".txt") continue;
        std::ifstream file(entry.path());
        if (!file) continue;
        std::stringstream buf;
        buf << file.rdbuf();
        std::string text = buf.str();
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
        if (!text.empty()) queue.push(text);
    }
}

void AI_TextMeme_LoadFileTexts() {
    std::lock_guard<std::mutex> lock(s_mutex);
    while (!s_fileQueue.empty()) s_fileQueue.pop();

    LoadTextsFromDirectory(ConfigDirPath() / "TextMemes", s_fileQueue);
    LoadTextsFromDirectory(ASSET_ROOT / "Assets" / "Text" / "NotepadMessages", s_fileQueue);

    fprintf(stderr, "[AITEXT] Loaded %zu file texts\n", s_fileQueue.size());
}

#pragma mark - Public Interface

void AI_TextMeme_Tick(double time) {
    if (!g_config.ai.textMemeEnabled) return;

    double now = [[NSDate date] timeIntervalSince1970];
    if (now < s_nextGenTime) return;

    std::lock_guard<std::mutex> lock(s_mutex);
    if ((int)s_aiQueue.size() >= g_config.ai.textMemeMaxQueue) return;

    std::string prompt = BuildPrompt();
    float temp = g_config.ai.textMemeTemperature;

    // Try local CoreML LLM when local LLM is enabled and no HTTP provider is configured
    if (g_config.ai.localLlmEnabled) {
        LocalLLM_Init();
        if (LocalLLM_GetState() == LocalLLMState::Ready) {
            fprintf(stderr, "[AITEXT] Generating via local CoreML... (ai_queue=%d)\n", (int)s_aiQueue.size());
            TryLocalGeneration(prompt, temp);
            return;
        }
    }

    fprintf(stderr, "[AITEXT] Generating via HTTP... (ai_queue=%d, temp=%.1f)\n", (int)s_aiQueue.size(), temp);
    SendGenerateRequest(prompt, temp);
}

bool AI_TextMeme_HasAvailable() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return !s_aiQueue.empty() || !s_fileQueue.empty();
}

std::string AI_TextMeme_Dequeue() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_aiQueue.empty()) {
        std::string text = s_aiQueue.front();
        s_aiQueue.pop();
        return text;
    }
    if (!s_fileQueue.empty()) {
        std::string text = s_fileQueue.front();
        s_fileQueue.pop();
        // Reload when we're low
        if (s_fileQueue.size() < 5) {
            // Schedule async reload
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
                AI_TextMeme_LoadFileTexts();
            });
        }
        return text;
    }
    return "";
}

int AI_TextMeme_QueueSize() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return (int)(s_aiQueue.size() + s_fileQueue.size());
}

void AI_TextMeme_Reset() {
    std::lock_guard<std::mutex> lock(s_mutex);
    while (!s_aiQueue.empty()) s_aiQueue.pop();
    while (!s_fileQueue.empty()) s_fileQueue.pop();
    s_seenHashes.clear();
    s_nextGenTime = 0;
    s_responseTime = 0;
}

void AI_TextMeme_Inject(const std::string& text) {
    size_t hash = SimpleHash(text);
    std::lock_guard<std::mutex> lock(s_mutex);
    if (SeenHash(hash)) return;
    s_seenHashes.push(hash);
    s_aiQueue.push(text);
}

#endif
