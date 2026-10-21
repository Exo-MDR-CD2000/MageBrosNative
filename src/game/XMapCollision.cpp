#include "XMapCollision.h"
#include <cmath>

int MapCollision::CellX(float p, bool floorFlag) {
    if (p < 0.0f) return -1;
    if (floorFlag) return (int)(p / MapGeometry::kCellSize);
    return (int)std::ceil(p / MapGeometry::kCellSize) - 1;
}

int MapCollision::CellY(float p) {
    if (p < 0.0f) return -1;
    return (int)(p / MapGeometry::kLevelHeight);
}

bool MapCollision::IsBlocked(int x, int z, int level) const {
    if (x < 0 || x >= LevelGrid::kWidth || z < 0 || z >= depth_) return true;
    return level < MapGeometry::FloorLevel(cells_, depth_, x, z);
}

bool MapCollision::CollisionCheckX(float& newX, const XCollideBox& box) const {
    const D3DVECTOR& pos = box.Pos();
    if (newX == pos.x) return false;
    float half = box.WidthHalfX();
    int x0 = CellX(newX - half, true), x1 = CellX(newX + half, false);
    int za = CellZ(box.Node1().z, true), zb = CellZ(pos.z, true), zc = CellZ(box.Node2().z, false);
    int y = CellY(pos.y);
    bool hit = false;
    if (pos.x > newX && (IsBlocked(x0, za, y) || IsBlocked(x0, zb, y) || IsBlocked(x0, zc, y))) {
        newX = (x0 + 1) * MapGeometry::kCellSize + half;
        hit = true;
    }
    if (newX > pos.x && (IsBlocked(x1, za, y) || IsBlocked(x1, zb, y) || IsBlocked(x1, zc, y))) {
        newX = x1 * MapGeometry::kCellSize - half;
        hit = true;
    }
    if (hit) {
        x0 = CellX(newX - half, true);
        x1 = CellX(newX + half, false);
        if (IsBlocked(x0, za, y) || IsBlocked(x0, zb, y) || IsBlocked(x0, zc, y) || IsBlocked(x1, za, y) ||
            IsBlocked(x1, zb, y) || IsBlocked(x1, zc, y))
            newX = pos.x;
    }
    return hit;
}

bool MapCollision::CollisionCheckZ(float& newZ, const XCollideBox& box) const {
    const D3DVECTOR& pos = box.Pos();
    if (newZ == pos.z) return false;
    float half = box.WidthHalfZ();
    int z0 = CellZ(newZ - half, true), z1 = CellZ(newZ + half, false);
    int xa = CellX(box.Node1().x, true), xb = CellX(pos.x, true), xc = CellX(box.Node2().x, false);
    int y = CellY(pos.y);
    bool hit = false;
    if (pos.z > newZ && (IsBlocked(xa, z0, y) || IsBlocked(xb, z0, y) || IsBlocked(xc, z0, y))) {
        newZ = (z0 + 1) * MapGeometry::kCellSize + half;
        hit = true;
    }
    if (newZ > pos.z && (IsBlocked(xa, z1, y) || IsBlocked(xb, z1, y) || IsBlocked(xc, z1, y))) {
        newZ = z1 * MapGeometry::kCellSize - half;
        hit = true;
    }
    if (hit) {
        z0 = CellZ(newZ - half, true);
        z1 = CellZ(newZ + half, false);
        if (IsBlocked(xa, z0, y) || IsBlocked(xb, z0, y) || IsBlocked(xc, z0, y) || IsBlocked(xa, z1, y) ||
            IsBlocked(xb, z1, y) || IsBlocked(xc, z1, y))
            newZ = pos.z;
    }
    return hit;
}

bool MapCollision::CollisionCheckY(float& newY, const XCollideBox& box) const {
    const D3DVECTOR& pos = box.Pos();
    int xs[3] = {CellX(box.Node1().x, true), CellX(pos.x, true), CellX(box.Node2().x, false)};
    int zs[3] = {CellZ(box.Node1().z, true), CellZ(pos.z, true), CellZ(box.Node2().z, false)};
    auto anyBlocked = [&](int level) {
        for (int x : xs)
            for (int z : zs)
                if (IsBlocked(x, z, level)) return true;
        return false;
    };
    bool hit = false;
    if (pos.y > newY) {
        int y = CellY(newY);
        if (anyBlocked(y)) {
            newY = (y + 1) * MapGeometry::kLevelHeight;
            hit = true;
        }
    }
    if (hit && anyBlocked(CellY(newY))) newY = pos.y;
    return hit;
}
