#include "XMapGeometry.h"
#include "framework/d3dutil.h"

namespace MapGeometry {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kHalfPi = 1.5707964f;
constexpr float kInset = 0.00390625f;  // half a texel of a 128-pixel texture, as the original
constexpr int kWidth = LevelGrid::kWidth;

enum Axis { kAxisX = 0, kAxisY = 1, kAxisZ = 2 };

// JObject3D_Rect: a w x h quad lying in the XZ plane, centred on the origin.
struct Quad {
    D3DLVERTEX v[4];

    Quad(float w, float h) {
        float hw = w / 2.0f, hh = h / 2.0f;
        const float pos[4][3] = {{-hw, 0, hh}, {hw, 0, hh}, {-hw, 0, -hh}, {hw, 0, -hh}};
        for (int i = 0; i < 4; i++) {
            v[i] = D3DLVERTEX{};
            v[i].x = pos[i][0];
            v[i].y = pos[i][1];
            v[i].z = pos[i][2];
            v[i].color = 0xFFFFFFFF;
        }
    }

    // JObject3D_Rect::SetUV(u1, u2, v1, v2, inset)
    Quad& UV(float u1, float u2, float v1, float v2, float inset = kInset) {
        float ua = u1 + inset, ub = u2 - inset, va = v1 + inset, vb = v2 - inset;
        v[0].tu = ua; v[0].tv = va;
        v[1].tu = ub; v[1].tv = va;
        v[2].tu = ua; v[2].tv = vb;
        v[3].tu = ub; v[3].tv = vb;
        return *this;
    }

    // JObject3D::Rotate / Shift: transform the vertices in place.
    void Apply(const D3DMATRIX& m) {
        for (auto& lv : v) {
            D3DVECTOR in(lv.x, lv.y, lv.z), out;
            D3DMath_VectorMatrixMultiply(out, in, m);
            lv.x = out.x;
            lv.y = out.y;
            lv.z = out.z;
        }
    }
    Quad& Rotate(Axis axis, float a) {
        D3DMATRIX m;
        if (axis == kAxisX) D3DUtil_SetRotateXMatrix(m, a);
        else if (axis == kAxisY) D3DUtil_SetRotateYMatrix(m, a);
        else D3DUtil_SetRotateZMatrix(m, a);
        Apply(m);
        return *this;
    }
    Quad& Shift(float x, float y, float z) {
        D3DMATRIX m;
        D3DUtil_SetTranslateMatrix(m, x, y, z);
        Apply(m);
        return *this;
    }
};

// The merge in _ConstructFloorAndWall: vertices in creation order, indices offset per quad.
void Append(Mesh& mesh, const Quad& q) {
    static const WORD kIdx[6] = {0, 1, 3, 0, 3, 2};
    WORD base = (WORD)mesh.vertices.size();
    for (const auto& lv : q.v) mesh.vertices.push_back(lv);
    for (WORD i : kIdx) mesh.indices.push_back((WORD)(base + i));
}

// The three orientations the builders use for an upright quad, by the side it faces.
Quad& FacingPlusX(Quad& q) { return q.Rotate(kAxisY, -kHalfPi).Rotate(kAxisZ, -kHalfPi); }
Quad& FacingPlusZ(Quad& q) { return q.Rotate(kAxisY, kPi).Rotate(kAxisX, kHalfPi); }
Quad& FacingMinusZ(Quad& q) { return q.Rotate(kAxisX, -kHalfPi); }

struct Builder {
    const LevelGrid& grid;
    std::vector<char> cells;
    int depth;
    Result out;

    explicit Builder(const LevelGrid& g) : grid(g), cells(g.cells), depth(g.depth) {}

    int Level(int x, int z) const { return FloorLevel(cells, depth, x, z); }

