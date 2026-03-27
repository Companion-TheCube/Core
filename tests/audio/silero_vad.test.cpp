#include <gtest/gtest.h>

#include "audio/sileroVad.h"
#include "utils.h"

#include <vector>

namespace {

void resetSileroVadConfig()
{
    Config::erase("SILERO_VAD_ENABLED");
    Config::erase("SILERO_VAD_MODEL_PATH");
    Config::erase("SILERO_VAD_THRESHOLD");
    Config::erase("SILERO_VAD_RELEASE_DELTA");
    Config::erase("SILERO_VAD_WINDOW_MS");
    Config::erase("SILERO_VAD_MIN_SILENCE_MS");
    Config::erase("SILERO_VAD_INTRA_THREADS");
    Config::erase("SILERO_VAD_INTER_THREADS");
}

} // namespace

TEST(SileroVad, DisableByConfigFallsBackCleanly)
{
    resetSileroVadConfig();
    Config::set("SILERO_VAD_ENABLED", "0");

    audio::SileroVad vad;
    EXPECT_FALSE(vad.available());
    EXPECT_EQ(vad.failureReason(), "disabled_by_config");

    const auto result = vad.analyzePcm16(std::vector<int16_t>(1280, 2000));
    EXPECT_FALSE(result.speechObserved);
    EXPECT_FALSE(result.segmentActive);

    resetSileroVadConfig();
}

TEST(SileroVad, MissingModelFallsBackCleanly)
{
    resetSileroVadConfig();
    Config::set("SILERO_VAD_ENABLED", "1");
    Config::set("SILERO_VAD_MODEL_PATH", "does/not/exist/silero_vad.onnx");

    audio::SileroVad vad;
    EXPECT_FALSE(vad.available());
    EXPECT_EQ(vad.failureReason(), "model_not_found");

    resetSileroVadConfig();
}
