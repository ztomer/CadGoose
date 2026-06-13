#include <gtest/gtest.h>
#include "items.h"

TEST(ItemDataTest, DefaultConstructor) {
    ItemData data;
    EXPECT_EQ(data.type, ItemData::MEME);
    EXPECT_EQ(data.w, 0);
    EXPECT_EQ(data.h, 0);
    EXPECT_FALSE(data.isAIGenerated);
}

TEST(ItemDataTest, TextContent) {
    ItemData data;
    data.type = ItemData::TEXT;
    data.textContent = std::make_shared<std::string>("Hello World");
    EXPECT_EQ(data.Text(), "Hello World");
}

TEST(ItemDataTest, TextEmpty) {
    ItemData data;
    EXPECT_EQ(data.Text(), "");
}

TEST(ItemDataTest, ImageDimensions) {
    ItemData data;
    data.type = ItemData::MEME;
    data.w = 100;
    data.h = 50;
    EXPECT_EQ(data.w, 100);
    EXPECT_EQ(data.h, 50);
}

TEST(ItemDataTest, TextSetThenClear) {
    ItemData data;
    data.type = ItemData::TEXT;
    data.textContent = std::make_shared<std::string>("temporary");
    EXPECT_EQ(data.Text(), "temporary");
    data.textContent.reset();
    EXPECT_EQ(data.Text(), "");
}

TEST(DroppedItemTest, NotExpiredByDefault) {
    ItemData data;
    DroppedItem item;
    item.data = &data;
    item.timeDropped = 0.0;
    item.pinned = false;
    // With itemLifetime > 0, fresh items should not be expired
    EXPECT_FALSE(item.isExpired(1.0));
}

TEST(DroppedItemTest, ExpiredAfterLifetime) {
    ItemData data;
    DroppedItem item;
    item.data = &data;
    item.timeDropped = 0.0;
    item.pinned = false;
    // With default itemLifetime, item dropped at t=0 should be expired well past lifetime
    EXPECT_TRUE(item.isExpired(1000000.0));
}

TEST(DroppedItemTest, PinnedNeverExpires) {
    ItemData data;
    DroppedItem item;
    item.data = &data;
    item.timeDropped = 0.0;
    item.pinned = true;
    EXPECT_FALSE(item.isExpired(1000000.0));
}
