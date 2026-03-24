#include "../test_framework.h"
#include "../../include/config.h"

TEST(ConfigPath) {
    std::string path = Config_GetPath();
    ASSERT_TRUE(path.length() > 0);
}

TEST(ConfigInit) {
    Config_InitRegistry();
    ASSERT_TRUE(true);
}