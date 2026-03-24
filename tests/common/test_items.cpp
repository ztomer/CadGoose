#include "../test_framework.h"
#include "../../include/items.h"

TEST(ItemData_DefaultConstructor) {
    ItemData data;
    ASSERT_EQ(data.type, ItemData::MEME);
    ASSERT_EQ(data.w, 0);
    ASSERT_EQ(data.h, 0);
}

TEST(ItemData_Text) {
    ItemData data;
    data.type = ItemData::TEXT;
    data.textContent = std::make_shared<std::string>("Hello World");
    ASSERT_EQ(data.Text(), "Hello World");
}

TEST(ItemData_Text_Empty) {
    ItemData data;
    ASSERT_EQ(data.Text(), "");
}

TEST(ItemData_ImageDimensions) {
    ItemData data;
    data.type = ItemData::MEME;
    data.w = 100;
    data.h = 50;
    ASSERT_EQ(data.w, 100);
    ASSERT_EQ(data.h, 50);
}