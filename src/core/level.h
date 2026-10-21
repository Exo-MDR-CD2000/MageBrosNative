// Level grids, decoded the way the original's XMap::_LoadMapArray reads them.
//
// A level is text, one line per row of the playfield. Reading stops at the first empty line or
// the first line that starts with '_'. The LAST line read becomes row 0 (nearest the start).
// Each line is read by character position, without checking its length:
//   [0]       row flag
//   [2]..[6]  five height/terrain cells
//   [8]..[12] five object cells
// A line shorter than 13 characters reads its missing positions as 0 (the string's
// terminator, and zeroes past it); characters after position 12 are ignored.
// (Two of the shipped levels have one such row each; this keeps them identical to the original.)
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct LevelGrid {
    static constexpr int kWidth = 5;  // XMap's map width, fixed at 5
    int depth = 0;                    // number of rows
    std::vector<char> rowFlags;       // [depth]
    std::vector<char> cells;          // [depth * kWidth], the height/terrain field
    std::vector<char> objects;        // [depth * kWidth], the object field

    char Flag(int row) const { return rowFlags[row]; }
    char Cell(int row, int x) const { return cells[row * kWidth + x]; }
    char Object(int row, int x) const { return objects[row * kWidth + x]; }
};

// Splits the resource into lines the way the game's string list does (CR/LF or LF).
std::vector<std::string> SplitLevelLines(const uint8_t* data, size_t size);

bool ParseLevel(const uint8_t* data, size_t size, LevelGrid* out, std::string* error);
