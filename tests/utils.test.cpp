#include <gtest/gtest.h>
#include "../src/utils.cpp"

TEST(utils, GenericSleep_Time){
    // get the current time
    auto start = std::chrono::high_resolution_clock::now();
    // sleep for 1 second
    genericSleep(1000);
    // get the current time
    auto end = std::chrono::high_resolution_clock::now();
    // calculate the time difference
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // check if the time difference is greater than 1 second
    EXPECT_GE(duration.count(), 1000);
    EXPECT_LT(duration.count(), 1100);
}