// XWater (XWater.cpp of the original): the water surface of a water level, two textured sheets
// over the whole map at y -39 and -38 whose textures drift at different speeds.
//
// A level has water when any row's flag (the first character of its line) is 'W'
// (XMap::_ConstructWallObjects, XMap::UseWater). On a water level a hero falling below -58, a
// shot below -38, a rolling cannonball below -38 and a dropping platform reaching -38 meet the
// water (waves, XParticle_WaterWave), and the fish jump out of it.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <memory>

class Device;
class JObject3DRect;

class XWater {
public:
    XWater();
    ~XWater();
    // XWater::Init: the sheets over a map of `cellsX` x `cellsZ` cells (world 3 has its own textures)
    void Init(int cellsX, int cellsZ, int world);
    void Deinit() { initialised_ = false; }
    void ProcessFrame();
    void Render(Device* d);

private:
    int cellsX_ = 0, cellsZ_ = 0;
    unsigned localCounter_ = 0;
    bool initialised_ = false;
    float waterU1_ = 0, waterV1_ = 0, waterU2_ = 0, waterV2_ = 0;
    // native (smooth motion): the drift before tick prevTick_, to draw between
    float prevU1_ = 0, prevV1_ = 0, prevU2_ = 0, prevV2_ = 0;
    unsigned prevTick_ = ~0u;
    std::unique_ptr<JObject3DRect> water1_, water2_;
};
