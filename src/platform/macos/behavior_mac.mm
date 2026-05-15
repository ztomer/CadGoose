// ===========================
// behavior_mac.mm
// macOS Accessibility API & Failsafe Implementation
// ===========================
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <atomic>
#include "behavior.h"
#include "config.h"
#include "hotkey.h"
#include <iostream>
#include <cstring>

// ===========================
// Accessibility Manager
// ===========================
class AccessibilityManager {
public:
    static AccessibilityManager& Instance() {
        static AccessibilityManager inst;
        return inst;
    }

    bool IsProcessTrusted() {
        return AXIsProcessTrusted();
    }

    bool RequestAccessibilityPermission() {
        NSDictionary* options = @{
            (__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES
        };
        return AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    }

    bool CanEnableBehavior(const char* behaviorId) {
        if (!RequiresAccessibility(behaviorId)) {
            return true;
        }

        if (IsProcessTrusted()) {
            return true;
        }

        return RequestAccessibilityPermission();
    }

    bool RequiresAccessibility(const char* behaviorId) {
        static const char* behaviorsRequiringAccess[] = {
            "onePunch",
            "drag"
        };

        for (size_t i = 0; i < sizeof(behaviorsRequiringAccess) / sizeof(behaviorsRequiringAccess[0]); ++i) {
            if (std::strcmp(behaviorId, behaviorsRequiringAccess[i]) == 0) {
                return true;
            }
        }
        return false;
    }

    void ShowPermissionDeniedAlert(const char* behaviorId) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert* alert = [[NSAlert alloc] init];
            NSString* message = [NSString stringWithFormat:@"%s Requires Accessibility Permission",
                                  behaviorId];
            alert.messageText = message;
            alert.informativeText = @"Please enable Accessibility permissions in System Settings > Privacy & Security > Accessibility to use this behavior.";
            [alert addButtonWithTitle:@"Open System Settings"];
            [alert addButtonWithTitle:@"Cancel"];

            NSInteger result = [alert runModal];
            if (result == NSAlertFirstButtonReturn) {
                NSURL* url = [NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"];
                [[NSWorkspace sharedWorkspace] openURL:url];
            }
        });
    }

    std::string GetPermissionStatus() {
        if (IsProcessTrusted()) {
            return "trusted";
        }
        return "untrusted";
    }

private:
    AccessibilityManager() = default;
};

extern "C" bool CheckAccessibilityForBehavior(const char* behaviorId) {
    if (!AccessibilityManager::Instance().RequiresAccessibility(behaviorId)) {
        return true;
    }

    if (!AccessibilityManager::Instance().IsProcessTrusted()) {
        AccessibilityManager::Instance().ShowPermissionDeniedAlert(behaviorId);
        return false;
    }

    return true;
}

extern "C" const char* GetAccessibilityStatus() {
    static std::string status = AccessibilityManager::Instance().GetPermissionStatus();
    return status.c_str();
}

// ===========================
// Failsafe Hotkey Monitor
// ===========================
@interface FailsafeHotkeyMonitor : NSObject
+ (instancetype)shared;
- (void)startMonitoring;
- (void)stopMonitoring;
@property (nonatomic, assign) std::atomic<bool>* terminateFlag;
@end

@implementation FailsafeHotkeyMonitor

+ (instancetype)shared {
    static FailsafeHotkeyMonitor* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[FailsafeHotkeyMonitor alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _terminateFlag = nullptr;
    }
    return self;
}

