// Link stubs for the config-only test binaries (CadGooseTsanTests on macOS,
// CadGooseLinuxTests on Linux): the config sources they carry reach into the
// audio module, the theme probe and the actor registry, none of which a
// config test touches. Each stub is the inert answer.
#include "actor.h"
#include "actor_manager.h"
#include "config.h"

void Audio_SetEnabled(bool) {}
void Audio_SetMuted(bool) {}
bool Config_IsSystemDarkTheme() { return false; }

ActorManager& ActorManager::Instance() {
    static ActorManager manager;
    return manager;
}

const std::vector<Goose*>& ActorManager::getGeese() const {
    static const std::vector<Goose*> none;
    return none;
}
