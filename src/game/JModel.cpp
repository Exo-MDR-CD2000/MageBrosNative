#include "JModel.h"
#include "coldet.h"
#include "core/log.h"
#include "framework/d3dfile.h"
#include "framework/d3dutil.h"
#include "framework/interp.h"
#include "gfx/device.h"
#include <cmath>
#include <set>

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

JObject3DX::JObject3DX(uint16_t firstModel) : firstModel_(firstModel) {
    frames_.push_back(JModelList::Get(firstModel));
    UpdateWorldMatrix();
}

JObject3DX::~JObject3DX() = default;

void JObject3DX::AppendFrame(uint16_t model) { frames_.push_back(JModelList::Get(model)); }

void JObject3DX::SetCurrentFrame(int frame) {
    if (frame >= 0 && frame < (int)frames_.size()) current_ = frame;
}

void JObject3DX::SetPos(const D3DVECTOR& p) {
    TouchPose();
    pos_ = p;
}
void JObject3DX::SetScale(float s) {
    TouchPose();
    scale_ = s;
}
void JObject3DX::SetYaw(float a) {
    TouchPose();
    yaw_ = a;
}
void JObject3DX::SetPitch(float a) {
    TouchPose();
    pitch_ = a;
}
void JObject3DX::SetRoll(float a) {
    TouchPose();
    roll_ = a;
}

namespace {
// world = scale * roll(Z) * pitch(X) * yaw(Y) * translation, as JObject3DX::UpdateWorldMatrix
void BuildWorld(D3DMATRIX& world, const D3DVECTOR& pos, float scale, float yaw, float pitch, float roll) {
    D3DMATRIX s, rz, rx, ry, t;
    D3DUtil_SetScaleMatrix(s, scale, scale, scale);
    D3DUtil_SetRotateZMatrix(rz, roll);
    D3DUtil_SetRotateXMatrix(rx, pitch);
    D3DUtil_SetRotateYMatrix(ry, yaw);
    D3DUtil_SetTranslateMatrix(t, pos.x, pos.y, pos.z);
    D3DUtil_SetIdentityMatrix(world);
    for (const D3DMATRIX* m : {&s, &rz, &rx, &ry, &t}) {
        D3DMATRIX r;
        D3DMath_MatrixMultiply(r, world, *m);
        world = r;
    }
}

// further than this in one tick is a jump, drawn where it lands (a cell; the mage moves at most
// 4 a tick)
constexpr float kMaxBlendStep = 40.0f;
}  // namespace

void JObject3DX::UpdateWorldMatrix() {
    BuildWorld(world_, pos_, scale_, yaw_, pitch_, roll_);
    if (col_) col_->setTransform(reinterpret_cast<float*>(&world_));
}

void JObject3DX::TouchPose() {
    if (!blend_ || poseTick_ == Interp::CurrentTick()) return;
    poseTick_ = Interp::CurrentTick();
    // not drawn in the last frame (just made, or hidden until now): its earlier pose is from
    // before it showed, so this tick is drawn as it is
    if (lastDrawFrame_ != Interp::FramesDrawn()) snapTick_ = poseTick_;
    prevPos_ = pos_;
    prevScale_ = scale_;
    prevYaw_ = yaw_;
    prevPitch_ = pitch_;
    prevRoll_ = roll_;
}

void JObject3DX::Snap() {
    poseTick_ = Interp::CurrentTick();
    prevPos_ = pos_;
    prevScale_ = scale_;
    prevYaw_ = yaw_;
    prevPitch_ = pitch_;
    prevRoll_ = roll_;
}

bool JObject3DX::Blending() const {
    if (!blend_ || poseTick_ != Interp::CurrentTick() || snapTick_ == poseTick_ || Interp::Alpha() >= 1.f)
        return false;
    return std::fabs(pos_.x - prevPos_.x) <= kMaxBlendStep && std::fabs(pos_.y - prevPos_.y) <= kMaxBlendStep &&
           std::fabs(pos_.z - prevPos_.z) <= kMaxBlendStep;
}

D3DVECTOR JObject3DX::BlendedPos() const {
    return Blending() ? Interp::Lerp(prevPos_, pos_, Interp::Alpha()) : pos_;
}

D3DVECTOR JObject3DX::BlendOffset() const {
    if (!Blending()) return D3DVECTOR(0, 0, 0);
    D3DVECTOR b = Interp::Lerp(prevPos_, pos_, Interp::Alpha());
    return D3DVECTOR(b.x - pos_.x, b.y - pos_.y, b.z - pos_.z);
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
    if (Interp::InScope()) {
        // native (smooth motion): one model drawn twice in a frame would have one history for two
        // places; logged once per model
        if (lastDrawFrame_ == Interp::FramesDrawn()) {
            static std::set<uint16_t> logged;
            if (logged.insert(firstModel_).second)
                Log("Smooth motion: model %#x drawn twice in one frame (its blend may be wrong)", firstModel_);
        }
        lastDrawFrame_ = Interp::FramesDrawn();
    }
    if (Blending()) {
        float a = Interp::Alpha();
        D3DMATRIX world;
        BuildWorld(world, Interp::Lerp(prevPos_, pos_, a), Interp::Lerp(prevScale_, scale_, a),
                   Interp::LerpAngle(prevYaw_, yaw_, a), Interp::LerpAngle(prevPitch_, pitch_, a),
                   Interp::LerpAngle(prevRoll_, roll_, a));
        d->SetTransform(D3DTRANSFORMSTATE_WORLD, &world);
    } else {
        d->SetTransform(D3DTRANSFORMSTATE_WORLD, &world_);
    }
    // models are shared between objects, so the flag is set for this draw only
    mesh->UseWhiteMaterial(white_);
    mesh->Render(d);
    mesh->UseWhiteMaterial(false);
}
