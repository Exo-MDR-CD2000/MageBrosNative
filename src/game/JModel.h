// The J library's models (JModel.cpp of the original): JModelList and JObject3DX.
//
// JModelList loads each .x model resource once and bakes the game's global model transform into
// it, set by XApplication at start-up: rotated -90 degrees about X (the models are authored
// Z-up), then scaled by 20. JObject3DX is an object made of one or more of those models as
// animation frames, with a scale, three rotations and a position.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

class CD3DFile;
class CollisionModel3D;
class Device;

namespace JModelList {
// The model for a resource, loaded on first use (nullptr if it can't be read).
CD3DFile* Get(uint16_t resId);
void Clear();
}  // namespace JModelList

class JObject3DX {
public:
    // The first model is frame 0.
    explicit JObject3DX(uint16_t firstModel);
    ~JObject3DX();

    void AppendFrame(uint16_t model);
    void SetCurrentFrame(int frame);
    int CurrentFrame() const { return current_; }
    int FrameCount() const { return (int)frames_.size(); }

    void SetPos(const D3DVECTOR& p);
    void SetScale(float s);
    void SetYaw(float a);    // about Y
    void SetPitch(float a);  // about X
    void SetRoll(float a);   // about Z
    const D3DVECTOR& GetPos() const { return pos_; }
    float GetYaw() const { return yaw_; }

    // world = scale * roll(Z) * pitch(X) * yaw(Y) * translation, as JObject3DX::UpdateWorldMatrix
    void UpdateWorldMatrix();
    const D3DMATRIX& WorldMatrix() const { return world_; }

    // Draw with the white material (the hero's damage flash), as JObject3DX::UseWhiteMaterial.
    void UseWhiteMaterial(bool w) { white_ = w; }
    bool UsingWhiteMaterial() const { return white_; }

    void Render(Device* d);

    // ColDet collision against the object's collision mesh (a separate model resource), placed
    // by the world matrix as of the last UpdateWorldMatrix. Without a collision model both tests
    // fail.
    void SetCollisionModel(uint16_t model);
    bool HasCollisionModel() const { return col_ != nullptr; }
    bool CheckCollision(const JObject3DX& other) const;                         // mesh against mesh
    bool CheckCollision_Sphere(const D3DVECTOR& centre, float radius) const;  // mesh against sphere

private:
    std::vector<CD3DFile*> frames_;
    int current_ = 0;
    D3DVECTOR pos_{0, 0, 0};
    float scale_ = 1.0f, yaw_ = 0, pitch_ = 0, roll_ = 0;
    D3DMATRIX world_;
    bool white_ = false;
    std::unique_ptr<CollisionModel3D> col_;
};
