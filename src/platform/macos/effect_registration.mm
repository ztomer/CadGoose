// effect_registration.mm
// Registry storage + API for effect window types.
// Each effect type registers itself from its own source file.

#import "effect_registration.h"
#include <vector>

static std::vector<EffectRegistration> s_registrations;

void EffectRegister(const EffectRegistration& reg) {
    s_registrations.push_back(reg);
}

const std::vector<EffectRegistration>& EffectGetRegistrations() {
    return s_registrations;
}
