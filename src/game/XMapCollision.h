// XMap's collision with the level (XMap::CollisionCheckX / Y / Z, IsBlocked, ConvertPos*ToMap*).
// See docs/REVERSE_ENGINEERING.md, "Collision with the map".
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XCollideBox.h"
#include "game/XMapGeometry.h"
#include <vector>

class MapCollision {
public:
    MapCollision() = default;
    // `cells` is the floor field after bridges are resolved (MapGeometry::Result::cells).
    MapCollision(std::vector<char> cells, int depth) : cells_(std::move(cells)), depth_(depth) {}

    // Each corrects `newValue` in place for a box moving there; true on a hit.
    bool CollisionCheckX(float& newX, const XCollideBox& box) const;
    bool CollisionCheckZ(float& newZ, const XCollideBox& box) const;
    bool CollisionCheckY(float& newY, const XCollideBox& box) const;  // only while moving down
    // Outside the map, or the floor at (x, z) is higher than `level`.
    bool IsBlocked(int x, int z, int level) const;
    // XMap::IsBlockedFloat: IsBlocked at a world position (cells with the floor flag set)
    bool IsBlockedFloat(float x, float z, float y) const { return IsBlocked(CellX(x, true), CellZ(z, true), CellY(y)); }
    // -1 below 0; with `floorFlag` trunc(p / 40), else ceil(p / 40) - 1.
    static int CellX(float p, bool floorFlag);
    static int CellZ(float p, bool floorFlag) { return CellX(p, floorFlag); }
    static int CellY(float p);
    // XMap::GetFloorLevel: the floor level 0..9 of a cell, -1 for a hole or outside the map
    int GetFloorLevel(int x, int z) const { return MapGeometry::FloorLevel(cells_, depth_, x, z); }
    int Depth() const { return depth_; }

private:
    std::vector<char> cells_;
    int depth_ = 0;
};