    // _CreateSideWalls: end walls at both ends, and walls and cliffs beyond them.
    void SideWalls() {
        for (int x = 0; x < kWidth; x++) {
            float px = x * kCellSize + 20.0f;
            Quad a(40.0f, 320.0f);
            a.UV(0.75f, 1.0f, 0.0f, 1.0f);
            FacingPlusZ(a).Shift(px, 160.0f, 0.0f);
            Append(out.floorMesh, a);
            Quad b(40.0f, 320.0f);
            b.UV(0.75f, 1.0f, 0.0f, 1.0f);
            FacingMinusZ(b).Shift(px, 160.0f, depth * kCellSize);
            Append(out.floorMesh, b);
            // native: where the end row is a pit, the end wall carries on below the floor as a
            // 200-deep cliff face, as every other pit edge does (_CreateUnderground). The original
            // has none, so at the start of 3-9, which opens on a row of pits, a black strip shows
            // on the end wall between the floor's height and the water.
            if (Level(x, 0) < 0) {
                Quad c(40.0f, 200.0f);
                c.UV(0.625f, 0.875f, 0.0f, 1.0f);
                FacingPlusZ(c).Shift(px, -100.0f, 0.0f);
                Append(out.undergroundMesh, c);
            }
            if (Level(x, depth - 1) < 0) {
                Quad c(40.0f, 200.0f);
                c.UV(0.625f, 0.875f, 0.0f, 1.0f);
                FacingMinusZ(c).Shift(px, -100.0f, depth * kCellSize);
                Append(out.undergroundMesh, c);
            }
        }
        for (int i = 0; i < 2; i++) {
            float off = i * kCellSize + 20.0f;
            float frontX = kWidth * kCellSize;
            Quad a(40.0f, 320.0f);
            a.UV(0.5f, 0.75f, 0.0f, 1.0f);
            FacingPlusX(a).Shift(frontX, 160.0f, -off);
            Append(out.floorMesh, a);
            Quad b(40.0f, 320.0f);
            b.UV(0.5f, 0.75f, 0.0f, 1.0f);
            FacingPlusX(b).Shift(frontX, 160.0f, depth * kCellSize + off);
            Append(out.floorMesh, b);
            Quad c(40.0f, 200.0f);
            c.UV(0.0f, 0.25f, 0.0f, 1.0f);
            FacingPlusX(c).Shift(frontX, -100.0f, -off);
            Append(out.undergroundMesh, c);
            Quad d(40.0f, 200.0f);
            d.UV(0.0f, 0.25f, 0.0f, 1.0f);
            FacingPlusX(d).Shift(frontX, -100.0f, depth * kCellSize + off);
            Append(out.undergroundMesh, d);
        }
    }

    // _CreateBackWalls: one 240-high wall per row along the far side (x = 0).
    void BackWall(float pz) {
        Quad q(40.0f, 240.0f);
        q.UV(0.5f, 0.75f, 0.0f, 1.0f);
        FacingPlusX(q).Shift(0.0f, 120.0f, pz);
        Append(out.floorMesh, q);
    }

    // _CreateUnderground: 200-deep cliff faces under the edge of a floor cell where a neighbour
    // (+X, +Z or -Z) has no floor. A hole cell gets only the face at the back wall (x = 0), so a
    // run of holes is one open pit.
    void Underground(int x, int z, float px, float pz) {
        if (Level(x, z) < 0) {
            if (x - 1 < 0) {
                Quad q(40.0f, 200.0f);
                q.UV(0.125f, 0.375f, 0.0f, 1.0f);
                FacingPlusX(q).Shift(px - 20.0f, -100.0f, pz);
                Append(out.undergroundMesh, q);
            }
            return;
        }
        if (Level(x + 1, z) < 0) {
            Quad q(40.0f, 200.0f);
            q.UV(0.125f, 0.375f, 0.0f, 1.0f);
            FacingPlusX(q).Shift(px + 20.0f, -100.0f, pz);
            Append(out.undergroundMesh, q);
        }
        if (Level(x, z + 1) < 0) {
            Quad q(40.0f, 200.0f);
            q.UV(0.625f, 0.875f, 0.0f, 1.0f);
            FacingPlusZ(q).Shift(px, -100.0f, pz + 20.0f);
            Append(out.undergroundMesh, q);
        }
        if (Level(x, z - 1) < 0) {
            Quad q(40.0f, 200.0f);
            q.UV(0.625f, 0.875f, 0.0f, 1.0f);
            FacingMinusZ(q).Shift(px, -100.0f, pz - 20.0f);
            Append(out.undergroundMesh, q);
        }
    }

