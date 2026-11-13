#include "benchmark.h"
#include "benchmark_stats.h"
#include "core/config.h"
#include "core/log.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace Benchmark {

namespace {
bool g_active = false;
uint32_t g_ticks = 0;
int g_originalWorld = -1, g_originalLevel = -1;

std::string FolderOf(const std::string& path) {
    size_t p = path.find_last_of("/\\");
    return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

void EnsureReportsFolder(const std::string& file) {
    std::string dir = FolderOf(file);
    if (!dir.empty()) SDL_CreateDirectory(dir.c_str());
}

// The first free name, so two runs in the same second do not overwrite each other.
std::string FreePath(const std::string& base) {
    std::string path = base;
    for (int n = 2; n < 100; n++) {
        FILE* f = OpenFileUtf8(path.c_str(), "rb");
        if (!f) return path;
        std::fclose(f);
        std::string stem = base;
        size_t dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);
        char b[32];
        std::snprintf(b, sizeof b, "-%d", n);
        path = stem + b + ".json";
    }
    return base;
}

}  // namespace

bool Active() { return g_active; }

void SetActive(bool on) {
    g_active = on;
    if (on) g_ticks = 0;
}

void SetOriginalLevel(int world, int level) {
    g_originalWorld = world;
    g_originalLevel = level;
}

bool HasOriginalLevel() { return g_originalWorld >= 0 && g_originalLevel >= 0; }

int OriginalWorld() { return g_originalWorld; }

int OriginalLevel() { return g_originalLevel; }

uint32_t Ticks() { return g_ticks; }

void AddTick() {
    if (g_ticks != UINT32_MAX) g_ticks++;
}

std::string DefaultReportPath() {
    std::string folder = FolderOf(GetConfig().path);
    if (folder.empty()) folder = ".";
    if (folder.back() != '/' && folder.back() != '\\') folder += '/';
    folder += "reports";
    SDL_CreateDirectory(folder.c_str());
    std::time_t now = std::time(nullptr);
    std::tm t{};
#if defined(_WIN32)
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    char stamp[32];
    std::strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", &t);
    return FreePath(folder + "/benchmark-" + stamp + ".json");
}

void Run::Begin(double seconds, const std::string& reportPath, std::function<void()> onFinish) {
    running_ = true;
    measuring_ = false;
    gpuWaitSeen_ = false;
    targetMs_ = std::max(1.0, seconds) * 1000.0;
    elapsedMs_ = measuredMs_ = 0;
    gpuSum_ = gpuWaitSum_ = 0;
    gpuFrames_ = 0;
    draws3DSum_ = 0;
    draws3DFrames_ = 0;
    samples_.clear();
    warmupMs_.clear();
    reportPath_ = reportPath;
    onFinish_ = std::move(onFinish);
    SetActive(true);
    Log("Benchmark: %.1f s measurement after a %.1f s warm-up", targetMs_ / 1000.0, kWarmupSeconds);
}

double Run::RemainingSeconds() const {
    if (!running_) return 0;
    if (!measuring_) return targetMs_ / 1000.0;
    return std::max(0.0, (targetMs_ - measuredMs_) / 1000.0);
}

void Run::Frame(double frameMs, double cpuMs, double swapMs, double gpuWaitMs, double tickMs, double renderMs,
                int draws, size_t uploadBytes) {
    if (!running_ || frameMs <= 0) return;
    elapsedMs_ += frameMs;
    if (!measuring_) {
        warmupMs_.push_back(frameMs);
        if (elapsedMs_ >= kWarmupSeconds * 1000.0) measuring_ = true;
        return;
    }
    Sample s;
    s.ms = frameMs;
    s.cpu = cpuMs;
    s.swap = swapMs;
    s.tick = tickMs;
    s.render = renderMs;
    s.draws = draws;
    s.bytes = uploadBytes;
    if (gpuWaitMs >= 0) {
        gpuWaitSeen_ = true;
        gpuWaitSum_ += gpuWaitMs;
    }
    samples_.push_back(s);
    measuredMs_ += frameMs;
}

