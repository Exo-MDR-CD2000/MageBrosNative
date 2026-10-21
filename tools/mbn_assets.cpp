// MageBrosAssets: inspects the user's own MageBros.exe.
//
//   MageBrosAssets [--game PATH] [--allow-unverified] --inventory
//   MageBrosAssets [--game PATH] [--allow-unverified] --selftest
//   MageBrosAssets [--game PATH] [--allow-unverified] --export FOLDER
//
// --selftest decodes every bitmap, sound, model and level with the port's loaders and reports
// any that fail. --export writes the original files out of your copy for reference and modding;
// they are Saito Games' work and must not be shared.
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "core/game_files.h"
#include "core/image.h"
#include "core/level.h"
#include "core/log.h"
#include "core/wave.h"
#include "core/xfile.h"

namespace {

void MakeDir(const std::string& p) {
#ifdef _WIN32
    _mkdir(p.c_str());
#else
    mkdir(p.c_str(), 0755);
#endif
}

bool WriteFile(const std::string& path, const uint8_t* d, size_t n) {
    FILE* f = OpenFileUtf8(path.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(d, 1, n, f) == n;
    std::fclose(f);
    return ok;
}

// Name of a level resource as "w<world>-<level>" (1-based), or empty.
std::string LevelName(uint32_t id) {
    for (int w = 0; w < GameFiles::kWorldCount; w++)
        for (int l = 0; l < GameFiles::kLevelsPerWorld; l++)
            if (GameFiles::LevelResourceId(w, l) == id) {
                char b[16];
                std::snprintf(b, sizeof b, "w%d-%02d", w + 1, l + 1);
                return b;
            }
    return std::string();
}

// The first mesh name of a model, which is how the original names its parts.
std::string ModelName(const std::vector<uint8_t>& d) {
    XModel m;
    std::string err;
    if (!ParseXFile(d.data(), d.size(), &m, &err) || m.meshes.empty()) return std::string();
    return m.meshes[0].name;
}

int Inventory() {
    std::map<std::string, std::pair<int, size_t>> totals;
    for (const auto& e : GameFiles::ListGameResources()) {
        auto& t = totals[GameFiles::KindName(e.kind)];
        t.first++;
        t.second += e.data->size();
    }
    std::printf("%s\n%s\n\n", GameFiles::ExePath().c_str(), GameFiles::BuildLabel().c_str());
    for (const auto& kv : totals)
        std::printf("  %-8s %4d  %8.1f KB\n", kv.first.c_str(), kv.second.first, kv.second.second / 1024.0);
    std::printf("\nMusic:");
    for (int i = 1; i <= GameFiles::kMusicTrackCount; i++)
        std::printf(" %d:%s", i, GameFiles::MusicPath(i).empty() ? "missing" : "ok");
    std::printf("\n");
    return 0;
}

int SelfTest() {
    int ok = 0, bad = 0;
    std::map<std::string, int> perKind;
    size_t triangles = 0, levelRows = 0;
    for (const auto& e : GameFiles::ListGameResources()) {
        const auto& d = *e.data;
        std::string err;
        bool good = true;
        switch (e.kind) {
            case GameFiles::Kind::Bitmap: {
                Image img;
                good = DecodeDIB(d.data(), d.size(), &img);
                if (!good) err = "DIB decode failed";
                break;
            }
            case GameFiles::Kind::Wave: {
                WaveData w;
                good = DecodeWave(d.data(), d.size(), &w) && w.FrameCount() > 0;
                if (!good) err = "WAVE decode failed";
                break;
            }
            case GameFiles::Kind::Model: {
                XModel m;
                good = ParseXFile(d.data(), d.size(), &m, &err) && !m.meshes.empty();
                if (good) triangles += m.TriangleCount();
                break;
            }
            case GameFiles::Kind::Level: {
                LevelGrid g;
                good = ParseLevel(d.data(), d.size(), &g, &err);
                if (good) levelRows += g.depth;
                break;
            }
            default: continue;
        }
        if (good) {
            ok++;
            perKind[GameFiles::KindName(e.kind)]++;
        } else {
            bad++;
            std::printf("FAIL %s %u: %s\n", GameFiles::KindName(e.kind), e.id, err.c_str());
        }
    }
    // Every world/level slot must point at a level that parses.
    for (int w = 0; w < GameFiles::kWorldCount; w++)
        for (int l = 0; l < GameFiles::kLevelsPerWorld; l++) {
            const auto* d = GameFiles::RCData(GameFiles::LevelResourceId(w, l));
            LevelGrid g;
            std::string err;
            if (!d || !ParseLevel(d->data(), d->size(), &g, &err)) {
                bad++;
                std::printf("FAIL world %d level %d: missing or unreadable\n", w + 1, l + 1);
            }
        }
    for (const auto& kv : perKind) std::printf("  %-8s %4d decoded\n", kv.first.c_str(), kv.second);
    std::printf("  models: %zu triangles in total; levels: %zu rows in total\n", triangles, levelRows);
    std::printf("%s: %d ok, %d failed\n", bad ? "FAILED" : "PASSED", ok, bad);
    return bad ? 1 : 0;
}

int Export(const std::string& dir) {
    MakeDir(dir);
    for (const char* sub : {"/bitmaps", "/sounds", "/models", "/levels"}) MakeDir(dir + sub);
    int n = 0;
    for (const auto& e : GameFiles::ListGameResources()) {
        const auto& d = *e.data;
        char id[16];
        std::snprintf(id, sizeof id, "%u", e.id);
        std::string path;
        std::vector<uint8_t> bytes;
        switch (e.kind) {
            case GameFiles::Kind::Bitmap:
                path = dir + "/bitmaps/" + id + ".bmp";
                bytes = DIBToBmpFile(d.data(), d.size());
                break;
            case GameFiles::Kind::Wave:
                path = dir + "/sounds/" + id + ".wav";
                bytes = d;
                break;
            case GameFiles::Kind::Model: {
                std::string name = ModelName(d);
                path = dir + "/models/" + id + (name.empty() ? "" : "_" + name) + ".x";
                bytes = d;
                break;
            }
            case GameFiles::Kind::Level: {
                std::string name = LevelName(e.id);
                path = dir + "/levels/" + (name.empty() ? std::string(id) : name + "_" + id) + ".txt";
                bytes = d;
                break;
            }
            default: continue;
        }
        if (bytes.empty() || !WriteFile(path, bytes.data(), bytes.size())) {
            std::printf("could not write %s\n", path.c_str());
            continue;
        }
        n++;
    }
    std::printf("Exported %d files to %s\n"
                "These files come from your copy of Mage Bros. and are (c) Saito Games: do not share them.\n",
                n, dir.c_str());
    return 0;
}

std::string ProgramDir(const char* argv0) {
    std::string p = argv0 ? argv0 : "";
    size_t s = p.find_last_of("/\\");
    return s == std::string::npos ? std::string() : p.substr(0, s + 1);
}

}  // namespace

int main(int argc, char** argv) {
    LogSetEcho(false);
    std::string game, exportDir, mode;
    bool allowUnverified = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--game" && i + 1 < argc) game = argv[++i];
        else if (a == "--allow-unverified") allowUnverified = true;
        else if (a == "--inventory" || a == "--selftest") mode = a;
        else if (a == "--export" && i + 1 < argc) { mode = a; exportDir = argv[++i]; }
        else {
            std::fprintf(stderr,
                         "usage: MageBrosAssets [--game PATH] [--allow-unverified] "
                         "(--inventory | --selftest | --export FOLDER)\n");
            return 2;
        }
    }
    if (mode.empty()) mode = "--inventory";
    std::string err;
    if (!GameFiles::Open(game, ProgramDir(argv[0]), allowUnverified, &err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    if (mode == "--selftest") return SelfTest();
    if (mode == "--export") return Export(exportDir);
    return Inventory();
}
