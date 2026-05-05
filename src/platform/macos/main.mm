#import <Cocoa/Cocoa.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "cursor_backend.h"
#include "world.h"
#include "command_socket.h"
#include "app_actions.h"
#include "window.h"
#include "renderer.h"
#include "audio.h"

bool g_debugMode = false;
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

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSStatusItem* statusItem;
@end

@implementation AppDelegate

- (void)setupMenubar {
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];

    self.statusItem.button.title = g_config.general.canadaGooseMode ? @"\U0001F341" : @"\U0001FABF";

    NSMenu* menu = [[NSMenu alloc] init];

    [menu addItemWithTitle:@"About CadGoose" action:@selector(showAbout:) keyEquivalent:@""];
    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* spawnItem = [menu addItemWithTitle:@"Spawn Goose" action:@selector(spawnGoose:) keyEquivalent:@"g"];
    spawnItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;

    NSMenuItem* clearItem = [menu addItemWithTitle:@"Clear All Geese" action:@selector(clearGeese:) keyEquivalent:@""];

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* honkItem = [menu addItemWithTitle:@"Test Honk" action:@selector(testHonk:) keyEquivalent:@"h"];
    honkItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenu* settingsMenu = [[NSMenu alloc] initWithTitle:@"Settings"];

    NSMenuItem* mudItem = [settingsMenu addItemWithTitle:@"Enable Mud Footprints" action:@selector(toggleMud:) keyEquivalent:@""];
    mudItem.target = self;
    mudItem.state = g_config.mud.enabled ? NSControlStateValueOn : NSControlStateValueOff;

    NSMenuItem* chaseItem = [settingsMenu addItemWithTitle:@"Enable Cursor Chase" action:@selector(toggleChase:) keyEquivalent:@""];
    chaseItem.target = self;
    chaseItem.state = g_config.cursor.chaseEnabled ? NSControlStateValueOn : NSControlStateValueOff;

    NSMenuItem* memeItem = [settingsMenu addItemWithTitle:@"Enable Memes" action:@selector(toggleMemes:) keyEquivalent:@""];
    memeItem.target = self;
    memeItem.state = g_config.general.memesEnabled ? NSControlStateValueOn : NSControlStateValueOff;

    NSMenuItem* canadaItem = [settingsMenu addItemWithTitle:@"Canada Goose Colors" action:@selector(toggleCanadaGoose:) keyEquivalent:@""];
    canadaItem.target = self;
    canadaItem.state = g_config.general.canadaGooseMode ? NSControlStateValueOn : NSControlStateValueOff;

    [settingsMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* openConfigItem = [settingsMenu addItemWithTitle:@"Open Configuration" action:@selector(openConfiguration:) keyEquivalent:@""];
    openConfigItem.target = self;

    NSMenuItem* reloadConfigItem = [settingsMenu addItemWithTitle:@"Reload Configuration" action:@selector(reloadConfiguration:) keyEquivalent:@""];
    reloadConfigItem.target = self;

    NSMenuItem* settingsItem = [menu addItemWithTitle:@"Settings" action:nil keyEquivalent:@""];
    settingsItem.submenu = settingsMenu;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quitItem = [menu addItemWithTitle:@"Quit" action:@selector(quitApp:) keyEquivalent:@"q"];
    quitItem.keyEquivalentModifierMask = NSEventModifierFlagCommand;

    self.statusItem.menu = menu;
    DEBUG_LOG("Menubar created");
}

- (void)openConfiguration:(id)sender {
    NSString* configPath = [NSString stringWithUTF8String:Config_GetPath().c_str()];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:configPath]];
    DEBUG_LOG("Opened configuration file: %s", Config_GetPath().c_str());
}

