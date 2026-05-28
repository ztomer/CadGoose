#pragma once

#include "goose.h"

class BabyStalinActor : public Goose {
public:
    BabyStalinActor(int id_, const std::string& name_, int screenW, int screenH);

    const char* type() const override { return "baby_stalin"; }

    void onHonk() override;

#ifdef __APPLE__
    void drawBody(CGContextRef ctx) override;
#endif
};
