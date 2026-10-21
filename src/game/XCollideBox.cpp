#include "game/XCollideBox.h"
#include <cmath>

bool XCollideBox::CollisionCheck(const D3DVECTOR& p) const {
    return p.z > node1_.z && node2_.z > p.z && p.x > node1_.x && node2_.x > p.x && p.y > node1_.y && node2_.y > p.y;
}

bool XCollideBox::CollisionCheck(const XCollideBox& o) const {
    return node2_.z > o.node1_.z && o.node2_.z > node1_.z && node2_.x > o.node1_.x && o.node2_.x > node1_.x &&
           node2_.y > o.node1_.y && o.node2_.y > node1_.y;
}

// XCollideBox::CollisionCheckX: a loose test on the Z distance first (the full widths, not the
// halves), then the mover's box at the new X against this one. On a hit the mover is put against
// the side it came from; if it still overlaps (it was inside already) it stays where it was.
bool XCollideBox::CollisionCheckX(float& newX, const XCollideBox& m, bool ignoreY) const {
    if (std::fabs(pos_.z - m.pos_.z) > widthZ_ + m.widthZ_) return false;
    auto overlaps = [&](float x) {
        float n1x = x - m.halfX_, n2x = x + m.halfX_;
        return node2_.z > m.node1_.z && m.node2_.z > node1_.z && node2_.x > n1x && n2x > node1_.x &&
               (ignoreY || (node2_.y > m.node1_.y && m.node2_.y > node1_.y));
    };
    if (!overlaps(newX)) return false;
    float n1x = newX - m.halfX_;
    if (n1x > node1_.x && node2_.x > n1x) newX = node2_.x + m.halfX_;
    else newX = node1_.x - m.halfX_;
    if (overlaps(newX)) newX = m.pos_.x;
    return true;
}

bool XCollideBox::CollisionCheckZ(float& newZ, const XCollideBox& m, bool ignoreY) const {
    if (std::fabs(pos_.z - m.pos_.z) > widthZ_ + m.widthZ_) return false;  // (the Z distance here too)
    auto overlaps = [&](float z) {
        float n1z = z - m.halfZ_, n2z = z + m.halfZ_;
        return node2_.x > m.node1_.x && m.node2_.x > node1_.x && node2_.z > n1z && n2z > node1_.z &&
               (ignoreY || (node2_.y > m.node1_.y && m.node2_.y > node1_.y));
    };
    if (!overlaps(newZ)) return false;
    float n1z = newZ - m.halfZ_;
    if (n1z > node1_.z && node2_.z > n1z) newZ = node2_.z + m.halfZ_;
    else newZ = node1_.z - m.halfZ_;
    if (overlaps(newZ)) newZ = m.pos_.z;
    return true;
}

// Landing on top (the mover's foot inside this box) or bumping a head on its underside.
bool XCollideBox::CollisionCheckY(float& newY, const XCollideBox& m) const {
    if (std::fabs(pos_.z - m.pos_.z) > widthZ_ + m.widthZ_) return false;
    auto overlaps = [&](float y) {
        float n1y = y, n2y = y + m.height_;
        return node2_.z > m.node1_.z && m.node2_.z > node1_.z && node2_.x > m.node1_.x && m.node2_.x > node1_.x &&
               node2_.y > n1y && n2y > node1_.y;
    };
    if (!overlaps(newY)) return false;
    if (newY > node1_.y && node2_.y > newY) newY = node2_.y;
    else newY = node1_.y - m.height_;
    if (overlaps(newY)) newY = m.pos_.y;
    return true;
}
