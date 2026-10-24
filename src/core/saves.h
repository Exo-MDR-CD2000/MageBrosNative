// Saved progress (native replacement for the original's registry storage).
//
// Each campaign ("pack") keeps its progress in its own text file:
//   <save dir>/original/progress.ini          the original 40 levels
//   <save dir>/packs/<pack id>/progress.ini   level packs (a future modding framework)
// The save dir is %APPDATA%\MageBrosNative\saves on Windows (the per-user data folder on other
// systems) unless [game] save_path in magebros_native.ini says otherwise.
//
// Files are written to a temporary file first and then swapped in, keeping the previous version
// as progress.bak, so a crash or power cut during a save never leaves a broken file.
// Unknown sections and keys are kept when a file is rewritten, so newer versions (and mods) can
// add data without older versions destroying it.
//
// The original game's own saves (the registry values HKCU\Software\SaitoGames\MageBros\
// 1P_SLOTn / 2P_SLOTn) are not read: the port starts its own progress.
//
// Adapted from Hopmon Native's core/saves (MIT).
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace Saves {

// One pack's save file. Values are strings; sections and keys keep their order.
struct PackSave {
    static constexpr int kVersion = 1;  // save_version written by this build
    int saveVersion = kVersion;         // as read from the file
    std::string pack;                   // pack id ("original" for the base game)
    std::string packVersion;            // version of the pack the progress belongs to
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> sections;

    bool HasSection(const std::string& section) const;
    std::string Get(const std::string& section, const std::string& key, const std::string& def = "") const;
    int GetInt(const std::string& section, const std::string& key, int def) const;
    void Set(const std::string& section, const std::string& key, const std::string& value);
    void SetInt(const std::string& section, const std::string& key, int value) { Set(section, key, std::to_string(value)); }
    void RemoveSection(const std::string& section);
};

// Resolves the save folder from the config, moves saves over if save_path changed since the
// last run, and logs where saves live. Call once at startup, after the config is loaded.
void Init();

// The resolved save folder, ending with a path separator.
const std::string& Dir();

// A pack id is lowercase letters, digits, '.', '_' and '-' (it becomes a folder name).
bool IsValidPackId(const std::string& id);

// Reads a pack's progress (falling back to its backup if the main file is damaged).
// Returns false when the pack has no saved progress yet.
bool Load(const std::string& packId, PackSave* out);

// Writes a pack's progress safely. Returns false (and logs why) on failure.
bool Store(const PackSave& save);

}  // namespace Saves