- (void)startMonitoring {
    __weak FailsafeHotkeyMonitor* weakSelf = self;
    
    auto parsed = ParseHotkeyString(g_config.general.failsafeHotkey);
    int targetKeyCode = parsed ? parsed->keyCode : 0x29;
    CGEventFlags targetModifiers = 0;
    if (parsed) {
        if (parsed->modifierMask & kCGEventFlagMaskCommand) targetModifiers |= NSEventModifierFlagCommand;
        if (parsed->modifierMask & kCGEventFlagMaskShift) targetModifiers |= NSEventModifierFlagShift;
        if (parsed->modifierMask & kCGEventFlagMaskAlternate) targetModifiers |= NSEventModifierFlagOption;
        if (parsed->modifierMask & kCGEventFlagMaskControl) targetModifiers |= NSEventModifierFlagControl;
        if (parsed->modifierMask & kCGEventFlagMaskSecondaryFn) targetModifiers |= NSEventModifierFlagFunction;
    }

    static std::atomic<bool> localTerminateFlag(false);
    self.terminateFlag = &localTerminateFlag;

    [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^(NSEvent* event) {
        __strong FailsafeHotkeyMonitor* strongSelf = weakSelf;
        if (!strongSelf) return;
        if (strongSelf->_terminateFlag && *strongSelf->_terminateFlag) {
            return;
        }

        NSEventModifierFlags flags = event.modifierFlags;
        BOOL modsMatch = (flags & targetModifiers) == targetModifiers;

        if (modsMatch && event.keyCode == (unsigned short)targetKeyCode) {
            fprintf(stderr, "[FAILSAFE] Emergency hotkey triggered! Terminating...\n");
            fflush(stderr);
            if (strongSelf->_terminateFlag) {
                *strongSelf->_terminateFlag = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            exit(1);
        }
    }];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent*(NSEvent* event) {
        __strong FailsafeHotkeyMonitor* strongSelf = weakSelf;
        if (!strongSelf) return event;
        if (strongSelf->_terminateFlag && *strongSelf->_terminateFlag) {
            return event;
        }

        NSEventModifierFlags flags = event.modifierFlags;
        BOOL modsMatch = (flags & targetModifiers) == targetModifiers;

        if (modsMatch && event.keyCode == (unsigned short)targetKeyCode) {
            fprintf(stderr, "[FAILSAFE] Emergency hotkey triggered! Terminating...\n");
            fflush(stderr);
            if (strongSelf->_terminateFlag) {
                *strongSelf->_terminateFlag = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            exit(1);
        }
        return event;
    }];

    fprintf(stderr, "[FAILSAFE] Monitoring for %s to terminate\n", g_config.general.failsafeHotkey.c_str());
    fflush(stderr);
}

- (void)stopMonitoring {
    if (self.terminateFlag) {
        *self.terminateFlag = true;
    }
}

@end

extern "C" void StartFailsafeHotkey() {
    [[FailsafeHotkeyMonitor shared] startMonitoring];
}

extern "C" void StopFailsafeHotkey() {
    [[FailsafeHotkeyMonitor shared] stopMonitoring];
}

// ===========================
// Cursor Hijack Protection
// ===========================
@interface CursorHijackProtection : NSObject
+ (instancetype)shared;
- (void)startProtection;
- (void)stopProtection;
- (void)setHijacking:(BOOL)hijacking;
- (BOOL)isHijacking;
@property (nonatomic, assign) std::chrono::steady_clock::time_point lastCursorMove;
@property (nonatomic, assign) std::chrono::steady_clock::time_point hijackStartTime;
@property (nonatomic, assign) BOOL isHijacking;
@end

@implementation CursorHijackProtection {
    std::chrono::steady_clock::time_point _lastCursorMove;
    std::chrono::steady_clock::time_point _hijackStartTime;
    std::atomic<bool> _isHijackingAtomic;
    BOOL _isHijacking;
}

+ (instancetype)shared {
    static CursorHijackProtection* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[CursorHijackProtection alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _lastCursorMove = std::chrono::steady_clock::now();
        _hijackStartTime = std::chrono::steady_clock::now();
        _isHijackingAtomic.store(false);
        _isHijacking = NO;
    }
    return self;
}

- (void)setHijacking:(BOOL)hijacking {
    if (hijacking && !_isHijacking) {
        _hijackStartTime = std::chrono::steady_clock::now();
    }
    _isHijacking = hijacking;
    _isHijackingAtomic.store(hijacking);
}

- (BOOL)isHijacking {
    return _isHijacking;
}

- (void)startProtection {
    __weak CursorHijackProtection* weakSelf = self;

    [NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskMouseMoved
                                           handler:^(NSEvent* event) {
        __strong CursorHijackProtection* strongSelf = weakSelf;
        if (!strongSelf) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - strongSelf->_lastCursorMove).count();

        if (elapsed > 100 && strongSelf->_isHijacking) {
            auto elapsed_since_hijack = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - strongSelf->_hijackStartTime).count();

            if (elapsed_since_hijack > 2000) {
                fprintf(stderr, "[FAILSAFE] Cursor hijack timeout! User moved mouse after 2s.\n");
                fflush(stderr);
                strongSelf->_isHijacking = NO;
                strongSelf->_isHijackingAtomic.store(false);
            }
        }

        strongSelf->_lastCursorMove = now;
        if (strongSelf->_isHijacking && elapsed > 50) {
            strongSelf->_isHijacking = NO;
            strongSelf->_isHijackingAtomic.store(false);
        }
    }];
}

- (void)stopProtection {
}

@end

extern "C" void StartCursorHijackProtection() {
    [[CursorHijackProtection shared] startProtection];
}

extern "C" void SetCursorHijacking(BOOL hijacking) {
    [[CursorHijackProtection shared] setHijacking:hijacking];
}

extern "C" BOOL IsCursorHijacking() {
    return [[CursorHijackProtection shared] isHijacking];
}

// ===========================
// Permission Check Helper
// ===========================
extern "C" bool IsAccessibilityEnabled() {
    return AccessibilityManager::Instance().IsProcessTrusted();
}

extern "C" void PromptAccessibilityPermission() {
    AccessibilityManager::Instance().RequestAccessibilityPermission();
}