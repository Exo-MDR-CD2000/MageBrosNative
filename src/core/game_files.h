// Access to the user's own copy of Mage Bros. Nothing from the game is bundled with this
// project: every image, model, sound and level is read at runtime from MageBros.exe, and the
// music from the Music01-06.mid files next to it.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "pe_resources.h"

namespace GameFiles {

constexpr int kWorldCount = 4;
constexpr int kLevelsPerWorld = 10;
constexpr int kMusicTrackCount = 6;

// Find and open MageBros.exe. `explicitPath` may be a file or a folder, or empty to search the
// program's own folder and the working directory. Builds whose SHA-256 is unknown are refused
// unless `allowUnverified` is set.
bool Open(const std::string& explicitPath, const std::string& programDir, bool allowUnverified,
          std::string* error);

bool IsOpen();
bool IsVerified();
const std::string& ExePath();
const std::string& GameDir();   // folder of MageBros.exe, with a trailing separator
const std::string& BuildLabel();
const PEResources& Resources();

// Raw resources by ID (nullptr when missing).
const std::vector<uint8_t>* Bitmap(uint32_t id);  // DIB: BITMAPINFOHEADER + palette + bits
const std::vector<uint8_t>* RCData(uint32_t id);  // levels and .x models
const std::vector<uint8_t>* Wave(uint32_t id);    // RIFF WAVE

// Resource ID of a level, as the original's GetMapResId(world, level) chooses it.
// world 0..3, level 0..9; returns 0 when out of range.
uint16_t LevelResourceId(int world, int level);

// Path of a music file next to the game ("Music01.mid" for track 1), empty if missing.
std::string MusicPath(int track);

// The kinds of resource the game stores, used by the tools.
enum class Kind { Bitmap, Wave, Level, Model, Other };
struct Entry {
    Kind kind;
    uint32_t type;  // numeric type; 0 for string-typed ("WAVE")
    uint32_t id;
    const std::vector<uint8_t>* data;
};
std::vector<Entry> ListGameResources();
const char* KindName(Kind k);

}  // namespace GameFiles
