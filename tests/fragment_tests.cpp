// The fragment/effect culling test: the draw-only visibility rule, without SDL or a game file.
#include <cstdio>
#include "framework/benchmark_spec.h"
#include "game/XGameContext.h"

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
    int w = 0, l = 0;
    float fx = 0, fz = 0;
    CHECK(BenchmarkSpec::ParseFragment("1,5,140,3600", &w, &l, &fx, &fz), "fragment parses");
    CHECK(w == 1 && l == 5 && fx == 140 && fz == 3600, "fragment values");
    CHECK(!BenchmarkSpec::ParseFragment("1,5,140", &w, &l, &fx, &fz), "short fragment rejected");
    CHECK(BenchmarkSpec::ParseSweep("2,8", &w, &l) && w == 2 && l == 8, "sweep parses");
    CHECK(!BenchmarkSpec::ParseSweep("2", &w, &l), "short sweep rejected");

    XGameContext g;
    g.cameraZ = 1000.0f;
    g.visibleRange = 400.0f;

    // The object's own test keeps its existing 400-unit meaning.
    CHECK(g.IsVisible(D3DVECTOR(140, 120, 1000)), "at the camera");
    CHECK(g.IsVisible(D3DVECTOR(140, 120, 1399)), "inside the draw range");
    CHECK(!g.IsVisible(D3DVECTOR(140, 120, 1401)), "outside the draw range");

    // A particle gets its own radius: a large flame at the edge is still drawn, and only a
    // centre beyond range + radius is culled.
    CHECK(g.IsVisible(D3DVECTOR(140, 200, 1405), 10), "radius keeps a nearby effect");
    CHECK(!g.IsVisible(D3DVECTOR(140, 200, 1411), 10), "beyond range + radius");
    CHECK(g.IsVisible(D3DVECTOR(20, 200, 550), 110), "a 110-unit flame at the far edge");
    CHECK(!g.IsVisible(D3DVECTOR(20, 200, 480), 110), "a flame beyond the edge");

    // A fragment far from the level start still exercises the same rule.
    g.cameraZ = 3600.0f;
    CHECK(g.IsVisible(D3DVECTOR(140, 120, 3600)), "fragment at the camera");
    CHECK(!g.IsVisible(D3DVECTOR(140, 120, 4001)), "fragment outside the range");

    std::printf("fragment culling: %s\n", g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
