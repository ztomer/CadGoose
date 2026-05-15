// local_llm_tokenizer.mm
// Tokenizer loading, encoding, and decoding for local CoreML LLM
#include "local_llm.h"
#include "config.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

#include <unordered_map>
#include <vector>
#include <cstdio>

static std::unordered_map<std::string, int> s_vocab;
static std::unordered_map<int, std::string> s_idToToken;
static bool s_tokenizerReady = false;

void LocalLLM_LoadTokenizer(NSString* baseDir) {
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

std::vector<int> LocalLLM_EncodeText(const std::string& text) {
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

std::string LocalLLM_DecodeTokens(const std::vector<int>& tokens) {
    std::string result;
    for (int t : tokens) {
        auto it = s_idToToken.find(t);
        if (it != s_idToToken.end()) result += it->second;
    }
    return result;
}

bool LocalLLM_IsTokenizerReady() { return s_tokenizerReady; }
int LocalLLM_VocabSize() { return (int)s_vocab.size(); }
int LocalLLM_GetTokenId(const std::string& token) {
    auto it = s_vocab.find(token);
    return it != s_vocab.end() ? it->second : -1;
}
void LocalLLM_ClearTokenizer() {
    s_vocab.clear();
    s_idToToken.clear();
    s_tokenizerReady = false;
}
