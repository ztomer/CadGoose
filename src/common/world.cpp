// ===========================
// world.cpp
// ===========================
#include "world.h"
#include "actor.h"

WorldContext g_world;
static const size_t UI_LOG_MAX = 12;

void UiLogPush(const std::string& s) {
    g_world.uiLog.push_back(s);
    if (g_world.uiLog.size() > UI_LOG_MAX) g_world.uiLog.pop_front();
}

#ifdef __linux__
#include <gtk/gtk.h>
#endif

Goose* GetGooseById(int id) {
    for (auto* g : ActorManager::Instance().getGeese()) {
        if (g->id == id) return g;
    }
    return nullptr;
}