void Run::Second(double gpu3DMs, int draws3D) {
    if (!running_ || !measuring_) return;
    if (gpu3DMs >= 0) {
        gpuSum_ += gpu3DMs;
        gpuFrames_++;
    }
    if (draws3D > 0) {
        draws3DSum_ += draws3D;
        draws3DFrames_++;
    }
}

void Run::Finish(const Report& r, const std::string& reportPath) {
    if (!running_) return;
    running_ = false;
    measuring_ = false;
    SetActive(false);

    if (reportPath_ .empty()) reportPath_ = reportPath;
    if (reportPath_.empty()) reportPath_ = DefaultReportPath();
    EnsureReportsFolder(reportPath_);
    reportPath_ = FreePath(reportPath_);

    std::vector<double> ms;
    ms.reserve(samples_.size());
    double cpuSum = 0, swapSum = 0, drawSum = 0, byteSum = 0, tickSum = 0, renderSum = 0;
    for (const Sample& s : samples_) {
        ms.push_back(s.ms);
        cpuSum += s.cpu;
        swapSum += s.swap;
        tickSum += s.tick;
        renderSum += s.render;
        drawSum += s.draws;
        byteSum += (double)s.bytes;
    }
        const size_t n = samples_.size();
    const double sumMs = measuredMs_;
    const double avgFps = sumMs > 0 ? 1000.0 * (double)n / sumMs : 0;
    const double p95 = PercentileMs(ms, 0.95);
    const double p99 = PercentileMs(ms, 0.99);
    const double maxMs = ms.empty() ? 0 : *std::max_element(ms.begin(), ms.end());
    const double minMs = ms.empty() ? 0 : *std::min_element(ms.begin(), ms.end());
    const double fps1 = FpsFromMs(p99);
    const double cpu = n ? cpuSum / (double)n : 0;
    const double swap = n ? swapSum / (double)n : 0;
    const double tick = n ? tickSum / (double)n : 0;
    const double render = n ? renderSum / (double)n : 0;
    const double gpuWait = (n && gpuWaitSeen_) ? gpuWaitSum_ / (double)n : -1;
    const double gpu3D = gpuFrames_ ? gpuSum_ / gpuFrames_ : -1;
    const double draws = n ? drawSum / (double)n : 0;
    const double draws3D = draws3DFrames_ ? draws3DSum_ / draws3DFrames_ : 0;
    const double bytes = n ? byteSum / (double)n : 0;

    std::string out;
    out.reserve(4096 + ms.size() * 12);
    char b[512];
    auto kv = [&](const char* key, const std::string& val, bool comma = true) {
        out += "  \"";
        out += key;
        out += "\": ";
        out += val;
        if (comma) out += ',';
        out += '\n';
    };
    auto num = [&](double v, int prec = 3) {
        std::snprintf(b, sizeof b, "%.*f", prec, v);
        return std::string(b);
    };
    out += "{\n";
    kv("version", "\"" + JsonEscape(r.version) + "\"");
    kv("platform", "\"" + JsonEscape(r.platform) + "\"");
    kv("backend", "\"" + JsonEscape(r.backend) + "\"");
    kv("gl_vendor", "\"" + JsonEscape(r.vendor) + "\"");
    kv("gl_renderer", "\"" + JsonEscape(r.renderer) + "\"");
    kv("gl_version", "\"" + JsonEscape(r.glVersion) + "\"");
    kv("display_mode", "\"" + JsonEscape(r.displayMode) + "\"");
    kv("level", "\"" + JsonEscape(r.level) + "\"");
    kv("refresh_hz", num(r.refreshHz, 2));
    kv("window_w", std::to_string(r.windowW));
    kv("window_h", std::to_string(r.windowH));
    kv("render_target_w", std::to_string(r.fboW));
    kv("render_target_h", std::to_string(r.fboH));
    kv("picture_w", std::to_string(r.destW));
    kv("picture_h", std::to_string(r.destH));
    kv("vsync", std::to_string(r.vsync));
    kv("swap_every", std::to_string(r.swapEvery));
    kv("smooth_motion", r.smoothMotion ? "true" : "false");
    kv("low_latency", r.lowLatency ? "true" : "false");
    kv("msaa", std::to_string(r.samples));
    kv("anisotropy", std::to_string(r.anisotropy));
    kv("max_anisotropy", num(r.maxAnisotropy, 1));
    kv("render_scale", num(r.renderScale, 3));
    kv("max_fps", std::to_string(r.maxFps));
    kv("brightness", std::to_string(r.brightness));
    kv("ambient_occlusion", r.ambientOcclusion ? "true" : "false");
    kv("dynamic_shadows", std::to_string(r.dynamicShadows));
    kv("dynamic_lights", r.dynamicLights ? "true" : "false");
    kv("bloom", std::to_string(r.bloom));
    kv("music_source", "\"" + JsonEscape(r.musicSource) + "\"");
    kv("music_track", std::to_string(r.musicTrack));
    kv("warmup_s", num(kWarmupSeconds, 1));
    kv("duration_s", num(targetMs_ / 1000.0, 1));
    kv("measured_s", num(measuredMs_ / 1000.0, 3));
    kv("frames", std::to_string(n));
    out += "  \"summary\": {\n";
    auto skv = [&](const char* key, const std::string& val, bool comma = true) {
        out += "    \"";
        out += key;
        out += "\": ";
        out += val;
        if (comma) out += ',';
        out += '\n';
    };
    skv("fps_avg", num(avgFps, 2));
    skv("fps_1pct_low", num(fps1, 2));
    skv("frame_ms_avg", num(n ? sumMs / (double)n : 0, 3));
    skv("frame_ms_min", num(minMs, 3));
    skv("frame_ms_p95", num(p95, 3));
    skv("frame_ms_p99", num(p99, 3));
    skv("frame_ms_max", num(maxMs, 3));
    skv("cpu_ms_avg", num(cpu, 3));
    skv("tick_ms_avg", num(tick, 3));
    skv("render_ms_avg", num(render, 3));
    skv("swap_ms_avg", num(swap, 3));
    skv("gpu_wait_ms_avg", gpuWait >= 0 ? num(gpuWait, 3) : std::string("null"));
    skv("gpu3d_ms_avg", gpu3D >= 0 ? num(gpu3D, 3) : std::string("null"));
    skv("draws_avg", num(draws, 1));
    skv("draws3d_avg", num(draws3D, 1));
    skv("upload_bytes_avg", num(bytes, 0), false);
    out += "  },\n";
    out += "  \"frame_ms\": [";
    for (size_t i = 0; i < ms.size(); i++) {
        std::snprintf(b, sizeof b, "%s%.3f", i ? "," : "", ms[i]);
        out += b;
    }
    out += "],\n";
    out += "  \"warmup_frame_ms\": [";
    for (size_t i = 0; i < warmupMs_.size(); i++) {
        std::snprintf(b, sizeof b, "%s%.3f", i ? "," : "", warmupMs_[i]);
        out += b;
    }
    out += "]\n";
    out += "}\n";

    FILE* f = OpenFileUtf8(reportPath_.c_str(), "wb");
    bool wrote = false;
    if (f) {
        wrote = std::fwrite(out.data(), 1, out.size(), f) == out.size();
        std::fclose(f);
    }
    if (wrote)
        Log("Benchmark: %.1f fps average, %.1f fps 1%% low, p95 %.2f ms, p99 %.2f ms, max %.2f ms, "
            "%zu frames; CPU %.2f ms (ticks %.2f, render %.2f), swap %.2f ms, %.0f draws, %.0f bytes a frame; "
            "report: %s",
            avgFps, fps1, p95, p99, maxMs, n, cpu, tick, render, swap, draws, bytes, reportPath_.c_str());
    else
        Log("Benchmark: could not write the report %s", reportPath_.c_str());
    if (onFinish_) {
        auto cb = std::move(onFinish_);
        onFinish_ = nullptr;
        cb();
    }
}

}  // namespace Benchmark
