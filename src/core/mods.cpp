// Mods, stage 2: the interface the engine and options menu use, with the original game as the
// only campaign and no add-ons yet. Asset replacement and level packs for Mage Bros. come with
// the modding stage (docs/ROADMAP.md); until then every lookup falls through to MageBros.exe.
#include "mods.h"
#include "assets.h"
#include "game_files.h"
#include "level.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace Mods {

namespace {
std::vector<ModInfo> g_mods;  // none installed yet
std::vector<Pack> g_packs;
std::string g_modsFolder;
const std::string kOriginal = "original";
const std::vector<std::pair<uint32_t, const char*>> kNone;
std::vector<std::pair<uint32_t, const char*>> g_levelNames;
std::vector<std::string> g_levelNameStore;

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
}  // namespace

std::string Ini::Get(const std::string& section, const std::string& key, const std::string& def) const {
    const auto* s = Section(section);
    if (!s) return def;
    for (const auto& kv : *s)
        if (kv.first == key) return kv.second;
    return def;
}

const std::vector<std::pair<std::string, std::string>>* Ini::Section(const std::string& name) const {
    for (const auto& s : sections)
        if (s.first == name) return &s.second;
    return nullptr;
}

bool ParseIni(const std::string& text, Ini* out) {
    out->sections.clear();
    out->sections.push_back({"", {}});
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t eol = text.find('\n', pos);
        std::string line = Trim(text.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos));
        pos = eol == std::string::npos ? text.size() + 1 : eol + 1;
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') {
            size_t e = line.find(']');
            std::string name = Trim(line.substr(1, e == std::string::npos ? std::string::npos : e - 1));
            for (char& c : name) c = (char)std::tolower((unsigned char)c);
            out->sections.push_back({name, {}});
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        for (char& c : key) c = (char)std::tolower((unsigned char)c);
        out->sections.back().second.push_back({key, Trim(line.substr(eq + 1))});
    }
    return true;
}

void Init() {
    g_modsFolder = Assets::GameDir() + "mods/";
    g_packs.clear();
    Pack p;
    p.id = kOriginal;
    p.name = "Mage Bros. (original game)";
    p.author = "Saito Games";
    g_levelNames.clear();
    g_levelNameStore.clear();
    g_levelNameStore.reserve(GameFiles::kWorldCount * GameFiles::kLevelsPerWorld);
    for (int w = 0; w < GameFiles::kWorldCount; w++) {
        World world;
        char name[32];
        std::snprintf(name, sizeof name, "World %d", w + 1);
        world.name = name;
        world.look = w;
        p.worlds.push_back(world);
        for (int l = 0; l < GameFiles::kLevelsPerWorld; l++) {
            uint16_t id = GameFiles::LevelResourceId(w, l);
            std::snprintf(name, sizeof name, "w%d-%02d", w + 1, l + 1);
            p.levels.push_back(id);
            p.levelWorld.push_back(w);
            p.levelNames.push_back(name);
            g_levelNameStore.push_back(name);
            g_levelNames.push_back({id, g_levelNameStore.back().c_str()});
        }
    }
    g_packs.push_back(std::move(p));
}

const std::vector<ModInfo>& Installed() { return g_mods; }
std::vector<const ModInfo*> Addons() { return {}; }
std::vector<const ModInfo*> LevelPackMods() { return {}; }
std::vector<const ModInfo*> Active() { return {}; }
const std::string& ModsFolder() { return g_modsFolder; }

void CreateFolders() {
    if (g_modsFolder.empty()) return;
    SDL_CreateDirectory(g_modsFolder.c_str());
    SDL_CreateDirectory((g_modsFolder + "levelpacks").c_str());
    SDL_CreateDirectory((g_modsFolder + "addons").c_str());
}

void StoreInConfig() {}
void SetEnabled(const std::string&, bool) {}
void Move(const std::string&, int) {}

std::string FindFile(const std::string&, const std::string&, const std::vector<std::string>&,
                     const ModInfo** from) {
    if (from) *from = nullptr;
    return std::string();
}

const char* ImageName(uint32_t) { return ""; }
const char* ModelName(uint32_t) { return ""; }
const char* SoundName(uint32_t) { return ""; }
const char* LevelName(uint32_t id) {
    for (const auto& e : g_levelNames)
        if (e.first == id) return e.second;
    return "";
}
const std::vector<std::pair<uint32_t, const char*>>& ImageNames() { return kNone; }
const std::vector<std::pair<uint32_t, const char*>>& ModelNames() { return kNone; }
const std::vector<std::pair<uint32_t, const char*>>& SoundNames() { return kNone; }
const std::vector<std::pair<uint32_t, const char*>>& LevelNames() { return g_levelNames; }

bool IsDynamic(uint32_t id) { return id >= 0x8000; }
std::string DynamicPath(uint32_t) { return std::string(); }

std::string LevelText(uint32_t id) {
    const auto* d = Assets::RCData(id);
    if (!d) return std::string();
    std::string out;
    for (const auto& line : SplitLevelLines(d->data(), d->size())) {
        if (line.empty() || line[0] == '_') break;
        out += line;
        out += '\n';
    }
    return out;
}

std::string CheckLevel(const std::string& grid) {
    LevelGrid g;
    std::string err;
    return ParseLevel((const uint8_t*)grid.data(), grid.size(), &g, &err) ? std::string() : err;
}

const std::vector<Pack>& Packs() { return g_packs; }
const Pack* FindPack(const std::string& id) {
    for (const auto& p : g_packs)
        if (p.id == id) return &p;
    return nullptr;
}
const Pack& ActivePack() { return g_packs.front(); }
const std::string& ActivePackId() { return kOriginal; }
const std::string& ChosenPackId() { return kOriginal; }
void SetActivePack(const std::string&) {}

}  // namespace Mods
