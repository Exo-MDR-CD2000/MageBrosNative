// XCollideBox: an axis-aligned box. `pos` is its bottom centre; `node1` and `node2` are its low
// and high corners. See docs/REVERSE_ENGINEERING.md, "Collision with the map".
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"

class XCollideBox {
public:
    XCollideBox(float width, float height)
        : widthX_(width), widthZ_(width), halfX_(width * 0.5f), halfZ_(width * 0.5f), height_(height) {}
    // XCollideBox(app, D3DVECTOR size): x width, height, z width
    XCollideBox(float widthX, float height, float widthZ)
        : widthX_(widthX), widthZ_(widthZ), halfX_(widthX * 0.5f), halfZ_(widthZ * 0.5f), height_(height) {}

    void SetPos(const D3DVECTOR& p) {
        SetPosX(p.x);
        SetPosZ(p.z);
        SetPosY(p.y);
    }
    void SetPosX(float x) {
        pos_.x = x;
        node1_.x = x - halfX_;
        node2_.x = x + halfX_;
    }
    void SetPosZ(float z) {
        pos_.z = z;
        node1_.z = z - halfZ_;
        node2_.z = z + halfZ_;
    }
    void SetPosY(float y) {
        pos_.y = y;
        node1_.y = y;
        node2_.y = y + height_;
    }

    const D3DVECTOR& Pos() const { return pos_; }
    const D3DVECTOR& Node1() const { return node1_; }
    const D3DVECTOR& Node2() const { return node2_; }
    float WidthX() const { return widthX_; }
    float WidthZ() const { return widthZ_; }
    float WidthHalfX() const { return halfX_; }
    float WidthHalfZ() const { return halfZ_; }
    float Height() const { return height_; }

    // Strictly inside the box (the shots' test against objects and heroes).
    bool CollisionCheck(const D3DVECTOR& p) const;
    // The two boxes overlap (strictly).
    bool CollisionCheck(const XCollideBox& other) const;
    // `mover` moving to newX / newZ / newY against this box: the coordinate is corrected in place
    // and true returned on a hit. ignoreY skips the height overlap (enemies use it).
    bool CollisionCheckX(float& newX, const XCollideBox& mover, bool ignoreY) const;
    bool CollisionCheckZ(float& newZ, const XCollideBox& mover, bool ignoreY) const;
    bool CollisionCheckY(float& newY, const XCollideBox& mover) const;

private:
    D3DVECTOR pos_{0, 0, 0};
    float widthX_, widthZ_, halfX_, halfZ_, height_;
    D3DVECTOR node1_{0, 0, 0}, node2_{0, 0, 0};
};
