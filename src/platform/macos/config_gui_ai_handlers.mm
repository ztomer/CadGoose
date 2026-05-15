// config_gui_ai_handlers.mm
// AITabView event handlers
#import "config_gui_helpers.h"
#include "config.h"
#include "mcp_server.h"
#include "ai_text_meme.h"

extern "C" void AI_RefreshModelDisplay();

static constexpr float kMinMcpPort = 1024;
static constexpr float kMaxMcpPort = 65535;
static constexpr float kDefaultMcpPort = 31072;

@implementation AITabView (Handlers)

- (void)providerChanged:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    [self setProvider:idx];
    self.portField.integerValue = [self currentPort];
    [self.modelPopup removeAllItems];
    [self.modelPopup addItemWithTitle:[self currentModelName]];
    [self updateCustomVisibility];
    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)portChanged:(NSTextField*)sender {
    [self setPort:(int)sender.integerValue];
    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)modelPopupChanged:(NSPopUpButton*)sender {
    NSString* selected = [sender titleOfSelectedItem];
    if (selected && ![selected hasPrefix:@"\U0001F300"] && ![selected hasPrefix:@"\u274C"] && ![selected isEqualToString:@"(none)"]) {
        [self setModelName:selected];
        Config_SaveAll();
    }
}

- (void)endpointChanged:(NSTextField*)sender {
    g_config.ai.customEndpoint = std::string([sender.stringValue UTF8String]);
    Config_SaveAll();
}

- (void)customModelChanged:(NSTextField*)sender {
    g_config.ai.customModel = std::string([sender.stringValue UTF8String]);
    Config_SaveAll();
}

- (void)showStatusBarToggled:(NSButton*)sender {
    g_config.ai.showStatusBar = (sender.state == NSControlStateValueOn);
    AI_RefreshModelDisplay();
    Config_SaveAll();
}

- (void)mcpToggled:(NSButton*)sender {
    g_config.ai.enableMCP = (sender.state == NSControlStateValueOn);
    if (g_config.ai.enableMCP) {
        MCP_StartInternalServer();
        MCP_StartHTTPServer();
    } else {
        MCP_StopHTTPServer();
        MCP_StopInternalServer();
    }
    Config_SaveAll();
}

- (void)mcpPortChanged:(NSTextField*)sender {
    int port = [sender.stringValue intValue];
    if (port < kMinMcpPort || port > kMaxMcpPort) port = (int)kDefaultMcpPort;
    g_config.ai.mcpPort = port;
    Config_SaveAll();
}

- (void)textMemeToggled:(NSButton*)sender {
    g_config.ai.textMemeEnabled = (sender.state == NSControlStateValueOn);
    if (g_config.ai.textMemeEnabled) {
        AI_TextMeme_Reset();
    }
    Config_SaveAll();
}

- (void)textMemeTempChanged:(NSSlider*)sender {
    g_config.ai.textMemeTemperature = (float)sender.doubleValue;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSTextField class]] && [subview isNotEqualTo:sender] && ((NSTextField*)subview).tag == 301) {
            ((NSTextField*)subview).stringValue = [NSString stringWithFormat:@"%.1f", g_config.ai.textMemeTemperature];
            break;
        }
    }
    Config_SaveAll();
}

- (void)textMemeAutoSaveToggled:(NSButton*)sender {
    g_config.ai.textMemeAutoSave = (sender.state == NSControlStateValueOn);
    Config_SaveAll();
}

@end
