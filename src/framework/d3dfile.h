// Replacement for the DX7 SDK CD3DFile/CD3DFileObject (d3dfile.cpp) with a
// built-in parser for the text .X meshes stored in MageBros.exe (RT_RCDATA).
#pragma once
#include "../core/d3dtypes.h"
#include <string>
#include <vector>
#include <memory>

class Device;
class Texture;
class MeshBuffer;

struct MeshMaterialData {
    D3DMATERIAL7 mtrl{};
    std::string texture;
    DWORD numIndices = 0;
    std::shared_ptr<Texture> tex;  // native (mods): a glTF model's own texture, drawn with the material
};

class CD3DFileObject {
public:
    explicit CD3DFileObject(const std::string& name);
    ~CD3DFileObject();
    // The vertices changed (Scale/Shift/Rotate): upload them again on the next draw.
    void InvalidateGpu();
    // white: draw every material with the white material (the damage flash), as the game's
    // CD3DFileObject::Render(dev, alpha, white)
    void Render(Device* dev, bool alpha, bool white = false);
    std::string name;
    D3DMATRIX mat;
    bool hasMeshData = false;
    std::vector<D3DVERTEX> vertices;
    std::vector<WORD> indices;
    MeshMaterialData materials[16];
    DWORD numMaterials = 0;
    bool hasAlpha = false;
    std::vector<std::unique_ptr<CD3DFileObject>> children;  // child list (m_pChild + m_pNext chain)
    std::unique_ptr<MeshBuffer> gpu_;        // per-material path: uploaded once, kept on the card
    std::unique_ptr<MeshBuffer> mergedGpu_;  // opaque, untextured: all parts in one draw
    std::vector<D3DVERTEX> mergedVerts_;
    std::vector<WORD> mergedIndices_;
    std::vector<unsigned char> mergedMat_;
    D3DMATERIAL7 mergedMaterials_[16] = {};
    bool mergedBuilt_ = false;
    bool CanMerge() const;
    void BuildMerged();
};

class CD3DFile {
public:
    CD3DFile();
    ~CD3DFile();
    HRESULT LoadFromResource(WORD resId);
    // Native (mods): forget every loaded model so the next load picks up mod changes.
    static void ClearCache();
    // Native (asset export): the original model as a glTF binary (.glb) file.
    static bool ExportOriginalGlb(WORD resId, std::vector<uint8_t>* out);
    HRESULT Render(Device* dev);
    // The game's CD3DFile::UseWhiteMaterial: render with a white, slightly emissive material.
    void UseWhiteMaterial(bool w) { white_ = w; }
    void Scale(float s);
    void Shift(const D3DVECTOR& v);
    void RotateX(float a);
    void RotateY(float a);
    void RotateZ(float a);
    void GetBoxMin(D3DVECTOR& out);
    void GetBoxMax(D3DVECTOR& out);
    template <class F> void ForEachMesh(F f) {
        if (root_)
            for (auto& c : root_->children) Visit(c.get(), f);
    }

private:
    template <class F> void Visit(CD3DFileObject* o, F& f) {
        if (o->hasMeshData) f(*o);
        for (auto& c : o->children) Visit(c.get(), f);
    }
    std::unique_ptr<CD3DFileObject> root_;
    bool white_ = false;
};
