// Purpose: Provide a custom test runner entry point. While gtest_main could
// supply this automatically, having an explicit main() lets us customize
// initialization if needed (flags, test filtering, sharding, etc.).
#include <gtest/gtest.h>

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Future customization hook: handle custom flags here before running tests.
    return RUN_ALL_TESTS();
}
