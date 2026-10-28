// Stand-in for the J library's models in tests that run the game logic without a renderer:
// objects keep their frame and transform but load and draw nothing.
#include "game/JModel.h"
#include "../third_party/coldet/src/coldet.h"  // (the collision model type, for the destructor)
namespace JModelList {
CD3DFile* Get(uint16_t) { return nullptr; }
void Clear() {}
}  // namespace JModelList
JObject3DX::JObject3DX(uint16_t) { frames_.push_back(nullptr); }
void JObject3DX::AppendFrame(uint16_t) { frames_.push_back(nullptr); }
void JObject3DX::SetCurrentFrame(int f) { if (f >= 0 && f < (int)frames_.size()) current_ = f; }
void JObject3DX::SetPos(const D3DVECTOR& p) { pos_ = p; }
void JObject3DX::SetScale(float s) { scale_ = s; }
void JObject3DX::SetYaw(float a) { yaw_ = a; }
void JObject3DX::SetPitch(float a) { pitch_ = a; }
void JObject3DX::SetRoll(float a) { roll_ = a; }
void JObject3DX::UpdateWorldMatrix() {}
void JObject3DX::Render(Device*) {}
void JObject3DX::Snap() {}
D3DVECTOR JObject3DX::BlendedPos() const { return pos_; }
JObject3DX::~JObject3DX() = default;
void JObject3DX::SetCollisionModel(uint16_t) {}
bool JObject3DX::CheckCollision(const JObject3DX&) const { return false; }
bool JObject3DX::CheckCollision_Sphere(const D3DVECTOR&, float) const { return false; }