    // _CreateWalls: the side strips of height step `step`, where the neighbour is no higher.
    void Walls(int step, int x, int z, float px, float pz) {
        float y = step * kLevelHeight + 12.5f;
        if (Level(x + 1, z) <= step) {
            Quad q(40.0f, 25.0f);
            q.UV(0.0f, 0.25f, 0.25f, 0.40625f);
            FacingPlusX(q).Shift(px + 20.0f, y, pz);
            Append(out.floorMesh, q);
        }
        if (Level(x, z + 1) <= step) {
            Quad q(40.0f, 25.0f);
            q.UV(0.25f, 0.5f, 0.25f, 0.40625f);
            FacingPlusZ(q).Shift(px, y, pz + 20.0f);
            Append(out.floorMesh, q);
        }
        if (Level(x, z - 1) <= step) {
            Quad q(40.0f, 25.0f);
            q.UV(0.25f, 0.5f, 0.25f, 0.40625f);
            FacingMinusZ(q).Shift(px, y, pz - 20.0f);
            Append(out.floorMesh, q);
        }
    }

    // _CreateFloor
    void Floor(int level, float px, float pz) {
        Quad q(40.0f, 40.0f);
        q.UV(0.0f, 0.25f, 0.0f, 0.25f);
        q.Shift(px, level * kLevelHeight, pz);
        Append(out.floorMesh, q);
    }

    // _CreateBridge: a deck at the given height and a 10-high lip on the camera side.
    void Bridge(int level, float px, float pz) {
        Quad deck(40.0f, 40.0f);
        deck.UV(0.0f, 0.25f, 0.75f, 1.0f);
        deck.Shift(px, level * kLevelHeight, pz);
        Append(out.floorMesh, deck);
        Quad lip(40.0f, 10.0f);
        lip.UV(0.25f, 0.5f, 0.75f, 0.8125f);
        FacingPlusX(lip).Shift(px + 20.0f, level * kLevelHeight - 5.0f, pz);
        Append(out.floorMesh, lip);
    }

    void Run() {
        SideWalls();
        for (int z = 0; z < depth; z++) {
            float pz = z * kCellSize + 20.0f;
            BackWall(pz);
            for (int x = 0; x < kWidth; x++) {
                float px = x * kCellSize + 20.0f;
                Underground(x, z, px, pz);
                char c = cells[z * kWidth + x];
                if (c < '0' || c > '9') continue;
                int level = Level(x, z);
                for (int step = 0; step < level; step++) Walls(step, x, z, px, pz);
                Floor(level, px, pz);
            }
        }
        // Bridges, in a second pass. Each takes the height of the cell before it in Z and is then
        // written into the field as that height (a run of bridges carries it along).
        for (int z = 0; z < depth; z++) {
            for (int x = 0; x < kWidth; x++) {
                float pz = z * kCellSize + 20.0f;
                float px = x * kCellSize + 20.0f;
                int before = Level(x, z - 1);
                char& c = cells[z * kWidth + x];
                if (c != 'H') continue;
                Bridge(before, px, pz);
                c = (char)('0' + before);
            }
        }
        out.cells = cells;
    }
};

}  // namespace

int FloorLevel(const std::vector<char>& cells, int depth, int x, int z) {
    if (x < 0 || x >= kWidth || z < 0 || z >= depth) return -1;
    char c = cells[z * kWidth + x];
    return (c >= '0' && c <= '9') ? c - '0' : -1;
}

Result Build(const LevelGrid& grid) {
    Builder b(grid);
    b.Run();
    return std::move(b.out);
}

}  // namespace MapGeometry
