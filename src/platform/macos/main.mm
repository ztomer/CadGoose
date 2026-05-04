#import <Cocoa/Cocoa.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include "config.h"
#include "cursor_backend.h"
#include "world.h"
#include "command_socket.h"
#include "app_actions.h"
#include "src/macos/window.h"
#include "src/macos/audio.h"

static bool g_debugMode = false;

#define DEBUG_LOG(fmt, ...) do { if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    DEBUG_LOG("App launching...");
    
    Config_InitRegistry();
    DEBUG_LOG("Config init done");
    
    g_backendManager.Init();
    DEBUG_LOG("Backend: %s", g_backendManager.GetActiveBackend()->Name().c_str());

    DEBUG_LOG("Creating windows for %lu screens", (unsigned long)[NSScreen screens].count);
    [[WindowManager shared] createWindowsForAllScreens];
    
    NSArray* windows = [[WindowManager shared] allWindows];
    DEBUG_LOG("Windows created: %lu", (unsigned long)windows.count);

    for (GooseWindow* window in windows) {
        DEBUG_LOG("Window frame: %s", NSStringFromRect(window.frame).UTF8String);
        DEBUG_LOG("Window level: %ld", (long)window.level);
        
        [window.gooseView startAnimation];
        [window orderFront:nil];
        DEBUG_LOG("Window ordered front");
    }

    std::string error;
    if (!CommandSocket_StartServer(AppActions_HandleCommand, &error) && !error.empty()) {
        DEBUG_LOG("Socket error: %s", error.c_str());
    }

    AppActions_EnsureInitialGoose();
    DEBUG_LOG("Geese spawned: %zu", g_geese.size());
    
    for (auto& g : g_geese) {
        DEBUG_LOG("Goose %d at %.1f,%.1f", g.id, g.pos.x, g.pos.y);
    }

    g_screenWidth = (int)[[NSScreen mainScreen] frame].size.width;
    g_screenHeight = (int)[[NSScreen mainScreen] frame].size.height;
    DEBUG_LOG("Screen: %dx%d", g_screenWidth, g_screenHeight);

    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    DEBUG_LOG("App ready, entering run loop...");
}

- (void)applicationWillTerminate:(NSNotification*)aNotification {
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

static bool IsControlCommand(const std::string& command) {
    return command == "start" ||
           command == "spawn" ||
           command == "clear" ||
           command == "ram" ||
           command == "status" ||
           command == "quit";
}

static bool IsRunning() {
    std::string response;
    return CommandSocket_Send({"status"}, &response, nullptr);
}

static int DaemonizeProcess() {
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork" << std::endl;
        return 1;
    }
    if (pid > 0) {
        exit(0);
    }
    if (setsid() < 0) {
        std::cerr << "Failed to create session" << std::endl;
        return 1;
    }
    std::cout << "Desktop Goose started in background" << std::endl;
    return 0;
}

static int HandleCliCommand(int argc, char** argv, int* appArgc) {
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            g_debugMode = true;
            DEBUG_LOG("Debug mode enabled");
        }
    }

    if (argc <= 1) {
        if (IsRunning()) {
            std::cout << "Desktop Goose is already running" << std::endl;
            return 0;
        }
        *appArgc = 1;
        return -1;
    }

    const std::string command = argv[1];
    if (command == "--foreground") {
        *appArgc = 1;
        return -1;
    }

    if (command == "--help" || command == "help") {
        std::cout
            << "Desktop Goose commands:\n"
            << "  CppGoose\n"
            << "  CppGoose --debug\n"
            << "  CppGoose start\n"
            << "  CppGoose start --foreground\n"
            << "  CppGoose spawn [name]\n"
            << "  CppGoose clear\n"
            << "  CppGoose ram\n"
            << "  CppGoose status\n"
            << "  CppGoose quit\n";
        return 0;
    }

    if (!IsControlCommand(command)) return -1;

    if (command == "start") {
        if (argc > 2 && std::string(argv[2]) == "--foreground") {
            *appArgc = 1;
            return -1;
        }

        if (IsRunning()) {
            std::cout << "Desktop Goose is already running" << std::endl;
            return 0;
        }

        return DaemonizeProcess();
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    std::string response;
    std::string error;
    if (!CommandSocket_Send(args, &response, &error)) {
        std::cerr << error << std::endl;
        return 1;
    }

    if (!response.empty()) std::cout << response;
    return 0;
}

int main(int argc, char** argv) {
    fprintf(stderr, "[DEBUG] main() starting, argc=%d\n", argc);
    fflush(stderr);
    for (int i = 0; i < argc; i++) {
        fprintf(stderr, "[DEBUG] argv[%d]=%s\n", i, argv[i]);
    }
    fflush(stderr);
    
    char* runArgv[] = { argv[0], nullptr };
    int runArgc = 1;

    const int cliStatus = HandleCliCommand(argc, argv, &runArgc);
    fprintf(stderr, "[DEBUG] HandleCliCommand returned %d\n", cliStatus);
    if (cliStatus >= 0) return cliStatus;

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