#include <gtest/gtest.h>
#include <Cocoa/Cocoa.h>
#include "renderer.h"

TEST(Rendering, YAxisFlip) {
    // Coordinate Y-axis flip math test
    float boundsHeight = 1080.0f;
    float originalY = 200.0f;
    float flippedY = boundsHeight - originalY;
    EXPECT_FLOAT_EQ(flippedY, 880.0f);
}

TEST(Rendering, GooseViewInitialization) {
    // Test that GooseView can be allocated and initialized properly
    NSRect frame = NSMakeRect(0, 0, 1920, 1080);
    GooseView* view = [[GooseView alloc] initWithFrame:frame];
    
    EXPECT_NE(view, nil);
    EXPECT_TRUE(view.wantsLayer);
    
    // Simulate resolution scale test
    EXPECT_EQ(view.bounds.size.width, 1920);
    EXPECT_EQ(view.bounds.size.height, 1080);
}

TEST(Rendering, ResolutionAndDPISimulation) {
    // Testing different simulated screen bounds
    NSRect lowRes = NSMakeRect(0, 0, 800, 600);
    GooseView* viewLow = [[GooseView alloc] initWithFrame:lowRes];
    EXPECT_EQ(viewLow.bounds.size.width, 800);
    
    NSRect highRes = NSMakeRect(0, 0, 3840, 2160);
    GooseView* viewHigh = [[GooseView alloc] initWithFrame:highRes];
    EXPECT_EQ(viewHigh.bounds.size.width, 3840);
}
