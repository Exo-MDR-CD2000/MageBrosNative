// The generated benchmark level: validity, mix and geometry, with no game files needed.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include "core/benchmark_level.h"
#include "game/XMapGeometry.h"

static int g_fail = 0;
#define CHECK(c, ...)                         \
    do {                                      \
        if (!(c)) {                           \
            std::printf("FAIL: " __VA_ARGS__); \
            std::printf("\n");                \
            g_fail++;                         \
        }                                     \
    } while (0)

// The same classification XLevelView uses, kept here so the test also proves that every object
// character in the generated level is one the game knows.
static bool IsKnownObject(char c) {
    if (c == '#' || c == '$' || c == '*' || c == '+' || c == '/') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return false;
}

static bool IsEnemy(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '5');
}

static bool QuadIsRectangle(const D3DLVERTEX* v) {
    auto dist = [](const D3DLVERTEX& a, const D3DLVERTEX& b) {
        double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    };
    auto near = [](double a, double b) { return std::fabs(a - b) < 0.01; };
    return near(dist(v[0], v[1]), dist(v[2], v[3])) && near(dist(v[0], v[2]), dist(v[1], v[3])) &&
           near(dist(v[0], v[3]), dist(v[1], v[2])) && dist(v[0], v[1]) > 1.0 && dist(v[0], v[2]) > 1.0;
}

static void CheckMesh(const char* which, const MapGeometry::Mesh& m) {
    CHECK(m.vertices.size() % 4 == 0, "%s: vertex count is not whole quads", which);
    CHECK(m.indices.size() == m.vertices.size() / 4 * 6, "%s: wrong index count", which);
    CHECK(m.vertices.size() <= 65535, "%s: too many vertices for 16-bit indices (%zu)", which, m.vertices.size());
    for (WORD i : m.indices) CHECK(i < m.vertices.size(), "%s: index %u out of range", which, i);
    for (size_t q = 0; q < m.vertices.size(); q += 4) {
        CHECK(QuadIsRectangle(&m.vertices[q]), "%s: quad %zu is not a rectangle", which, q / 4);
        for (int k = 0; k < 4; k++) {
            const auto& p = m.vertices[q + k];
            CHECK(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z), "%s: non-finite vertex", which);
            CHECK(p.x >= -0.01f && p.x <= 200.01f, "%s: vertex x %g outside the level", which, p.x);
            CHECK(p.y >= -200.01f && p.y <= 320.01f, "%s: vertex y %g outside the level", which, p.y);
        }
    }
}

int main() {
    const std::string& text = BenchmarkLevel::Text();
    CHECK(!text.empty(), "the benchmark text is empty");
    CHECK(BenchmarkLevel::Hash() != 0, "the benchmark hash is zero");
    CHECK(BenchmarkLevel::Hash() == BenchmarkLevel::Hash(), "the benchmark hash changes between calls");

    LevelGrid grid;
    std::string error;
    CHECK(BenchmarkLevel::Build(&grid, &error), "the benchmark grid does not parse: %s", error.c_str());
    CHECK(grid.depth == BenchmarkLevel::kDepth, "depth %d, expected %d", grid.depth, BenchmarkLevel::kDepth);
    CHECK(grid.rowFlags.size() == (size_t)grid.depth, "row flag count");
    CHECK(grid.cells.size() == (size_t)grid.depth * LevelGrid::kWidth, "cell count");
    CHECK(grid.objects.size() == (size_t)grid.depth * LevelGrid::kWidth, "object count");

    int enemies = 0, coins = 0, torches = 0, fences = 0, platforms = 0, statics = 0, goals = 0, candles = 0, water = 0;
    bool enemyKinds[128] = {};
    for (int z = 0; z < grid.depth; z++) {
        char flag = grid.Flag(z);
        CHECK(flag == '-' || flag == 'C' || flag == 'W', "row %d: unknown flag '%c'", z, flag);
        candles += flag == 'C';
        water += flag == 'W';
        for (int x = 0; x < LevelGrid::kWidth; x++) {
            char cell = grid.Cell(z, x);
            bool floor = cell >= '0' && cell <= '9';
            CHECK(floor || cell == 'H' || cell == '.', "row %d cell %d: unknown terrain '%c'", z, x, cell);
            char obj = grid.Object(z, x);
            if (obj == 0 || obj == '-') continue;
            CHECK(IsKnownObject(obj), "row %d cell %d: unknown object '%c'", z, x, obj);
            CHECK(floor || cell == 'H', "row %d cell %d: object '%c' is over a pit", z, x, obj);
            if (obj == '#') goals++;
            if (obj == '$') coins++;
            if (obj == '*' || obj == '+') torches++;
            if (obj == '/') fences++;
            if (obj >= 'A' && obj <= 'G') platforms++;
            if (obj >= 'H' && obj <= 'Z') statics++;
            if (IsEnemy(obj)) {
                enemies++;
                enemyKinds[(unsigned char)obj] = true;
            }
        }
    }
    int kinds = 0;
    for (bool k : enemyKinds) kinds += k;
    CHECK(goals == 1, "goal count %d, expected 1", goals);
    CHECK(enemies >= 30, "enemy count %d, expected a dense scene", enemies);
    CHECK(kinds >= 8, "enemy kinds %d, expected a varied scene", kinds);
    CHECK(coins >= 5, "coin count %d", coins);
    CHECK(torches >= 4, "torch count %d", torches);
    CHECK(fences >= 2, "fence count %d", fences);
    CHECK(platforms >= 4, "platform count %d", platforms);
    CHECK(statics >= 4, "static prop count %d", statics);
    CHECK(candles >= 1, "candle row count %d", candles);
    CHECK(water >= 1, "the level has no water row");
    CHECK(enemies < 110, "enemy count %d exceeds the game's slots", enemies);
    CHECK(candles <= 30, "candle row count %d exceeds the game's slots", candles);

    // The hero starts at x=100, z=60 (cell 2, row 1); the goal is at the last row's centre. The
    // centre line must be solid all the way so the level can be finished.
    for (int z = 0; z < grid.depth; z++)
        CHECK(grid.Cell(z, 2) >= '0' && grid.Cell(z, 2) <= '9', "row %d centre is not walkable", z);

    MapGeometry::Result geo = MapGeometry::Build(grid);
    CHECK(!geo.floorMesh.vertices.empty(), "the floor mesh is empty");
    CHECK(!geo.undergroundMesh.vertices.empty(), "the underground mesh is empty (no cliff faces)");
    CheckMesh("floor", geo.floorMesh);
    CheckMesh("underground", geo.undergroundMesh);
    CHECK(geo.floorMesh.QuadCount() > (size_t)grid.depth * LevelGrid::kWidth,
          "the floor has %zu quads, expected walls and floors", geo.floorMesh.QuadCount());

    std::printf("benchmark: %d rows, %zu + %zu quads, %d enemies (%d kinds), %d coins, %d torches, "
                "%d fences, %d platforms, %d props, %d candle rows, %d water rows\n",
                grid.depth, geo.floorMesh.QuadCount(), geo.undergroundMesh.QuadCount(), enemies, kinds, coins,
                torches, fences, platforms, statics, candles, water);
    std::printf("benchmark level: %s\n", g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
