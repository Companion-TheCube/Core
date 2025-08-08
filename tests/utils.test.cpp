// Purpose: Verify that genericSleep blocks the current thread for at least
// the requested number of milliseconds. We allow a small upper bound tolerance
// because timers and scheduler wake-ups are not precise on all platforms.
//
// Notes:
// - On Linux genericSleep uses usleep(ms * 1000).
// - On Windows genericSleep uses Sleep(ms).
// - We measure wall time using a high-resolution clock and assert bounds.
#include <gtest/gtest.h>
#include "../src/utils.h"

TEST(utils, GenericSleep_Time){
    // Capture a start timestamp using a high-resolution, steady clock.
    auto start = std::chrono::high_resolution_clock::now();

    // Perform the operation under test: sleep for 1 second.
    genericSleep(1000);

    // Capture an end timestamp and compute the elapsed duration in ms.
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Lower bound: we must have waited at least 1000 ms.
    EXPECT_GE(duration.count(), 1000);

    // Upper bound: allow a little scheduling/precision slop but keep it tight so
    // the test still catches gross regressions (e.g., accidental microseconds input).
    EXPECT_LT(duration.count(), 1100);
}
