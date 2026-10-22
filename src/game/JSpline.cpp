#include "game/JSpline.h"
#include "framework/d3dutil.h"

namespace {
// POSITION_MIXER: the cubic Hermite basis for the rows (start velocity, start, end, end velocity)
const D3DMATRIX& PositionMixer() {
    static const D3DMATRIX m = [] {
        static const float kValues[16] = {1, 2, -2, 1, -2, -3, 3, -1, 1, 0, 0, 0, 0, 1, 0, 0};
        D3DMATRIX r;
        for (int i = 0; i < 16; i++) (&r._11)[i] = kValues[i];
        return r;
    }();
    return m;
}

// Spline_GetPosition: the point at `time` (0..1) on the cubic from p0 to p1 with the velocities
// v0 and v1, as (t^3, t^2, t, 1) x POSITION_MIXER x the rows (v0, p0, p1, v1)
D3DVECTOR Spline_GetPosition(const D3DVECTOR& p0, const D3DVECTOR& v0, const D3DVECTOR& p1, const D3DVECTOR& v1, float time) {
    D3DMATRIX matrix;
    D3DUtil_SetIdentityMatrix(matrix);
    matrix._11 = v0.x, matrix._12 = v0.y, matrix._13 = v0.z;
    matrix._21 = p0.x, matrix._22 = p0.y, matrix._23 = p0.z;
    matrix._31 = p1.x, matrix._32 = p1.y, matrix._33 = p1.z;
    matrix._41 = v1.x, matrix._42 = v1.y, matrix._43 = v1.z;
    D3DMATRIX matrix2;
    D3DMath_MatrixMultiply(matrix2, PositionMixer(), matrix);
    matrix2._14 = matrix2._24 = matrix2._34 = 0;
    matrix2._44 = 1;
    D3DVECTOR result(0, 0, 0);
    D3DMath_VectorMatrixMultiply(result, D3DVECTOR(time * time * time, time * time, time), matrix2);
    return result;
}
}  // namespace

void JSpline::Init(int maxNodes) {
    list_.assign(maxNodes, JSplineData());
    maxNodes_ = maxNodes;
    Reset();
}

void JSpline::Deinit() {
    list_.clear();
    maxNodes_ = 0;
}

void JSpline::Reset() {
    for (auto& d : list_) d = JSplineData();
    nodeIndex_ = 0;
    maxDistance_ = 0;
}

bool JSpline::AddNode(const D3DVECTOR& pos) {
    if (nodeIndex_ >= maxNodes_) return false;
    const int n = nodeIndex_;
    list_[n].pos = pos;
    if (n == 0) {
        list_[0].velocity = D3DVECTOR(0, 0, 0);
        list_[0].distance = 1.0f;
        maxDistance_ = 0;
        nodeIndex_++;
        return true;
    }
    // the new node's velocity points along the last segment; that segment's length is kept
    D3DVECTOR tmp = pos - list_[n - 1].pos;
    list_[n].velocity = Normalize(tmp);
    list_[n - 1].distance = Magnitude(tmp);
    maxDistance_ += list_[n - 1].distance;
    if (n == 1) {
        list_[0].velocity = list_[1].velocity;
        nodeIndex_++;
        return true;
    }
    // the previous node: the average of the directions on either side of it
    D3DVECTOR t = list_[n - 1].pos - list_[n - 2].pos;
    if (list_[n - 2].distance > 0) t *= 1.0f / list_[n - 2].distance;
    list_[n - 1].velocity = list_[n].velocity + t;
    list_[n - 1].velocity *= 0.5f;
    list_[n - 1].velocity = Normalize(list_[n - 1].velocity);
    // the new end: mirrored about the segment (3 x direction - the previous velocity)
    list_[n].velocity *= 3.0f;
    list_[n].velocity -= list_[n - 1].velocity;
    list_[n].velocity = Normalize(list_[n].velocity);
    if (n == 2) {  // and the start, the same way, once it has a neighbour with a velocity
        list_[0].velocity *= 3.0f;
        list_[0].velocity -= list_[1].velocity;
        list_[0].velocity = Normalize(list_[0].velocity);
    }
    nodeIndex_++;
    return true;
}

bool JSpline::MergeEnds() {
    const int n = nodeIndex_;
    if (n < 3) return false;
    list_[n - 1].pos = list_[0].pos;
    D3DVECTOR a = list_[1].pos - list_[0].pos;
    a *= 1.0f / list_[0].distance;
    D3DVECTOR b = list_[n - 1].pos - list_[n - 2].pos;
    b *= 1.0f / list_[n - 2].distance;
    D3DVECTOR v = a + b;
    v *= 0.5f;
    v = Normalize(v);
    list_[0].velocity = v;
    list_[n - 1].velocity = v;
    return true;
}

D3DVECTOR JSpline::GetPosition(float time) const {
    float dist = time * maxDistance_;
    float cur = 0;
    int i = 0;
    while (dist > cur + list_[i].distance && i < nodeIndex_ - 1) {
        cur += list_[i].distance;
        i++;
    }
    float t = dist - cur;
    if (list_[i].distance > 0) t /= list_[i].distance;
    if (i == nodeIndex_ - 1) return list_[i].pos;
    D3DVECTOR startVel = list_[i].velocity;
    startVel *= list_[i].distance;
    D3DVECTOR endVel = list_[i + 1].velocity;
    endVel *= list_[i].distance;
    return Spline_GetPosition(list_[i].pos, startVel, list_[i + 1].pos, endVel, t);
}
