#include "app_actions.h"
#include "config.h"
#include "world.h"
#include "behavior.h"
#include "actor.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(__linux__)
#include "glib.h"
#include "ui.h"
#endif

#if defined(__APPLE__)
void AppActions_SetApplication(void* app) {}
#endif

Goose* AppActions_SpawnGoose(const std::string& requestedName) {
    std::string name = requestedName;
    if (name.empty()) {
        size_t idx = ActorManager::Instance().getGeese().size();
        if (idx < g_config.gooseNames.size() && !g_config.gooseNames[idx].empty()) {
            name = g_config.gooseNames[idx];
        } else {
            name = "Goose " + std::to_string(g_world.nextId);
        }
    }

    Goose* goose = new Goose(g_world.nextId++, name, g_world.screenWidth, g_world.screenHeight);
    ActorManager::Instance().add(goose);
    BehaviorRegistry::Instance().InitAll(goose);

#if defined(__linux__)
    UiLogPush("Spawned " + name);
#endif
    return goose;
}

void AppActions_EnsureInitialGoose() {
    if (!ActorManager::Instance().getGeese().empty()) return;
    AppActions_SpawnGoose("");
}

void AppActions_ClearGeese() {
    for (auto& item : g_world.droppedItems) {
        delete item.data;
    }
    g_world.droppedItems.clear();
    g_world.footprints.clear();

    Config_SaveGooseNames();

    ActorManager::Instance().destroyAllOfType("goose");

    g_world.cursorGrabberId = -1;
    g_world.selectedGooseId = 0;
    g_world.nextId = 0;

#if defined(__linux__)
    for (GtkWidget* canvas : g_world.overlayCanvases) {
        if (canvas) gtk_widget_queue_draw(canvas);
    }
    UiLogPush("Cleared all geese.");
#endif
}

#if defined(__linux__)
static gboolean QuitAfterClearFrame(gpointer data) {
    GtkApplication* app = static_cast<GtkApplication*>(data);
    if (app) g_application_quit(G_APPLICATION(app));
    return G_SOURCE_REMOVE;
}

void AppActions_Quit() {
    // Linux: quit via GTK
}
#endif

#if defined(__APPLE__)
void AppActions_Quit() {
    // macOS: handled differently
}
#endif

