// Benchmark mode: a process-local flag, the scripted tick count, and one measured run.
//
// A run samples every drawn frame, skips a warm-up, writes a JSON report beside the game's log
// and returns a one-line summary to the log. It changes no game state and writes no saves.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Benchmark {

bool Active();
void SetActive(bool on);
// For calibration: run the benchmark harness on an original level instead of the generated one.
void SetOriginalLevel(int world, int level);
bool HasOriginalLevel();
int OriginalWorld();
int OriginalLevel();
// Ticks run since benchmark mode started (the scripted input is scheduled on this count).
uint32_t Ticks();
void AddTick();

// Everything a report names about the run's conditions. The application fills it in.
struct Report {
    std::string version, platform, backend, vendor, renderer, glVersion;
    std::string displayMode, musicSource, level;
    int windowW = 0, windowH = 0, fboW = 0, fboH = 0, destW = 0, destH = 0;
    double refreshHz = 0;
    int vsync = 0, swapEvery = 1, samples = 0, maxFps = 0, brightness = 100, musicTrack = 0;
    int anisotropy = 1;  // the setting in effect, not the card's maximum
    bool smoothMotion = false, lowLatency = false;
    float renderScale = 1.f, maxAnisotropy = 1.f;
    bool ambientOcclusion = false, dynamicLights = false;
    int dynamicShadows = 0, bloom = 0;
};

// The default report path: <the ini/log folder>/reports/benchmark-<timestamp>.json
std::string DefaultReportPath();

class Run {
public:
    // Starts the run. `seconds` is the measurement window; the warm-up is added to it.
    void Begin(double seconds, const std::string& reportPath, std::function<void()> onFinish);
    bool Running() const { return running_; }
    bool Measuring() const { return measuring_; }
    // Seconds left in the measurement window (0 before the warm-up ends, negative when done).
    double RemainingSeconds() const;
    // One drawn frame: the gap since the frame before (0 for the first), the work before the
    // swap, the swap (including a low-latency GPU wait when >= 0), draws and uploaded bytes.
    void Frame(double frameMs, double cpuMs, double swapMs, double gpuWaitMs, double tickMs, double renderMs,
               int draws, size_t uploadBytes);
    // Once a second, after the game's own summary: the graphics card's time for the level's 3D
    // (-1 when the driver has no timer query).
    void Second(double gpu3DMs, int draws3D);
    // Ends the run, writes the report and calls the finish callback once.
    void Finish(const Report& report, const std::string& reportPath = std::string());
    // Used by the game loop when the measurement window is over.
    bool Done() const { return running_ && measuring_ && measuredMs_ >= targetMs_; }

    static constexpr double kWarmupSeconds = 2.0;

private:
    struct Sample {
        double ms = 0, cpu = 0, swap = 0, tick = 0, render = 0;
        int draws = 0;
        size_t bytes = 0;
    };
    bool running_ = false;
    bool measuring_ = false;
    bool gpuWaitSeen_ = false;
    double targetMs_ = 30000;
    double elapsedMs_ = 0;
    double measuredMs_ = 0;
    double gpuSum_ = 0;
    double gpuWaitSum_ = 0;
    int gpuFrames_ = 0;
    double draws3DSum_ = 0;
    int draws3DFrames_ = 0;
    std::vector<Sample> samples_;
    std::vector<double> warmupMs_;
    std::string reportPath_;
    std::function<void()> onFinish_;
};

}  // namespace Benchmark
