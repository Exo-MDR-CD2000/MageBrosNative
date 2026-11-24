#include "benchmark_stats.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Benchmark {

double PercentileMs(const std::vector<double>& values, double p) {
    if (values.empty()) return 0;
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    p = std::clamp(p, 0.0, 1.0);
    size_t i = (size_t)std::llround(p * (double)(sorted.size() - 1));
    if (i >= sorted.size()) i = sorted.size() - 1;
    return sorted[i];
}

double FpsFromMs(double ms) { return ms > 0 ? 1000.0 / ms : 0; }

std::string JsonEscape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) {
                    char b[8];
                    std::snprintf(b, sizeof b, "\\u%04x", (unsigned)(unsigned char)c);
                    o += b;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

}  // namespace Benchmark
