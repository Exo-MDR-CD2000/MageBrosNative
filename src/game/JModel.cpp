#include "JModel.h"
#include "coldet.h"
#include "core/log.h"
#include "framework/d3dfile.h"
#include "framework/d3dutil.h"
#include "gfx/device.h"
#include <cmath>

namespace {
// XApplication::OneTimeSceneInit: JModelList::SetInitRotate((-pi/2, 0, 0)), SetInitScale(20)
constexpr float kInitRotateX = -1.5707964f;
constexpr float kInitScale = 20.0f;

std::map<uint16_t, std::unique_ptr<CD3DFile>>& Cache() {
    static std::map<uint16_t, std::unique_ptr<CD3DFile>> cache;
    return cache;
}
}  // namespace

namespace JModelList {

CD3DFile* Get(uint16_t resId) {
    auto& cache = Cache();
    auto it = cache.find(resId);
    if (it != cache.end()) return it->second.get();
    // JModelList::CreateModel: load, then rotate and scale the mesh itself, once
    auto m = std::make_unique<CD3DFile>();
    if (FAILED(m->LoadFromResource(resId))) {
        Log("JModelList: model %u could not be loaded", resId);
        m.reset();
    } else {
        m->RotateX(kInitRotateX);
        m->Scale(kInitScale);
    }
    CD3DFile* p = m.get();
    cache[resId] = std::move(m);
    return p;
}

void Clear() { Cache().clear(); }

}  // namespace JModelList

JObject3DX::JObject3DX(uint16_t firstModel) {
    frames_.push_back(JModelList::Get(firstModel));
    UpdateWorldMatrix();
}

JObject3DX::~JObject3DX() = default;

void JObject3DX::AppendFrame(uint16_t model) { frames_.push_back(JModelList::Get(model)); }

void JObject3DX::SetCurrentFrame(int frame) {
    if (frame >= 0 && frame < (int)frames_.size()) current_ = frame;
}

void JObject3DX::SetPos(const D3DVECTOR& p) { pos_ = p; }
void JObject3DX::SetScale(float s) { scale_ = s; }
void JObject3DX::SetYaw(float a) { yaw_ = a; }
void JObject3DX::SetPitch(float a) { pitch_ = a; }
void JObject3DX::SetRoll(float a) { roll_ = a; }

void JObject3DX::UpdateWorldMatrix() {
    D3DMATRIX s, rz, rx, ry, t;
    D3DUtil_SetScaleMatrix(s, scale_, scale_, scale_);
    D3DUtil_SetRotateZMatrix(rz, roll_);
    D3DUtil_SetRotateXMatrix(rx, pitch_);
    D3DUtil_SetRotateYMatrix(ry, yaw_);
    D3DUtil_SetTranslateMatrix(t, pos_.x, pos_.y, pos_.z);
    D3DUtil_SetIdentityMatrix(world_);
    for (const D3DMATRIX* m : {&s, &rz, &rx, &ry, &t}) {
        D3DMATRIX r;
        D3DMath_MatrixMultiply(r, world_, *m);
        world_ = r;
    }
    if (col_) col_->setTransform(reinterpret_cast<float*>(&world_));
}

namespace {
// the triangles of a model, as loaded by JModelList (rotated and scaled), kept per resource
const std::vector<float>& Triangles(uint16_t resId) {
    static std::map<uint16_t, std::vector<float>> cache;
    auto it = cache.find(resId);
    if (it != cache.end()) return it->second;
    std::vector<float> t;
    if (CD3DFile* m = JModelList::Get(resId)) {
        m->ForEachMesh([&](CD3DFileObject& o) {
            for (size_t i = 0; i + 2 < o.indices.size(); i += 3)
                for (int k = 0; k < 3; k++) {
                    const D3DVERTEX& v = o.vertices[o.indices[i + k]];
                    t.push_back(v.x);
                    t.push_back(v.y);
                    t.push_back(v.z);
                }
        });
    }
    return cache.emplace(resId, std::move(t)).first->second;
}
}  // namespace

void JObject3DX::SetCollisionModel(uint16_t model) {
    const std::vector<float>& t = Triangles(model);
    if (t.empty()) {
        col_.reset();
        return;
    }
    col_.reset(newCollisionModel3D());
    col_->setTriangleNumber((int)(t.size() / 9));
    for (size_t i = 0; i + 8 < t.size(); i += 9)
        col_->addTriangle(t[i], t[i + 1], t[i + 2], t[i + 3], t[i + 4], t[i + 5], t[i + 6], t[i + 7], t[i + 8]);
    col_->finalize();
    col_->setTransform(reinterpret_cast<float*>(&world_));
}

bool JObject3DX::CheckCollision(const JObject3DX& other) const {
    if (!col_ || !other.col_) return false;
    return col_->collision(other.col_.get());
}

bool JObject3DX::CheckCollision_Sphere(const D3DVECTOR& centre, float radius) const {
    if (!col_) return false;
    float c[3] = {centre.x, centre.y, centre.z};
    return col_->sphereCollision(c, radius);
}

void JObject3DX::Render(Device* d) {
    CD3DFile* mesh = frames_.empty() ? nullptr : frames_[current_];
    if (!mesh) return;
    d->SetTransform(D3DTRANSFORMSTATE_WORLD, &world_);
    // models are shared between objects, so the flag is set for this draw only
    mesh->UseWhiteMaterial(white_);
    mesh->Render(d);
    mesh->UseWhiteMaterial(false);
}
