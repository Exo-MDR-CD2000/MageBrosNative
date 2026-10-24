// XSaveData (XApplication.cpp of the original): one save slot. The application plays from one
// (its level status and both heroes' status live in it) and writes it back after each level.
//
// The original keeps 8 slots for one-player games and 8 for two-player games, as binary registry
// values HKCU\Software\SaitoGames\MageBros\1P_SLOTn and 2P_SLOTn. The port keeps them as sections
// of the same names in its own save file (core/saves): ReadSlot / WriteSlot.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XHero.h"
#include "game/XLevelStatus.h"
#include <cstdint>
#include <string>

namespace Saves {
struct PackSave;
}

struct XSaveData {
    static constexpr int kSlots = 8;  // per game mode

    int8_t month = 1, day = 1;  // the date it was last saved (shown on the save slots)
    XLevelStatus levelStatus;   // +0x02
    XHeroStatus heroStatusA;    // +0x0e
    XHeroStatus heroStatusB;    // +0x14

    // XSaveData::Reset: 1 January, a new game
    void Reset() {
        month = 1;
        day = 1;
        levelStatus.Reset();
        heroStatusA.Reset();
        heroStatusB.Reset();
    }
    // XSaveData::Valid: a real date and every field in its range
    bool Valid() const {
        return month >= 1 && month <= 12 && day >= 1 && day <= 31 && levelStatus.Valid() && heroStatusA.Valid() &&
               heroStatusB.Valid();
    }

    // the slot's section name: "1P_SLOT0".."1P_SLOT7", "2P_SLOT0".."2P_SLOT7"
    static std::string SlotName(bool onePlayer, int slot);
    // XSelectSlot::_LoadSlots for one slot: the slot from the save file, or a reset one when it is
    // missing or not Valid. Returns whether it was there and valid.
    bool ReadSlot(const Saves::PackSave& file, bool onePlayer, int slot);
    // XApplication::DoSaveData's write: the slot into the save file (in memory; Saves::Store
    // writes it out)
    void WriteSlot(Saves::PackSave& file, bool onePlayer, int slot) const;
};
