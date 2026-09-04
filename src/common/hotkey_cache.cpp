#include "hotkey_cache.h"
#include "hotkey.h"
#include "config.h"

#include <array>
#include <atomic>
#include <mutex>

// One mutex for the canonical g_config hotkey strings; the resolved key codes
// live in lock-free atomics read by the per-frame behavior ticks.
static std::mutex g_hotkeyMutex;
static std::array<std::atomic<int>, static_cast<size_t>(HotkeyId::Count)> g_keyCodes = {};

// Map a hotkey id to its canonical std::string field in g_config.
static std::string* FieldFor(HotkeyId id) {
    switch (id) {
        case HotkeyId::HonckerHonk: return &g_config.behaviors.honcker.hotkey;
        case HotkeyId::JailSet:     return &g_config.behaviors.jail.hotkeyO;
        case HotkeyId::JailToggle:  return &g_config.behaviors.jail.hotkeyP;
        case HotkeyId::Portal1:     return &g_config.portal.hotkey1;
        case HotkeyId::Portal2:     return &g_config.portal.hotkey2;
        case HotkeyId::Portal0:     return &g_config.portal.hotkey0;
        case HotkeyId::Breadcrumbs: return &g_config.behaviors.breadCrumbs.hotkey;
        case HotkeyId::Count:       return nullptr;
    }
    return nullptr;
}

int Hotkey_KeyCode(HotkeyId id) {
    if (static_cast<size_t>(id) >= static_cast<size_t>(HotkeyId::Count)) return -1;
    return g_keyCodes[static_cast<size_t>(id)].load(std::memory_order_relaxed);
}

std::string Hotkey_Name(HotkeyId id) {
    std::lock_guard<std::mutex> lock(g_hotkeyMutex);
    std::string* f = FieldFor(id);
    return f ? *f : std::string();
}

void Hotkey_SetName(HotkeyId id, const std::string& name) {
    if (static_cast<size_t>(id) >= static_cast<size_t>(HotkeyId::Count)) return;
    int code = KeyNameToKeyCode(name);  // parse outside the lock
    std::lock_guard<std::mutex> lock(g_hotkeyMutex);
    if (std::string* f = FieldFor(id)) *f = name;
    g_keyCodes[static_cast<size_t>(id)].store(code, std::memory_order_relaxed);
}

void Hotkey_SyncFromConfig() {
    std::lock_guard<std::mutex> lock(g_hotkeyMutex);
    for (size_t i = 0; i < static_cast<size_t>(HotkeyId::Count); i++) {
        std::string* f = FieldFor(static_cast<HotkeyId>(i));
        g_keyCodes[i].store(f ? KeyNameToKeyCode(*f) : -1, std::memory_order_relaxed);
    }
}
