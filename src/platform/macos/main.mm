#import <Cocoa/Cocoa.h>
#import <dispatch/dispatch.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "config_gui.h"
#include "cursor_backend.h"
#include "world.h"
#include "command_socket.h"
#include "mcp_server.h"
#include "app_actions.h"
#include "app_cli.h"
#include "window.h"
#include "renderer.h"
#include "audio.h"

// --- Menu bar icon constants ---
static constexpr float kMenuBarIconSize = 16;
static constexpr float kSpeakerX1 = 4, kSpeakerY1 = 5;
static constexpr float kSpeakerX2 = 4, kSpeakerY2 = 11;
static constexpr float kSpeakerX3 = 8, kSpeakerY3 = 11;
static constexpr float kSpeakerX4 = 12, kSpeakerY4 = 14;
static constexpr float kSpeakerX5 = 12, kSpeakerY5 = 2;
static constexpr float kSpeakerX6 = 8, kSpeakerY6 = 5;
static constexpr float kSlashLineWidth = 2;
static constexpr float kSlashX1 = 2, kSlashY1 = 14;
static constexpr float kSlashX2 = 14, kSlashY2 = 2;
static constexpr float kWaveLineWidth = 1.5;
static constexpr float kWave1ArcX = 14, kWave1ArcY1 = 5, kWave1ArcY2 = 11, kWave1Radius = 3;
static constexpr float kWave2ArcX = 15, kWave2ArcY1 = 3, kWave2ArcY2 = 13, kWave2Radius = 5;
#include "ai_text_meme.h"

extern bool g_debugMode;
static bool g_mcpMode = false;
FILE* g_logFile = nullptr;

void OpenLogFile() {
    if (g_logFile) return;
    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    char path[256];
    snprintf(path, sizeof(path), "/tmp/CadGoose_%04d%02d%02d_%02d%02d%02d.log",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    g_logFile = fopen(path, "w");
    if (g_logFile) {
        fprintf(stderr, "[LOG] Log file: %s\n", path);
    }
}

void LogWrite(const char* level, const char* fmt, ...) {
    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm);

    va_list args;
    va_start(args, fmt);

    if (g_logFile) {
        fprintf(g_logFile, "[%s] %s: ", timestamp, level);
        vfprintf(g_logFile, fmt, args);
        fprintf(g_logFile, "\n");
        fflush(g_logFile);
    }
    fprintf(stderr, "[%s] %s: ", timestamp, level);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

#define LOG(fmt, ...) LogWrite("INFO", fmt, ##__VA_ARGS__)

#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) LogWrite("DEBUG", fmt, ##__VA_ARGS__); \
} while(0)

extern "C" void AI_OpenChat(const char* gooseName);
extern "C" void AI_SendMessage(const char* message);

bool Config_IsSystemDarkTheme();

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSStatusItem* statusItem;
@property (nonatomic, strong) NSMenuItem* muteMenuItem;
- (void)setupMenubar;
- (void)addBehaviorItem:(NSString*)title configKey:(NSString*)key toMenu:(NSMenu*)menu;
- (void)toggleBehavior:(NSMenuItem*)sender;
- (bool*)getBehaviorFlag:(NSString*)key;
- (void)openPresencePanel:(id)sender;
- (void)toggleMute:(id)sender;
- (void)updateMuteMenuItem;
@end

@implementation AppDelegate

- (void)setupMenubar {
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];

    {
        bool darkAppearance = (g_config.general.appearanceMode == APPEARANCE_DARK) ||
            (g_config.general.appearanceMode == APPEARANCE_SYSTEM && Config_IsSystemDarkTheme());
        self.statusItem.button.title = darkAppearance ? @"\U0001F341" : @"\U0001FABF";
    }

    NSMenu* menu = [[NSMenu alloc] init];

    [menu addItemWithTitle:@"About CadGoose" action:@selector(showAbout:) keyEquivalent:@""];
    [menu addItem:[NSMenuItem separatorItem]];

    [menu addItemWithTitle:@"Spawn Goose" action:@selector(spawnGoose:) keyEquivalent:@"g"];
    [menu addItemWithTitle:@"Clear All Geese" action:@selector(clearGeese:) keyEquivalent:@""];

    [menu addItem:[NSMenuItem separatorItem]];

    [menu addItemWithTitle:@"Honk!" action:@selector(testHonk:) keyEquivalent:@"h"];

    self.muteMenuItem = [menu addItemWithTitle:@"" action:@selector(toggleMute:) keyEquivalent:@""];
    [self updateMuteMenuItem];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* prefsItem = [menu addItemWithTitle:@"Preferences..." action:@selector(openPreferences:) keyEquivalent:@","];
    prefsItem.target = self;

    NSMenuItem* chatItem = [menu addItemWithTitle:@"AI Chat" action:@selector(openAIChat:) keyEquivalent:@";"];
    chatItem.target = self;
    chatItem.keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagShift;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quitItem = [menu addItemWithTitle:@"Quit" action:@selector(quitApp:) keyEquivalent:@"q"];
    quitItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;

    self.statusItem.menu = menu;
    DEBUG_LOG("Menubar created");
}

