// The wall candles (XCandleList.cpp and XCandle.cpp of the original): one on the back wall of
// every row whose flag (the first character of its line) is 'C' (XMap::_ConstructWallObjects),
// high up at x 19.8, y 175. Each has a small box the heroes can stand on and bump into and the
// shots stop at, a steady flame (XParticle_CandleFire1) and a flicker rising from it every 15
// ticks (XParticle_CandleFire2).
//
// XCandleList keeps 30 slots, filled in order; its loops stop at the first empty slot.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/XCollideBox.h"
#include <memory>

class Device;
class JObject3DX;
struct XGameContext;

class XCandle {
public:
    XCandle(const XGameContext* game, const D3DVECTOR& pos);
    ~XCandle();
    void ProcessFrame();
    void Render(Device* d);
    const XCollideBox& GetCollideBox() const { return *collideBox_; }

private:
    const XGameContext* game_;
    unsigned localCounter_ = 0;
    D3DVECTOR pos_, firePos_;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XCollideBox> collideBox_;
};

class XCandleList {
public:
    static constexpr int kSlots = 30;
    explicit XCandleList(const XGameContext* game) : game_(game) {}
    ~XCandleList();

    void Empty();
    // XCandleList::CreateCandle: `pos` is the row's (0, 0, z); the candle places itself
    void CreateCandle(const D3DVECTOR& pos);
    void ProcessFrame();
    void Render(Device* d);

    // A box moving against the candles; each corrects the coordinate in place, true on a hit.
    bool CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckY(float& newY, const XCollideBox& box) const;
    bool CollisionCheck(const D3DVECTOR& p) const;  // a point inside a candle (shots)

private:
    int GetNullIndex() const;

    const XGameContext* game_;
    std::unique_ptr<XCandle> slots_[kSlots];
};
