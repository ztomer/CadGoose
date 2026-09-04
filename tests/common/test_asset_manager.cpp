// AssetManager coverage — creation paths and cache behavior that the
// behavior tests only touch incidentally. All loads resolve through the
// real ASSET_ROOT set by AssetManager::Init in the global test environment.
#include <gtest/gtest.h>
#include "assets.h"
#include "config.h"

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#endif

namespace {

TEST(AssetManagerTest, CreateTextItemCarriesContentAndAIGenerationFlag) {
    ItemData* data = g_assets.CreateTextItem("hello world");
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->type, ItemData::TEXT);
    ASSERT_NE(data->textContent, nullptr);
    EXPECT_EQ(*data->textContent, "hello world");
    EXPECT_TRUE(data->isAIGenerated);
    delete data;
}

TEST(AssetManagerTest, GetRandomTextReadsRealFile) {
    // The environment Init scanned Assets/Text/NotepadMessages — non-empty
    // on any real checkout.
    if (g_assets.textPaths.empty()) {
        GTEST_SKIP() << "no text assets in this checkout";
    }
    ItemData* data = g_assets.GetRandomText();
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->type, ItemData::TEXT);
    ASSERT_NE(data->textContent, nullptr);
    EXPECT_FALSE(data->textContent->empty());
    delete data;
}

TEST(AssetManagerTest, CreateToyItemStickAndBallShapes) {
    ItemData* stick = g_assets.CreateToyItem(true);
    ASSERT_NE(stick, nullptr);
    EXPECT_EQ(stick->type, ItemData::TOY);
    EXPECT_EQ(stick->w, 32);
    EXPECT_EQ(stick->h, 8);
#ifdef __APPLE__
    EXPECT_NE(stick->image, nil);
#endif
    delete stick;

    ItemData* ball = g_assets.CreateToyItem(false);
    ASSERT_NE(ball, nullptr);
    EXPECT_EQ(ball->w, 20);
    EXPECT_EQ(ball->h, 20);
#ifdef __APPLE__
    EXPECT_NE(ball->image, nil);
#endif
    delete ball;
}

TEST(AssetManagerTest, CreateTestImageIsExactSize) {
    ItemData* data = g_assets.CreateTestImage(48, 24);
    ASSERT_NE(data, nullptr);
    EXPECT_EQ(data->type, ItemData::MEME);
    EXPECT_EQ(data->w, 48);
    EXPECT_EQ(data->h, 24);
#ifdef __APPLE__
    ASSERT_NE(data->image, nil);
    EXPECT_EQ(CGImageGetWidth(data->image), 48u);
    EXPECT_EQ(CGImageGetHeight(data->image), 24u);
#endif
    delete data;
}

TEST(AssetManagerTest, GetBehaviorImageCachesAndReturnsNullOnMiss) {
#ifdef __APPLE__
    const std::string kPath = "Assets/Images/OtherGfx/honk.png";

    CGImageRef first = g_assets.GetBehaviorImage(kPath);
    ASSERT_NE(first, nullptr);

    CGImageRef second = g_assets.GetBehaviorImage(kPath);
    EXPECT_EQ(first, second) << "second lookup must hit the cache, not reload";

    CGImageRef miss = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/definitely_not_a_real_file.png");
    EXPECT_EQ(miss, nullptr);
#endif
}

TEST(AssetManagerTest, GetRandomMemeRespectsMaxSizeFraction) {
    if (g_assets.memePaths.empty()) {
        GTEST_SKIP() << "no meme assets in this checkout";
    }
    ItemData* small = g_assets.GetRandomMeme(1920, 1080, 0.05f);
    ASSERT_NE(small, nullptr);
    EXPECT_EQ(small->type, ItemData::MEME);
    // Every meme is scaled down to fit within 5% of the given screen.
    EXPECT_LE(small->w, (int)(1920 * 0.05f));
    EXPECT_LE(small->h, (int)(1080 * 0.05f));
    delete small;
}

}  // namespace
