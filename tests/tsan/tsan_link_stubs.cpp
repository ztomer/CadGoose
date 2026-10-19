// Link stubs for CadGooseTsanTests: the config sources it instruments reach
// into the audio module, the theme probe and the actor registry, none of which
// the race under test touches. Each stub is the inert answer.
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