- (void)reloadConfiguration:(id)sender {
    Config_Init();
    DEBUG_LOG("Configuration reloaded");
}

- (void)openPreferences:(id)sender {
    ConfigGUI_ShowWindow();
}

- (void)openAIChat:(id)sender {
    if (!g_world.geese.empty()) {
        AI_OpenChat(g_world.geese.front().name.c_str());
    } else {
        AI_OpenChat("Goose");
    }
}

- (void)showAbout:(id)sender {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"CadGoose";
    alert.informativeText = @"Desktop Goose for macOS\nA playful desktop companion that honks, steals your cursor, and leaves muddy footprints.";
    [alert runModal];
}

- (void)spawnGoose:(id)sender {
    AppActions_SpawnGoose("");
    DEBUG_LOG("Spawned goose from menubar");
}

- (void)clearGeese:(id)sender {
    AppActions_ClearGeese();
    DEBUG_LOG("Cleared geese from menubar");
}

- (void)testHonk:(id)sender {
    Audio_Init();
    Audio_PlayHonk();
    DEBUG_LOG("Test honk from menubar");
}

- (void)updateMuteMenuItem {
    NSSize size = NSMakeSize(kMenuBarIconSize, kMenuBarIconSize);
    NSImage* icon = [[NSImage alloc] initWithSize:size];
    [icon lockFocus];
    NSRect rect = NSMakeRect(0, 0, size.width, size.height);
    [[NSColor labelColor] set];
    
    // Speaker shape: trapezoid + rectangle
    NSBezierPath* speaker = [NSBezierPath bezierPath];
    [speaker moveToPoint:NSMakePoint(kSpeakerX1, kSpeakerY1)];
    [speaker lineToPoint:NSMakePoint(kSpeakerX2, kSpeakerY2)];
    [speaker lineToPoint:NSMakePoint(kSpeakerX3, kSpeakerY3)];
    [speaker lineToPoint:NSMakePoint(kSpeakerX4, kSpeakerY4)];
    [speaker lineToPoint:NSMakePoint(kSpeakerX5, kSpeakerY5)];
    [speaker lineToPoint:NSMakePoint(kSpeakerX6, kSpeakerY6)];
    [speaker closePath];
    [speaker fill];
    
    if (g_config.general.audioMuted) {
        // Add slash for muted state
        NSBezierPath* slash = [NSBezierPath bezierPath];
        [slash setLineWidth:kSlashLineWidth];
        [slash moveToPoint:NSMakePoint(kSlashX1, kSlashY1)];
        [slash lineToPoint:NSMakePoint(kSlashX2, kSlashY2)];
        [slash stroke];
    } else {
        // Add sound waves for unmuted state
        NSBezierPath* wave1 = [NSBezierPath bezierPath];
        [wave1 setLineWidth:kWaveLineWidth];
        [wave1 appendBezierPathWithArcFromPoint:NSMakePoint(kWave1ArcX, kWave1ArcY1) toPoint:NSMakePoint(kWave1ArcX, kWave1ArcY2) radius:kWave1Radius];
        [wave1 stroke];
        
        NSBezierPath* wave2 = [NSBezierPath bezierPath];
        [wave2 setLineWidth:kWaveLineWidth];
        [wave2 appendBezierPathWithArcFromPoint:NSMakePoint(kWave2ArcX, kWave2ArcY1) toPoint:NSMakePoint(kWave2ArcX, kWave2ArcY2) radius:kWave2Radius];
        [wave2 stroke];
    }
    
    [icon unlockFocus];
    self.muteMenuItem.image = icon;
    self.muteMenuItem.title = g_config.general.audioMuted ? @"Unmute" : @"Mute";
}

- (void)toggleMute:(id)sender {
    g_config.general.audioMuted = !g_config.general.audioMuted;
    [self updateMuteMenuItem];
    DEBUG_LOG("Audio muted: %d", g_config.general.audioMuted);
}

