// Builds the geometry of all 40 levels from the user's MageBros.exe and checks it:
//   mbn_map_tests --game PATH
// Not run by ctest unless MBN_GAME_EXE is set at configure time (it needs the game).
#include <cmath>
#include <cstdio>
#include <string>
#include "core/game_files.h"
#include "core/level.h"
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

static float Dist(const D3DLVERTEX& a, const D3DLVERTEX& b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Every quad keeps its size through the rotations: opposite edges equal, diagonals equal.
static bool QuadIsRectangle(const D3DLVERTEX* v) {
    auto near = [](float a, float b) { return std::fabs(a - b) < 0.01f; };
    return near(Dist(v[0], v[1]), Dist(v[2], v[3])) && near(Dist(v[0], v[2]), Dist(v[1], v[3])) &&
           near(Dist(v[0], v[3]), Dist(v[1], v[2])) && Dist(v[0], v[1]) > 1.0f && Dist(v[0], v[2]) > 1.0f;
}

static void CheckMesh(const char* name, const char* which, const MapGeometry::Mesh& m, float zMax) {
    CHECK(m.vertices.size() % 4 == 0 && m.indices.size() == m.vertices.size() / 4 * 6, "%s %s: mesh shape", name, which);
    for (WORD i : m.indices) CHECK(i < m.vertices.size(), "%s %s: index %u out of range", name, which, i);
    if (m.vertices.size() > 65535) CHECK(false, "%s %s: too many vertices for 16-bit indices", name, which);
    for (size_t q = 0; q < m.vertices.size(); q += 4) {
        const D3DLVERTEX* v = &m.vertices[q];
        CHECK(QuadIsRectangle(v), "%s %s: quad %zu is not a rectangle", name, which, q / 4);
        for (int k = 0; k < 4; k++) {
            const auto& p = v[k];
            bool inside = std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && p.x >= -0.01f &&
                          p.x <= 200.01f && p.y >= -200.01f && p.y <= 320.01f && p.z >= -100.01f && p.z <= zMax + 100.01f;
            CHECK(inside, "%s %s: vertex (%g, %g, %g) outside the level", name, which, p.x, p.y, p.z);
            CHECK(p.tu >= 0 && p.tu <= 1 && p.tv >= 0 && p.tv <= 1, "%s %s: UV out of range", name, which);
        }
    }
}

int main(int argc, char** argv) {
    std::string game;
    for (int i = 1; i + 1 < argc; i++)
        if (std::string(argv[i]) == "--game") game = argv[i + 1];
    std::string err;
    if (!GameFiles::Open(game, "", true, &err)) {
        std::printf("%s\n", err.c_str());
        return 2;
    }
    size_t totalQuads = 0;
    for (int w = 0; w < GameFiles::kWorldCount; w++)
        for (int l = 0; l < GameFiles::kLevelsPerWorld; l++) {
            char name[16];
            std::snprintf(name, sizeof name, "w%d-%02d", w + 1, l + 1);
            const auto* d = GameFiles::RCData(GameFiles::LevelResourceId(w, l));
            LevelGrid g;
            if (!d || !ParseLevel(d->data(), d->size(), &g, &err)) {
                CHECK(false, "%s: level unreadable", name);
                continue;
            }
            int digits = 0, bridges = 0;
            for (char c : g.cells) {
                digits += c >= '0' && c <= '9';
                bridges += c == 'H';
            }
            MapGeometry::Result r = MapGeometry::Build(g);
            float zMax = g.depth * MapGeometry::kCellSize;
            CheckMesh(name, "floor", r.floorMesh, zMax);
            CheckMesh(name, "underground", r.undergroundMesh, zMax);

            // Floor tiles are the horizontal 40x40 quads at a height step; bridges add one deck each.
            int flat = 0;
            for (size_t q = 0; q < r.floorMesh.vertices.size(); q += 4) {
                const D3DLVERTEX* v = &r.floorMesh.vertices[q];
                bool horizontal = std::fabs(v[0].y - v[1].y) < 0.01f && std::fabs(v[0].y - v[2].y) < 0.01f &&
                                  std::fabs(v[0].y - v[3].y) < 0.01f;
                if (horizontal) flat++;
            }
            CHECK(flat == digits + bridges, "%s: %d flat tiles for %d floor cells + %d bridges", name, flat, digits, bridges);
            for (char c : r.cells) CHECK(c != 'H', "%s: a bridge cell was not resolved", name);

            size_t quads = r.floorMesh.QuadCount() + r.undergroundMesh.QuadCount();
            totalQuads += quads;
            std::printf("%s  %3d rows  %4d floor cells  %2d bridges  %5zu + %4zu quads\n", name, g.depth, digits,
                        bridges, r.floorMesh.QuadCount(), r.undergroundMesh.QuadCount());
        }
    std::printf("%zu quads in all 40 levels. %s\n", totalQuads, g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
