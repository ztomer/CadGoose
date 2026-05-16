// config_gui_ai_connection.mm
// AITabView connection testing and model refresh
#import "config_gui_helpers.h"
#include "config.h"
#include "local_llm.h"

static constexpr float kTestTimeout = 30.0f;
static constexpr float kModelRefreshDelay = 0.5f;
static constexpr float kErrorDescMaxLength = 30;
static constexpr float kModelPopupTag = 101;

@implementation AITabView (Connection)

- (NSString*)modelsEndpointForTest {
     NSInteger prov = [self currentProvider];
     if (prov == 0) return @"local://coreml/foundation";
     if (prov == 1) return [NSString stringWithFormat:@"http://localhost:%d/v1/models", g_config.ai.osaurusPort];
     if (prov == 2) return [NSString stringWithFormat:@"http://localhost:%d/api/tags", g_config.ai.ollamaPort];
     if (prov == 3) return [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
     return @"http://localhost:1337/v1/models";
 }

- (void)refreshModels:(id)sender {
    NSInteger prov = [self currentProvider];
    if (prov == 0) return;
    if (prov == 3) return;
    int port = [self currentPort];
    [self.modelPopup removeAllItems];
    [self.modelPopup addItemWithTitle:@"\U0001F300 Loading..."];
    NSString* endpoint = (prov == 1) ? [NSString stringWithFormat:@"http://localhost:%d/v1/models", port]
                                      : [NSString stringWithFormat:@"http://localhost:%d/api/tags", port];
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) return;
    __weak NSPopUpButton* weakPopup = self.modelPopup;
    __weak AITabView* weakSelf = self;
    NSURLSessionDataTask* task = [[NSURLSession sharedSession] dataTaskWithURL:url completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSPopUpButton* strongPopup = weakPopup;
            AITabView* strongSelf = weakSelf;
            if (!strongPopup || !strongSelf) return;
            [strongPopup removeAllItems];
            if (error || !data) {
                [strongPopup addItemWithTitle:[NSString stringWithFormat:@"\u274C %@", error ? [error.localizedDescription substringToIndex:MIN((NSInteger)kErrorDescMaxLength,error.localizedDescription.length)] : @"no data"]];
                return;
            }
            NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (!json) { [strongPopup addItemWithTitle:@"(invalid response)"]; return; }
            if ([json[@"data"] isKindOfClass:[NSArray class]]) {
                for (NSDictionary* m in json[@"data"]) {
                    [strongPopup addItemWithTitle:m[@"id"] ?: @"?"];
                }
            } else if ([json[@"models"] isKindOfClass:[NSArray class]]) {
                for (NSDictionary* m in json[@"models"]) {
                    [strongPopup addItemWithTitle:m[@"name"] ?: m[@"model"] ?: @"?"];
                }
            } else {
                [strongPopup addItemWithTitle:@"(unknown format)"];
            }
            if (strongPopup.numberOfItems == 0) {
                [strongPopup addItemWithTitle:@"(none found)"];
            }
            if (prov == 1 || prov == 2) {
                std::string savedModel = (prov == 1) ? g_config.ai.osaurusModel : g_config.ai.ollamaModel;
                if (!savedModel.empty()) {
                    NSString* savedName = [NSString stringWithUTF8String:savedModel.c_str()];
                    NSInteger idx = [strongPopup indexOfItemWithTitle:savedName];
                    if (idx >= 0) [strongPopup selectItemAtIndex:idx];
                }
            }
        });
    }];
    [task resume];
}

- (void)testConnection:(id)sender {
    if (!self.statusLabel) return;
    NSInteger prov = [self currentProvider];
    if (prov == 0) {
        LocalLLM_Init();
        LocalLLMState state = LocalLLM_GetState();
        switch (state) {
            case LocalLLMState::Ready:
                self.statusLabel.stringValue = @"\u2705 Local LLM ready!";
                self.statusLabel.textColor = [NSColor systemGreenColor];
                break;
            case LocalLLMState::Loading:
                self.statusLabel.stringValue = @"\U0001F300 Local LLM loading...";
                self.statusLabel.textColor = [NSColor systemOrangeColor];
                break;
            case LocalLLMState::Error:
                self.statusLabel.stringValue = @"\u274C Local LLM error";
                self.statusLabel.textColor = [NSColor systemRedColor];
                break;
            case LocalLLMState::Unavailable:
                self.statusLabel.stringValue = @"\u274C No local model found";
                self.statusLabel.textColor = [NSColor systemRedColor];
                break;
        }
        return;
    }
    self.statusLabel.stringValue = @"\U0001F300 Testing...";
    self.statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    NSString* endpoint = [self modelsEndpointForTest];
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) { self.statusLabel.stringValue = @"\u274C Invalid URL"; self.statusLabel.textColor = [NSColor systemRedColor]; return; }
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    request.timeoutInterval = kTestTimeout;
    __weak NSTextField* weakStatus = self.statusLabel;
    [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSTextField* strongStatus = weakStatus;
            if (!strongStatus) return;
            if (error) {
                strongStatus.stringValue = [NSString stringWithFormat:@"\u274C %@", error.localizedDescription];
                strongStatus.textColor = [NSColor systemRedColor];
            } else {
                NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
                if (httpResp.statusCode == 200) {
                    strongStatus.stringValue = @"\u2705 Connected!";
                    strongStatus.textColor = [NSColor systemGreenColor];
                } else {
                    strongStatus.stringValue = [NSString stringWithFormat:@"\u274C HTTP %ld", (long)httpResp.statusCode];
                    strongStatus.textColor = [NSColor systemRedColor];
                }
            }
        });
    }] resume];
}

@end
