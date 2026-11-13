// The benchmark report's percentile and JSON helpers, without SDL or a game.
#include <cstdio>
#include <string>
#include <vector>
#include "framework/benchmark_stats.h"

static int g_fail = 0;
#define CHECK(c, ...)                         \
    do {                                      \
        if (!(c)) {                           \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                \
            g_fail++;                         \
        }                                     \
    } while (0)

int main() {
    using Benchmark::FpsFromMs;
    using Benchmark::JsonEscape;
    using Benchmark::PercentileMs;
    std::vector<double> v{40, 10, 30, 20, 50};
    CHECK(PercentileMs(v, 0.0) == 10, "p0");
    CHECK(PercentileMs(v, 0.5) == 30, "p50");
    CHECK(PercentileMs(v, 0.95) == 50, "p95");
    CHECK(PercentileMs(v, 1.0) == 50, "p100");
    CHECK(PercentileMs({}, 0.99) == 0, "empty");
    CHECK(FpsFromMs(16.6667) > 59.99 && FpsFromMs(16.6667) < 60.01, "60 fps at 16.67 ms");
    CHECK(FpsFromMs(0) == 0, "zero frame time");

    CHECK(JsonEscape("plain") == "plain", "plain text");
    CHECK(JsonEscape("a\"b") == "a\\\"b", "quote");
    CHECK(JsonEscape("a\\b") == "a\\\\b", "backslash");
    CHECK(JsonEscape("a\nb") == "a\\nb", "newline");
    std::string ctrl("\x01", 1);
    CHECK(JsonEscape(ctrl) == "\\u0001", "control character");

    std::printf("benchmark stats: %s\n", g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
