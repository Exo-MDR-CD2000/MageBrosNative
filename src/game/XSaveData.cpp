#include "XSaveData.h"
#include "core/saves.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
// four small numbers as "a,b,c,d"
std::string JoinFour(const int8_t v[4]) {
    char s[32];
    std::snprintf(s, sizeof s, "%d,%d,%d,%d", v[0], v[1], v[2], v[3]);
    return s;
}
bool SplitFour(const std::string& s, int8_t out[4]) {
    int v[4];
    char extra;
    if (std::sscanf(s.c_str(), "%d,%d,%d,%d%c", &v[0], &v[1], &v[2], &v[3], &extra) != 4) return false;
    for (int i = 0; i < 4; i++) {
        if (v[i] < -128 || v[i] > 127) return false;
        out[i] = (int8_t)v[i];
    }
    return true;
}
// a value that must fit the original's field; anything else marks the slot unreadable
bool GetField(const Saves::PackSave& f, const std::string& sec, const char* key, int lo, int hi, int* out) {
    const int kMissing = -100000;
    int v = f.GetInt(sec, key, kMissing);
    if (v < lo || v > hi) return false;
    *out = v;
    return true;
}
bool ReadHero(const Saves::PackSave& f, const std::string& sec, const char* p, XHeroStatus* h) {
    int gold, life, magic, magicLevel, wand;
    std::string k = p;
    if (!GetField(f, sec, (k + "_gold").c_str(), -32768, 32767, &gold) ||
        !GetField(f, sec, (k + "_life").c_str(), -128, 127, &life) ||
        !GetField(f, sec, (k + "_magic").c_str(), -128, 127, &magic) ||
        !GetField(f, sec, (k + "_magic_level").c_str(), -128, 127, &magicLevel) ||
        !GetField(f, sec, (k + "_wand").c_str(), -128, 127, &wand))
        return false;
    h->gold = (int16_t)gold;
    h->life = (int8_t)life;
    h->magicPoint = (int8_t)magic;
    h->magicLevel = (int8_t)magicLevel;
    h->wandType = (int8_t)wand;
    return true;
}
void WriteHero(Saves::PackSave& f, const std::string& sec, const char* p, const XHeroStatus& h) {
    std::string k = p;
    f.SetInt(sec, k + "_gold", h.gold);
    f.SetInt(sec, k + "_life", h.life);
    f.SetInt(sec, k + "_magic", h.magicPoint);
    f.SetInt(sec, k + "_magic_level", h.magicLevel);
    f.SetInt(sec, k + "_wand", h.wandType);
}
}  // namespace

std::string XSaveData::SlotName(bool onePlayer, int slot) {
    char s[16];
    std::snprintf(s, sizeof s, onePlayer ? "1P_SLOT%d" : "2P_SLOT%d", slot);  // the original's value names
    return s;
}

bool XSaveData::ReadSlot(const Saves::PackSave& file, bool onePlayer, int slot) {
    Reset();
    std::string sec = SlotName(onePlayer, slot);
    if (!file.HasSection(sec)) return false;
    XSaveData d;
    int month, day, world, level, maxWorld;
    bool ok = GetField(file, sec, "month", -128, 127, &month) && GetField(file, sec, "day", -128, 127, &day) &&
              GetField(file, sec, "world", -128, 127, &world) && GetField(file, sec, "level", -128, 127, &level) &&
              GetField(file, sec, "max_world", -128, 127, &maxWorld) &&
              SplitFour(file.Get(sec, "max_level"), d.levelStatus.maxSelLevel) &&
              SplitFour(file.Get(sec, "boss_defeated"), d.levelStatus.bossDefeated) &&
              ReadHero(file, sec, "a", &d.heroStatusA) && ReadHero(file, sec, "b", &d.heroStatusB);
    if (!ok) return false;
    d.month = (int8_t)month;
    d.day = (int8_t)day;
    d.levelStatus.currentWorld = (int8_t)world;
    d.levelStatus.currentLevel = (int8_t)level;
    d.levelStatus.maxSelWorld = (int8_t)maxWorld;
    if (!d.Valid()) return false;  // _LoadSlots: an invalid slot is reset
    *this = d;
    return true;
}

void XSaveData::WriteSlot(Saves::PackSave& file, bool onePlayer, int slot) const {
    std::string sec = SlotName(onePlayer, slot);
    // (keys set in place: the slot keeps its place in the file, and keys a newer version added stay)
    file.SetInt(sec, "month", month);
    file.SetInt(sec, "day", day);
    file.SetInt(sec, "world", levelStatus.currentWorld);
    file.SetInt(sec, "level", levelStatus.currentLevel);
    file.SetInt(sec, "max_world", levelStatus.maxSelWorld);
    file.Set(sec, "max_level", JoinFour(levelStatus.maxSelLevel));
    file.Set(sec, "boss_defeated", JoinFour(levelStatus.bossDefeated));
    WriteHero(file, sec, "a", heroStatusA);
    WriteHero(file, sec, "b", heroStatusB);
}
