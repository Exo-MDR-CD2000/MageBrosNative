// The benchmark level: a dense, playable level generated from the port's own data.
//
// It is not a game resource and is never listed in the campaign, level select or a mod pack: the
// benchmark mode loads it by name. The text uses the original's row format, so ParseLevel and
// the level viewer read it exactly like a shipped level. The dense part sits in the opening view
// (rows 0..18); the tail exists so the level is valid and has a goal.
//
// Object mix (final, calibrated against 3-10):
//   enemies: slime, bee, wallaby, mushroom, squid, needle ball, barrel, needle fan, turtle, kiwi,
//            hammer, ghost, needle, jump needle, stalactite, fish
//   props:   large and small torches, coins, fences, pillars, trees, plants, pots, barrels,
//            cannons and moving platforms
//   rows with 'C' add wall candles; rows with 'W' make the level a water level.
#pragma once
#include "level.h"
#include <cstdint>
#include <string>

namespace BenchmarkLevel {

constexpr int kDepth = 40;

// The level text in the original's format (the last line is row 0).
const std::string& Text();
// Parses Text() into a grid. False only if the generated text is malformed (never in a release).
bool Build(LevelGrid* out, std::string* error);
// A stable hash of the text, logged at start so two runs can be shown to use the same level.
uint32_t Hash();
// The level is drawn with the castle look (world 0).
constexpr int kWorld = 0;
constexpr int kLevel = 0;

}  // namespace BenchmarkLevel
