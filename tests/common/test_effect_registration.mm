#include <gtest/gtest.h>
#include "effect_registration.h"
#include <vector>

// Provide implementations for the function pointers needed by EffectRegister
// so they can be valid C function pointers in the registration.
static std::vector<Vector2> dummyPositions() { return {Vector2{10, 20}}; }
static float dummyRadius(const Vector2&) { return 15.0f; }
static bool dummyExists(const Vector2&) { return true; }

TEST(EffectRegistration, RegisterAndRetrieve) {
    EffectRegistration reg;
    reg.type = 42;
    reg.getPositions = dummyPositions;
    reg.getRadius = dummyRadius;
    reg.existsAt = dummyExists;
    reg.configureWindow = nullptr;
    reg.configureContentView = nullptr;
    EffectRegister(reg);

    const auto& registrations = EffectGetRegistrations();
    ASSERT_FALSE(registrations.empty());

    const auto* found = [](int type) -> const EffectRegistration* {
        const auto& regs = EffectGetRegistrations();
        for (size_t i = 0; i < regs.size(); i++) {
            if (regs[i].type == type) return &regs[i];
        }
        return nullptr;
    }(42);

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->type, 42);
    EXPECT_EQ(found->getPositions().size(), 1u);
    EXPECT_EQ(found->getPositions()[0].x, 10);
    EXPECT_FLOAT_EQ(found->getRadius({}), 15.0f);
    EXPECT_TRUE(found->existsAt({}));
}

TEST(EffectRegistration, MultipleRegistrations) {
    EffectRegistration r1, r2;
    r1.type = 1; r1.getPositions = dummyPositions; r1.getRadius = dummyRadius; r1.existsAt = dummyExists;
    r2.type = 2; r2.getPositions = dummyPositions; r2.getRadius = dummyRadius; r2.existsAt = dummyExists;
    EffectRegister(r1);
    EffectRegister(r2);

    const auto& regs = EffectGetRegistrations();
    bool found1 = false, found2 = false;
    for (size_t i = 0; i < regs.size(); i++) {
        if (regs[i].type == 1) found1 = true;
        if (regs[i].type == 2) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}