- (void)quitApp:(id)sender {
    Config_SaveGooseNames();
    [[NSApplication sharedApplication] terminate:nil];
}

- (void)addBehaviorItem:(NSString*)title configKey:(NSString*)key toMenu:(NSMenu*)menu {
    NSMenuItem* item = [menu addItemWithTitle:title action:@selector(toggleBehavior:) keyEquivalent:@""];
    item.target = self;
    item.representedObject = key;

    bool* flag = [self getBehaviorFlag:key];
    if (flag) {
        item.state = *flag ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (bool*)getBehaviorFlag:(NSString*)key {
    const char* k = [key UTF8String];
    std::string s = k ? k : "";
    const ConfigOption* opt = Config_FindOptionByKey(s);
    if (opt && opt->type == CFG_BOOL) return (bool*)opt->ptr;
    return nullptr;
}

- (void)toggleBehavior:(NSMenuItem*)sender {
    bool* flag = [self getBehaviorFlag:sender.representedObject];
    if (flag) {
        *flag = !*flag;
        sender.state = *flag ? NSControlStateValueOn : NSControlStateValueOff;
        DEBUG_LOG("Behavior '%s' toggled: %d", [sender.representedObject UTF8String], *flag);
    }
}

- (void)openPresencePanel:(id)sender {
    DEBUG_LOG("Opening presence panel");
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    DEBUG_LOG("App launching...");

    Config_Init();
    Config_LoadAll();
    DEBUG_LOG("Config init done");
    if (g_config.ai.enableMCP && !g_mcpMode) {
        MCP_StartInternalServer();
        MCP_StartHTTPServer();
    }

    if (g_mcpMode) {
        DEBUG_LOG("Starting MCP stdio server...");
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            MCP_RunStdioServer();
            dispatch_async(dispatch_get_main_queue(), ^{
                [[NSApplication sharedApplication] terminate:nil];
            });
        });
        return;
    }

    g_assets.Init();
    DEBUG_LOG("Assets init done");

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        AI_TextMeme_LoadFileTexts();
        DEBUG_LOG("AI text memes loaded");
    });

    g_backendManager.Init();
    DEBUG_LOG("Backend: %s", g_backendManager.GetActiveBackend()->Name().c_str());

    DEBUG_LOG("Creating windows for %lu screens", (unsigned long)[NSScreen screens].count);

    WindowManager* wm = [WindowManager shared];
    DEBUG_LOG("WindowManager instance: %p", wm);

    [wm createWindowsForAllScreens];
    DEBUG_LOG("createWindowsForAllScreens completed");

    NSArray* windows = [wm windows];
    DEBUG_LOG("Windows created: %lu", (unsigned long)windows.count);

    if (windows.count == 0) {
        LOG("ERROR: No windows created!");
    }

    for (GooseWindow* window in windows) {
        DEBUG_LOG("Processing window: %p", window);
        DEBUG_LOG("  frame: %s", NSStringFromRect(window.frame).UTF8String);
        DEBUG_LOG("  level: %ld", (long)window.level);
        DEBUG_LOG("  opaque: %d", window.opaque);
        DEBUG_LOG("  ignoresMouseEvents: %d", window.ignoresMouseEvents);

        GooseView* view = window.gooseView;
        DEBUG_LOG("  gooseView: %p", view);
        DEBUG_LOG("  gooseView.frame: %s", NSStringFromRect(view.frame).UTF8String);

        [view startAnimation];
        DEBUG_LOG("  startAnimation done");

        [window orderFront:nil];
        DEBUG_LOG("  orderFront done");

        DEBUG_LOG("Window setup complete for window %p", window);
    }

    LOG("Window setup complete, count: %lu", (unsigned long)windows.count);

    std::string error;
    // Wrapper to add macOS-specific socket commands
    auto cmdHandler = [](const std::vector<std::string>& args) -> std::string {
        if (!args.empty() && (args[0] == "prefs" || args[0] == "openprefs")) {
            dispatch_async(dispatch_get_main_queue(), ^{ ConfigGUI_ShowWindow(); });
            return "ok\n";
        }
        if (!args.empty() && args[0] == "honk") {
            dispatch_async(dispatch_get_main_queue(), ^{ Audio_Init(); Audio_PlayHonk(); });
            return "ok honk\n";
        }
        if (!args.empty() && args[0] == "send" && args.size() > 1) {
            dispatch_async(dispatch_get_main_queue(), ^{ AI_SendMessage(args[1].c_str()); });
            return "ok message sent\n";
        }
        return AppActions_HandleCommand(args);
    };
    if (!CommandSocket_StartServer(cmdHandler, &error) && !error.empty()) {
        DEBUG_LOG("Socket error: %s", error.c_str());
    }

    g_world.screenWidth = (int)[[NSScreen mainScreen] frame].size.width;
    g_world.screenHeight = (int)[[NSScreen mainScreen] frame].size.height;
    DEBUG_LOG("Screen: %dx%d", g_world.screenWidth, g_world.screenHeight);

    AppActions_EnsureInitialGoose();
    DEBUG_LOG("Geese spawned: %zu", g_world.geese.size());

    for (auto& g : g_world.geese) {
        DEBUG_LOG("Goose %d at %.1f,%.1f", g.id, g.pos.x, g.pos.y);
    }

    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    [self setupMenubar];
    DEBUG_LOG("App ready, entering run loop...");

    if (g_config.behaviors.info.presence && !g_world.geese.empty()) {
        auto& goose = g_world.geese.front();
        const char* stateStr = "?";
        switch (goose.state) {
            case GooseState::WANDER: stateStr = "W"; break;
            case GooseState::FETCHING: stateStr = "F"; break;
            case GooseState::RETURNING: stateStr = "R"; break;
            case GooseState::CHASE_CURSOR: stateStr = "C"; break;
            case GooseState::SNATCH_CURSOR: stateStr = "S"; break;
            default: stateStr = "?"; break;
        }
        self.statusItem.button.title = [NSString stringWithUTF8String:stateStr];
    }
}

