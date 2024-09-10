#include "../src/logger/logger.cpp"
#include <benchmark/benchmark.h>

static void BM_LogCreation(benchmark::State& state) {
    for (auto _ : state) {
        CubeLog log = CubeLog(LogVerbosity::MINIMUM, LogLevel::LOGGER_OFF, LogLevel::LOGGER_OFF);
        log.setConsoleLoggingEnabled(false);
    }
}
static void BM_LogInfo(benchmark::State& state) {
    CubeLog log = CubeLog(LogVerbosity::MINIMUM, LogLevel::LOGGER_OFF, LogLevel::LOGGER_OFF);
    log.setConsoleLoggingEnabled(false);
    for (auto _ : state) {
        log.info("Test");
    }
}

BENCHMARK(BM_LogCreation);
BENCHMARK(BM_LogInfo);