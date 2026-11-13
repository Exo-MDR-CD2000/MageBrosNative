// The small, dependency-free parts of the benchmark report: percentiles and JSON strings.
// Kept apart from the run so they can be tested without SDL, a window or a game.
#pragma once
#include <string>
#include <vector>

namespace Benchmark {

// The p-th percentile (0..1) of the values, nearest rank on a sorted copy.
double PercentileMs(const std::vector<double>& values, double p);
// The frame rate at a frame time in milliseconds.
double FpsFromMs(double ms);
// A JSON string body with quotes, backslashes and control characters escaped.
std::string JsonEscape(const std::string& s);

}  // namespace Benchmark
