// JSpline (JSpline.cpp of the original's J library): a smooth curve through a list of nodes, as a
// rounded non-uniform spline. Each node keeps its position, a unit velocity and the distance to
// the next node; positions along it are found by the share of the total length travelled, and
// each segment is a cubic Hermite curve (Spline_GetPosition, the POSITION_MIXER matrix).
//
// The dragon (XBoss_Dragon) moves along these.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include <vector>

class JSpline {
public:
    void Init(int maxNodes);  // room for `maxNodes` nodes, all cleared
    void Deinit();
    void Reset();
    // Appends a node; false when the list is full. The velocities of the new node and the one
    // before it are worked out as it goes.
    bool AddNode(const D3DVECTOR& pos);
    // Closes the curve: the last node is moved onto the first and both get the same velocity
    // (false with fewer than three nodes).
    bool MergeEnds();
    // The point `time` (0..1) of the way along the curve's length.
    D3DVECTOR GetPosition(float time) const;
    float GetTotalDistance() const { return maxDistance_; }

private:
    struct JSplineData {
        D3DVECTOR pos{0, 0, 0}, velocity{0, 0, 0};
        float distance = 0;
    };
    int maxNodes_ = 0, nodeIndex_ = 0;
    float maxDistance_ = 0;
    std::vector<JSplineData> list_;
};
