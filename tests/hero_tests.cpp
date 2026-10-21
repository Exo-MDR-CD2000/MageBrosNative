// Runs player 1's movement on all 40 levels of the user's MageBros.exe, without a renderer:
//   mbn_hero_tests --game PATH
// Checks: the drop from y = 800 lands on the start floor; a jump from flat ground rises exactly
// as the original's per-tick integration does (speed 3.5, minus 0.08 before each step: 3.42 +
// 3.34 + ... over 43 ticks = 74.82); and during long random inputs the hero's box never ends up
// inside a wall or floor (it may only be inside the level geometry while dying).
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include "core/game_files.h"
#include "core/level.h"
#include "game/XHero.h"
#include "game/XMapCollision.h"

static int g_fail = 0;
#define CHECK(c, ...) do { if (!(c)) { std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); g_fail++; } } while (0)

// No part of the box at its height step is in a blocked cell.
static bool Inside(const MapCollision& m, const D3DVECTOR& p) {
    const float h = 13.0f;
    int y = MapCollision::CellY(p.y);
    float xs[3] = {p.x - h, p.x, p.x + h}, zs[3] = {p.z - h, p.z, p.z + h};
    for (float x : xs)
        for (float z : zs) {
            int cx = MapCollision::CellX(x, true), cz = MapCollision::CellZ(z, true);
            // a box exactly on a cell boundary touches, but isn't inside, the next cell
            if (std::fmod(x, 40.0f) == 0.0f && x == p.x + h) cx--;
            if (std::fmod(z, 40.0f) == 0.0f && z == p.z + h) cz--;
            if (y >= 0 && m.IsBlocked(cx, cz, y)) return true;
        }
    return false;
}

int main(int argc, char** argv) {
    std::string game;
    for (int i = 1; i + 1 < argc; i++)
        if (std::string(argv[i]) == "--game") game = argv[i + 1];
    std::string err;
    if (!GameFiles::Open(game, "", true, &err)) { std::printf("%s\n", err.c_str()); return 2; }
    // the level as the hero sees it: no sounds, shots, goal or static objects
    struct QuietWorld : XHeroWorld {
        void PlaySound(uint16_t, float) override {}
        void CreateFire(const XHero&) override {}
        bool ReadyForWarp() const override { return false; }
        D3DVECTOR GoalPos() const override { return D3DVECTOR(0, 0, 0); }
        void CloseWarpZone() override {}
    } quiet;
    std::mt19937 rng(1234);
    long ticks = 0, deaths = 0;
    for (int w = 0; w < GameFiles::kWorldCount; w++)
        for (int l = 0; l < GameFiles::kLevelsPerWorld; l++) {
            char name[16];
            std::snprintf(name, sizeof name, "w%d-%02d", w + 1, l + 1);
            const auto* d = GameFiles::RCData(GameFiles::LevelResourceId(w, l));
            LevelGrid g;
            if (!d || !ParseLevel(d->data(), d->size(), &g, &err)) { CHECK(false, "%s unreadable", name); continue; }
            MapGeometry::Result geo = MapGeometry::Build(g);
            MapCollision map(geo.cells, g.depth);

            // 1. the drop lands on the start cell's floor
            XHero hero;
            hero.SetStartPos(D3DVECTOR(100, 0, 60));
            hero.Reset();
            XHeroInput none;
            for (int t = 0; t < 400; t++) hero.ProcessFrame(none, map, quiet);
            int floor = MapGeometry::FloorLevel(geo.cells, g.depth, 2, 1);
            if (floor >= 0) {
                CHECK(hero.GetLiveState() == XHero::NORMAL && hero.GetJumpState() == XHero::LAND &&
                          std::fabs(hero.GetPos().y - floor * 25.0f) < 0.001f,
                      "%s: drop ended at y %.3f (state %d/%d), floor %d", name, hero.GetPos().y,
                      hero.GetLiveState(), hero.GetJumpState(), floor);
                // 2. jump height from standing (only where nothing is overhead: the map has no ceilings)
                float base = hero.GetPos().y, peak = base;
                XHeroInput jump;
                jump.button2Pressed = jump.button2Down = true;
                hero.ProcessFrame(jump, map, quiet);
                for (int t = 0; t < 200; t++) {
                    hero.ProcessFrame(none, map, quiet);
                    peak = std::max(peak, hero.GetPos().y);
                }
                CHECK(std::fabs(peak - base - 74.82f) < 0.01f, "%s: jump rose %.3f, expected 74.82", name, peak - base);
                CHECK(std::fabs(hero.GetPos().y - base) < 0.001f, "%s: didn't land back (y %.2f)", name, hero.GetPos().y);
            }

            // 3. random play: never inside the level
            hero.Reset();
            XHeroInput in;
            int hold = 0;
            for (int t = 0; t < 20000; t++) {
                if (hold-- <= 0) {
                    int k = rng() % 9;
                    in = XHeroInput();
                    in.keyRight = k == 1 || k == 5 || k == 6;
                    in.keyLeft = k == 2 || k == 7;
                    in.keyUp = k == 3 || k == 5;
                    in.keyDown = k == 4 || k == 6 || k == 7;
                    in.keyControlUsed = k != 0 && k != 8;
                    if (k == 8) { in.valueX = (rng() % 2001 - 1000) / 1000.0f; in.valueY = (rng() % 2001 - 1000) / 1000.0f; }
                    hold = rng() % 120;
                }
                bool jumpNow = rng() % 40 == 0;
                in.button2Pressed = jumpNow;
                in.button2Down = jumpNow;
                hero.ProcessFrame(in, map, quiet);
                ticks++;
                XHero::LiveState s = hero.GetLiveState();
                if (s == XHero::NORMAL)
                    CHECK(!Inside(map, hero.GetPos()), "%s tick %d: inside the level at (%.2f, %.2f, %.2f)", name, t,
                          hero.GetPos().x, hero.GetPos().y, hero.GetPos().z);
                if (s == XHero::DEAD) { deaths++; hero.Reset(); }
                if (g_fail > 20) return 1;
            }
        }
    std::printf("%ld ticks on 40 levels, %ld falls out of the level. %s\n", ticks, deaths, g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
