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
#include "crash_logger.h"
#include "log.h"
#include "cursor_backend.h"
#include "world.h"
#include "command_socket.h"
#include "mcp_server.h"
#include "app_actions.h"
#include "app_cli.h"
#include "single_instance.h"
#include "window.h"
#include "audio.h"
#import "tick_manager.h"
#import "behavior_element_window.h"
#import "item_window.h"
#import "effect_window.h"
#include "goose.h"
#include "actor.h"
#include "assets.h"

#import "app_delegate.h"
extern bool g_debugMode;
bool g_mcpMode = false;
static FILE* g_logFile = nullptr;

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

// Debug printouts: compiled out entirely in release (CG_DISABLE_DEBUG_LOG),
// otherwise gated on g_debugMode at runtime.
#if defined(CG_DISABLE_DEBUG_LOG)
#define DEBUG_LOG(fmt, ...) ((void)0)
#else
#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) LogWrite("DEBUG", fmt, ##__VA_ARGS__); \
} while(0)
#endif

extern "C" void AI_OpenChat(const char* gooseName);
extern "C" void AI_SendMessage(const char* message);
extern "C" void UpdateStatusBarIcon();

bool Config_IsSystemDarkTheme();



extern "C" void UpdateStatusBarIcon() {
    AppDelegate* delegate = (AppDelegate*)[NSApp delegate];
    if (!delegate.statusItem) return;

    int mode = g_config.general.appearanceMode;
    bool isStalin = (mode == APPEARANCE_STALIN);
    bool isCanada = (mode == 1) || (mode == 2 && Config_IsSystemDarkTheme());

    std::string menubarFile = "menubar_goose_white.png";
    std::string appIconFile = "app_icon_white.png";

    if (isStalin) {
        menubarFile = "menubar_stalin.png";
        appIconFile = "app_icon_stalin.png";
    } else if (isCanada) {
        menubarFile = "menubar_goose_canada.png";
        appIconFile = "app_icon_canada.png";
    }

    std::string mbPath = (ASSET_ROOT / "Assets/Images/OtherGfx" / menubarFile).string();
    std::string appPath = (ASSET_ROOT / "Assets/Images/OtherGfx" / appIconFile).string();

    NSImage* mbImg = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:mbPath.c_str()]];
    NSImage* appImg = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:appPath.c_str()]];

    if (mbImg) {
        [mbImg setTemplate:YES];
        delegate.statusItem.button.image = mbImg;
        delegate.statusItem.button.title = @""; // Clear title since we are using image
    } else {
        // Fallback to text emojis if image fails to load
        delegate.statusItem.button.image = nil;
        delegate.statusItem.button.title = isStalin ? @"\u262D" : (isCanada ? @"\U0001F341" : @"\U0001FABF");
    }

    if (appImg) {
        [NSApp setApplicationIconImage:appImg];
    } else {
        [NSApp setApplicationIconImage:nil]; // reset to default Info.plist AppIcon
    }
}

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
        auto geese = ActorManager::Instance().getGeese();
        for (auto* g : geese) {
            if (g && g->m_perGooseWindow) {
                BehaviorElementWindow* win = (__bridge BehaviorElementWindow*)g->m_perGooseWindow;
                if (visible) {
                    [win orderFront:nil];
                } else {
                    [win orderOut:nil];
                }
            }
        }
    });
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    if (g_debugMode) OpenLogFile();
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

    // Single-instance guard for the GUI goose. Race-free (unlike the socket
    // ping in AppCli_HandleCommand) and self-healing on crash. MCP stdio mode
    // is exempt — it renders no geese and may run alongside the app.
    if (!g_mcpMode && !SingleInstance_Acquire()) {
        fprintf(stderr, "Desktop Goose is already running.\n");
        return 0;
    }

    // Quiet by default; verbose with --debug or CADGOOSE_VERBOSE.
    Log_InitLevel(g_debugMode);

    // Install crash/log capture for the GUI (and MCP) run. Placed after CLI
    // handling so quick CLI commands that exit early don't get stderr
    // redirected to a session log.
    CrashLogger_Init();

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
    Log_Shutdown();
    return 0;
}