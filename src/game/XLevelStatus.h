// XLevelStatus (XApplication.cpp of the original): where the players are and how far they have
// got, kept by the application across levels in its XSaveData.
//
// Worlds 1 to 3 are open from the start; world 4 opens once all three are cleared. Until then,
// clearing world 3 goes back to world 1.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <cstdint>

struct XLevelStatus {
    int8_t currentWorld = 0;       // 0-based
    int8_t currentLevel = 0;       // 0-based
    int8_t maxSelWorld = 2;        // the last world level select offers
    int8_t maxSelLevel[4] = {};    // the last level reached in each world
    int8_t bossDefeated[4] = {};   // each world cleared (its level 10 won)

    // XLevelStatus::Reset: a new game
    void Reset() {
        currentWorld = 0;
        currentLevel = 0;
        for (auto& b : bossDefeated) b = 0;
        maxSelWorld = 2;
        for (auto& m : maxSelLevel) m = 0;
    }
    // XLevelStatus::IncreaseLevel: the next level of the world (wrapping after 10), which level
    // select then offers
    void IncreaseLevel() {
        if (++currentLevel >= 10) currentLevel = 0;
        if (currentLevel > maxSelLevel[currentWorld]) maxSelLevel[currentWorld] = currentLevel;
    }
    // XLevelStatus::IncreaseWorld: the world is cleared; on to the first level of the next one.
    // World 4 opens once worlds 1 to 3 are all cleared; until then world 3 leads back to world 1.
    void IncreaseWorld() {
        currentLevel = 0;
        bossDefeated[currentWorld] = 1;
        if (++currentWorld >= 4) currentWorld = 0;
        if (bossDefeated[0] == 1 && bossDefeated[1] == 1 && bossDefeated[2] == 1) {
            maxSelWorld = 3;
        } else if (currentWorld == 3) {
            currentWorld = 0;
        } else if (currentWorld > maxSelWorld) {
            maxSelWorld = currentWorld;
        }
    }
    // XLevelStatus::Valid: worlds 0..3, levels 0..9, each world cleared 0 or 1
    bool Valid() const {
        if (currentWorld < 0 || currentWorld >= 4 || currentLevel < 0 || currentLevel >= 10) return false;
        if (maxSelWorld < 0 || maxSelWorld >= 4) return false;
        for (int8_t m : maxSelLevel)
            if (m < 0 || m >= 10) return false;
        for (int8_t b : bossDefeated)
            if (b != 0 && b != 1) return false;
        return true;
    }
    bool IsLastLevel() const { return currentLevel == 9; }
    bool IsLastWorld() const { return currentWorld == 3; }
};
