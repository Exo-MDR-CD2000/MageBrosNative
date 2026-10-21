// Level geometry, built the way XMap::_ConstructFloorAndWall builds it (see
// docs/REVERSE_ENGINEERING.md, "Map geometry").
//
// The original makes every floor tile, wall strip, bridge, cliff face and end wall as a separate
// JObject3D_Rect: a flat quad that is UV-mapped, rotated and shifted into place by transforming
// its vertices, then merges them in creation order into two meshes, one drawn with the level's
// floor texture and one with the underground texture. This module replays the same sequence with
// the same matrices, so the merged meshes match the original vertex for vertex.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "core/level.h"
#include <cstdint>
#include <vector>

namespace MapGeometry {

constexpr float kCellSize = 40.0f;     // a map cell, in X and Z
constexpr float kLevelHeight = 25.0f;  // one height step

struct Mesh {
    std::vector<D3DLVERTEX> vertices;
    std::vector<WORD> indices;
    size_t QuadCount() const { return vertices.size() / 4; }
};

struct Result {
    Mesh floorMesh;        // floors, walls, bridges, end walls: the level's floor texture
    Mesh undergroundMesh;  // cliff faces below ground: the underground texture
    // The cell field after construction: every bridge ('H') becomes a floor at the height of the
    // cell before it in Z, as the original writes back for its collision checks.
    std::vector<char> cells;
};

// Builds both meshes for a level. Vertex colours are white; textures are applied by the caller.
Result Build(const LevelGrid& grid);

// XMap::GetFloorLevel on a cell field: -1 outside the map, the digit for '0'..'9', else -1.
int FloorLevel(const std::vector<char>& cells, int depth, int x, int z);

}  // namespace MapGeometry