- (void)reloadConfiguration:(id)sender {
    Config_InitRegistry();
    DEBUG_LOG("Configuration reloaded from menubar");
    // Update menu items states based on reloaded config
    NSMenu* menu = self.statusItem.menu;
    NSMenuItem* settingsItem = [menu itemWithTitle:@"Settings"];
    if (settingsItem && settingsItem.submenu) {
        NSMenu* settingsMenu = settingsItem.submenu;
        [[settingsMenu itemWithTitle:@"Enable Mud Footprints"] setState:(g_config.mud.enabled ? NSControlStateValueOn : NSControlStateValueOff)];
        [[settingsMenu itemWithTitle:@"Enable Cursor Chase"] setState:(g_config.cursor.chaseEnabled ? NSControlStateValueOn : NSControlStateValueOff)];
        [[settingsMenu itemWithTitle:@"Enable Memes"] setState:(g_config.general.memesEnabled ? NSControlStateValueOn : NSControlStateValueOff)];
        [[settingsMenu itemWithTitle:@"Canada Goose Colors"] setState:(g_config.general.canadaGooseMode ? NSControlStateValueOn : NSControlStateValueOff)];
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

- (void)toggleMud:(id)sender {
    g_config.mud.enabled = !g_config.mud.enabled;
    NSMenuItem* item = (NSMenuItem*)sender;
    item.state = g_config.mud.enabled ? NSControlStateValueOn : NSControlStateValueOff;
    DEBUG_LOG("Mud enabled: %d", g_config.mud.enabled);
}

- (void)toggleChase:(id)sender {
    g_config.cursor.chaseEnabled = !g_config.cursor.chaseEnabled;
    NSMenuItem* item = (NSMenuItem*)sender;
    item.state = g_config.cursor.chaseEnabled ? NSControlStateValueOn : NSControlStateValueOff;
    DEBUG_LOG("Cursor chase enabled: %d", g_config.cursor.chaseEnabled);
}

- (void)toggleMemes:(id)sender {
    g_config.general.memesEnabled = !g_config.general.memesEnabled;
    NSMenuItem* item = (NSMenuItem*)sender;
    item.state = g_config.general.memesEnabled ? NSControlStateValueOn : NSControlStateValueOff;
    DEBUG_LOG("Memes enabled: %d", g_config.general.memesEnabled);
}

- (void)toggleCanadaGoose:(id)sender {
    g_config.general.canadaGooseMode = !g_config.general.canadaGooseMode;
    NSMenuItem* item = (NSMenuItem*)sender;
    item.state = g_config.general.canadaGooseMode ? NSControlStateValueOn : NSControlStateValueOff;
    self.statusItem.button.title = g_config.general.canadaGooseMode ? @"\U0001F341" : @"\U0001FABF";
    DEBUG_LOG("Canada Goose mode: %d", g_config.general.canadaGooseMode);
}

- (void)quitApp:(id)sender {
    [[NSApplication sharedApplication] terminate:nil];
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    DEBUG_LOG("App launching...");

    Config_InitRegistry();
    DEBUG_LOG("Config init done");

    g_assets.Init();
    DEBUG_LOG("Assets init done");

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
    if (!CommandSocket_StartServer(AppActions_HandleCommand, &error) && !error.empty()) {
        DEBUG_LOG("Socket error: %s", error.c_str());
    }

    g_screenWidth = (int)[[NSScreen mainScreen] frame].size.width;
    g_screenHeight = (int)[[NSScreen mainScreen] frame].size.height;
    DEBUG_LOG("Screen: %dx%d", g_screenWidth, g_screenHeight);

    AppActions_EnsureInitialGoose();
    DEBUG_LOG("Geese spawned: %zu", g_geese.size());

    for (auto& g : g_geese) {
        DEBUG_LOG("Goose %d at %.1f,%.1f", g.id, g.pos.x, g.pos.y);
    }

    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    [self setupMenubar];
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
            << "  CadGoose\n"
            << "  CadGoose --debug\n"
            << "  CadGoose start\n"
            << "  CadGoose start --foreground\n"
            << "  CadGoose spawn [name]\n"
            << "  CadGoose clear\n"
            << "  CadGoose ram\n"
            << "  CadGoose status\n"
            << "  CadGoose quit\n";
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
    OpenLogFile();
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