static std::string GetRamUsageReport() {
#if defined(__linux__)
    std::ifstream file("/proc/self/status");
    std::string line;
    long rssKb = -1;
    long hwmKb = -1;
    long vmKb = -1;

    while (std::getline(file, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream(line.substr(6)) >> rssKb;
        } else if (line.rfind("VmHWM:", 0) == 0) {
            std::istringstream(line.substr(6)) >> hwmKb;
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream(line.substr(7)) >> vmKb;
        }
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(2);
    if (rssKb >= 0) out << "ram_rss_mb=" << (rssKb / 1024.0) << "\n";
    if (hwmKb >= 0) out << "ram_peak_mb=" << (hwmKb / 1024.0) << "\n";
    if (vmKb >= 0) out << "ram_virtual_mb=" << (vmKb / 1024.0) << "\n";
    return out.str();
#elif defined(__APPLE__)
    return "";
#endif
}

std::string AppActions_GetStatus() {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    out << "running=1\n";
    auto geese = ActorManager::Instance().getGeese();
    out << "goose_count=" << geese.size() << "\n";
    out << "config_path=" << Config_GetPath() << "\n";
    out << GetRamUsageReport();

    if (!geese.empty()) {
        const auto& g = *geese.front();
        out << "goose_pos=" << g.pos.x << "," << g.pos.y << "\n";
        out << "goose_state=";
        switch (g.state) {
            case GooseState::WANDER: out << "wander"; break;
            case GooseState::FETCHING: out << "fetching"; break;
            case GooseState::RETURNING: out << "returning"; break;
            case GooseState::CHASE_CURSOR: out << "chase_cursor"; break;
            case GooseState::SNATCH_CURSOR: out << "snatch_cursor"; break;
        }
        out << "\n";
        out << "goose_heldItem=" << (g.heldItem ? "yes" : "no") << "\n";
    }

    out << "dropped_items=" << g_world.droppedItems.size() << "\n";
    for (const auto& item : g_world.droppedItems) {
        if (!item.data) {
            out << "item_null\n";
            continue;
        }
        out << "item_pos=" << item.pos.x << "," << item.pos.y
            << " size=" << item.data->w << "x" << item.data->h
            << " type=" << (int)item.data->type
            << " rotation=" << item.rotation
            << " pinned=" << (item.pinned ? "1" : "0") << "\n";
    }

    for (const auto& opt : g_configRegistry) {
        std::string value;
        Config_GetValueByKey(opt.key, &value, nullptr);
        out << opt.key << "=" << value << "\n";
    }

    return out.str();
}

std::string AppActions_HandleCommand(const std::vector<std::string>& args) {
    if (args.empty()) return "error missing command\n";

    const std::string& command = args.front();
    if (command == "spawn") {
        Goose* goose = AppActions_SpawnGoose(args.size() > 1 ? args[1] : "");
        return "ok id=" + std::to_string(goose ? goose->id : -1) + "\n";
    }

    if (command == "clear") {
        AppActions_ClearGeese();
        return "ok\n";
    }

    if (command == "status") {
        return AppActions_GetStatus();
    }

    if (command == "ram") {
        return GetRamUsageReport();
    }

    if (command == "quit") {
        AppActions_ClearGeese();
        AppActions_Quit();
        return "ok cleared and quitting\n";
    }

    if (command == "fetch") {
        auto geese = ActorManager::Instance().getGeese();
        if (geese.empty()) return "error no goose\n";
        int type = 0;
        if (args.size() > 1) {
            if (args[1] == "text") type = 1;
            else if (args[1] == "meme") type = 0;
        }
        fprintf(stderr, "[CLI] fetch type=%d geese.size=%zu\n", type, geese.size());
        geese.front()->ForceFetch(type, g_world.screenWidth, g_world.screenHeight);
        fprintf(stderr, "[CLI] after ForceFetch state=%d heldItem=%p\n", (int)geese.front()->state, (void*)geese.front()->heldItem);
        return "ok force_fetch type=" + std::to_string(type) + "\n";
    }

    if (command == "enable") {
        if (args.size() < 2) return "error missing behavior id\n";
        auto* behavior = BehaviorRegistry::Instance().Get(args[1].c_str());
        if (!behavior) return "error unknown behavior: " + args[1] + "\n";
        if (behavior->enabledPtr) {
            *behavior->enabledPtr = true;
            if (behavior->configPtr) *behavior->configPtr = true;
            return "ok enabled " + args[1] + "\n";
        }
        return "error behavior has no enabledPtr\n";
    }

    if (command == "disable") {
        if (args.size() < 2) return "error missing behavior id\n";
        auto* behavior = BehaviorRegistry::Instance().Get(args[1].c_str());
        if (!behavior) return "error unknown behavior: " + args[1] + "\n";
        if (behavior->enabledPtr) {
            *behavior->enabledPtr = false;
            if (behavior->configPtr) *behavior->configPtr = false;
            return "ok disabled " + args[1] + "\n";
        }
        return "error behavior has no enabledPtr\n";
    }

    if (command == "drag_test") {
        if (args.size() < 3) return "error usage: drag_test <x> <y>\n";
        float targetX = std::stof(args[1]);
        float targetY = std::stof(args[2]);
#if defined(__APPLE__)
        extern void ItemWindow_DragTest(float, float);
        ItemWindow_DragTest(targetX, targetY);
        return "ok drag_test dispatched\n";
#else
        return "error drag_test only available on macOS\n";
#endif
    }

    return "error unknown command: " + command + "\n";
}