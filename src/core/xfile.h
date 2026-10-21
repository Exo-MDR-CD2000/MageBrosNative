// Reader for the text DirectX .x models the game stores as RCDATA ("xof 0303txt 0032").
// The game's models use Mesh, MeshNormals, MeshMaterialList and Material; other templates
// (TextureFilename, MeshTextureCoords, Frame, ...) are read too where it costs nothing, so
// replacement models written by common exporters also load. Binary .x files are not supported.
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

struct XMaterial {
    std::string name;
    float diffuse[4] = {1, 1, 1, 1};  // RGBA
    float power = 0;
    float specular[3] = {0, 0, 0};
    float emissive[3] = {0, 0, 0};
    std::string texture;  // TextureFilename, if any
};

struct XMesh {
    std::string name;
    std::vector<float> positions;             // xyz per vertex
    std::vector<std::vector<uint32_t>> faces;  // vertex indices, any polygon size
    std::vector<float> normals;               // xyz per normal
    std::vector<std::vector<uint32_t>> normalFaces;  // per face, same shape as faces
    std::vector<float> texcoords;             // uv per vertex (optional)
    std::vector<uint32_t> faceMaterials;      // material index per face
    std::vector<XMaterial> materials;
};

struct XModel {
    std::vector<XMesh> meshes;
    size_t TriangleCount() const;
};

bool ParseXFile(const uint8_t* data, size_t size, XModel* out, std::string* error);