- (void)applicationWillTerminate:(NSNotification*)aNotification {
    Config_SaveGooseNames();
    MCP_StopHTTPServer();
    MCP_StopInternalServer();
    CommandSocket_StopServer();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    DEBUG_LOG("applicationShouldTerminateAfterLastWindowClosed called");
    return NO;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)sender hasVisibleWindows:(BOOL)hasVisibleWindows {
    DEBUG_LOG("applicationShouldHandleReopen: hasVisibleWindows=%d", hasVisibleWindows);
    return YES;
}

@end

extern "C" void Presence_UpdateStatusFromBehavior(const char* status) {
    dispatch_async(dispatch_get_main_queue(), ^{
        AppDelegate* delegate = (AppDelegate*)[NSApp delegate];
        if (delegate.statusItem) {
            delegate.statusItem.button.title = [NSString stringWithUTF8String:status];
        }
    });
}

extern "C" void Presence_SetGooseWindowVisible(bool visible) {
    dispatch_async(dispatch_get_main_queue(), ^{
        WindowManager* wm = [WindowManager shared];
        NSArray<GooseWindow*>* windows = [wm windows];
        for (GooseWindow* window in windows) {
            if (visible) {
                [window orderFront:nil];
            } else {
                [window orderOut:nil];
            }
        }
    });
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    OpenLogFile();
    fprintf(stderr, "[DEBUG] main() starting, argc=%d\n", argc);
    fflush(stderr);
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "[DEBUG] argv[%d]=%s\n", i, argv[i]);
    }
    fflush(stderr);

    char* runArgv[] = { argv[0], nullptr };
    int runArgc = 1;

    const int cliStatus = AppCli_HandleCommand(argc, argv, &runArgc);
    fprintf(stderr, "[DEBUG] AppCli_HandleCommand returned %d\n", cliStatus);
    if (cliStatus >= 0) return cliStatus;

    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--mcp") {
            g_mcpMode = true;
            break;
        }
    }

    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];

        // Create menu bar so app is considered "active"
        [NSApp setMainMenu:[[NSMenu alloc] init]];

        fprintf(stderr, "[DEBUG] Got NSApplication: %p, running=%d\n", (__bridge void*)app, [app isRunning]);

        AppDelegate* delegate = [[AppDelegate alloc] init];
        app.delegate = delegate;

        [app finishLaunching];
        fprintf(stderr, "[DEBUG] finishLaunching done, running=%d\n", [app isRunning]);

        [[NSRunningApplication currentApplication] activateWithOptions:0];
        fprintf(stderr, "[DEBUG] activated, running=%d\n", [app isRunning]);

        DEBUG_LOG("Starting run loop...");
        [app run];
        DEBUG_LOG("Run loop exited");
    }

    CommandSocket_StopServer();
    return 0;
}