// Tests for the format readers on small, hand-written inputs (no game files needed).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "core/image.h"
#include "core/level.h"
#include "core/wave.h"
#include "core/xfile.h"

static int g_fail = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            g_fail++;                                                    \
        }                                                                \
    } while (0)

static std::vector<uint8_t> Bytes(const std::string& s) { return std::vector<uint8_t>(s.begin(), s.end()); }

static void TestLevel() {
    // Three rows; the last line is row 0. The middle row is short (12 chars) and the first is
    // long (14 chars), as two rows in the shipped levels are.
    auto d = Bytes("-|12345|abcdeX\r\nC|.....|-a-a\r\nW|00000|--#--\r\n__________\r\n-|99999|zzzzz\r\n");
    LevelGrid g;
    std::string err;
    CHECK(ParseLevel(d.data(), d.size(), &g, &err));
    CHECK(g.depth == 3);
    CHECK(g.Flag(0) == 'W' && g.Flag(1) == 'C' && g.Flag(2) == '-');
    CHECK(g.Cell(0, 2) == '0' && g.Object(0, 2) == '#');
    CHECK(g.Object(1, 3) == 'a' && g.Object(1, 4) == 0);   // missing position reads as 0
    CHECK(g.Cell(2, 0) == '1' && g.Object(2, 4) == 'e');   // the 14th character is ignored

    auto e = Bytes("\r\n-|00000|-----\r\n");  // an empty first line: no rows
    CHECK(!ParseLevel(e.data(), e.size(), &g, &err));
}

static void TestXFile() {
    auto d = Bytes(
        "xof 0303txt 0032\n"
        "Header {\n 1;\n 0;\n 1;\n}\n"
        "Mesh Box_0 {\n 4;\n 0.0;0.0;0.0;,\n 1.0;0.0;0.0;,\n 1.0;1.0;0.0;,\n 0.0;1.0;0.0;;\n"
        " 2;\n 3;0,1,2;,\n 3;0,2,3;;\n"
        " MeshNormals {\n 1;\n 0.0;0.0;-1.0;;\n 2;\n 3;0,0,0;,\n 3;0,0,0;;\n }\n"
        " MeshMaterialList {\n 2;\n 2;\n 0,\n 1;;\n"
        "  Material {\n 1.0;0.5;0.25;1.0;;\n 8.0;\n 0.0;0.0;0.0;;\n 0.0;0.0;0.0;;\n }\n"
        "  Material Red {\n 1.0;0.0;0.0;0.5;;\n 0.0;\n 0.0;0.0;0.0;;\n 0.1;0.2;0.3;;\n"
        "   TextureFilename {\n \"wall.bmp\";\n }\n  }\n }\n}\n");
    XModel m;
    std::string err;
    CHECK(ParseXFile(d.data(), d.size(), &m, &err));
    if (!err.empty()) std::printf("  x error: %s\n", err.c_str());
    CHECK(m.meshes.size() == 1);
    if (m.meshes.size() != 1) return;
    const XMesh& x = m.meshes[0];
    CHECK(x.name == "Box_0");
    CHECK(x.positions.size() == 12 && x.faces.size() == 2 && x.faces[1][2] == 3);
    CHECK(x.normals.size() == 3 && x.normalFaces.size() == 2);
    CHECK(x.faceMaterials.size() == 2 && x.faceMaterials[1] == 1);
    CHECK(x.materials.size() == 2);
    if (x.materials.size() == 2) {
        CHECK(x.materials[0].diffuse[1] == 0.5f && x.materials[0].power == 8.0f);
        CHECK(x.materials[1].name == "Red" && x.materials[1].texture == "wall.bmp");
        CHECK(x.materials[1].emissive[2] == 0.3f);
    }
    CHECK(m.TriangleCount() == 2);

    auto bad = Bytes("xof 0303txt 0032\nMesh M {\n 1;\n 0;0;0;;\n 1;\n 3;0,1,2;;\n}\n");
    CHECK(!ParseXFile(bad.data(), bad.size(), &m, &err));  // index out of range
}

static void TestDib() {
    // 3x2, 8-bit, bottom-up, 2-colour palette
    std::vector<uint8_t> d(40 + 8 + 4 * 2, 0);
    auto p32 = [&](size_t o, uint32_t v) { for (int i = 0; i < 4; i++) d[o + i] = (v >> (8 * i)) & 0xFF; };
    p32(0, 40); p32(4, 3); p32(8, 2);
    d[12] = 1; d[14] = 8; p32(32, 2);
    d[40] = 0x00; d[41] = 0x00; d[42] = 0xFF;          // palette 0: red
    d[44] = 0xFF; d[45] = 0x00; d[46] = 0x00;          // palette 1: blue
    uint8_t* bits = &d[48];
    bits[0] = 1; bits[1] = 0; bits[2] = 0;             // bottom row
    bits[4] = 0; bits[5] = 0; bits[6] = 1;             // top row
    Image img;
    CHECK(DecodeDIB(d.data(), d.size(), &img));
    CHECK(img.width == 3 && img.height == 2 && img.bpp == 8);
    CHECK(img.pixels[2] == 0xFF0000FFu);               // top-right: blue
    CHECK(img.pixels[3] == 0xFF0000FFu && img.pixels[4] == 0xFFFF0000u);
    auto bmp = DIBToBmpFile(d.data(), d.size());
    CHECK(bmp.size() == d.size() + 14 && bmp[0] == 'B' && bmp[10] == 14 + 48);
}

static void TestWave() {
    std::vector<uint8_t> d = Bytes("RIFF\0\0\0\0WAVEfmt ");
    d.resize(12);
    const char hdr[] = "WAVEfmt ";
    (void)hdr;
    auto put = [&](const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; d.insert(d.end(), b, b + n); };
    d.assign({'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'});
    put("fmt ", 4);
    uint32_t len = 16; put(&len, 4);
    uint16_t fmt = 1, ch = 1; put(&fmt, 2); put(&ch, 2);
    uint32_t rate = 22050, bytesPerSec = 22050; put(&rate, 4); put(&bytesPerSec, 4);
    uint16_t align = 1, bits = 8; put(&align, 2); put(&bits, 2);
    put("data", 4);
    uint32_t dl = 3; put(&dl, 4);
    uint8_t s[3] = {128, 255, 0}; put(s, 3);
    WaveData w;
    CHECK(DecodeWave(d.data(), d.size(), &w));
    CHECK(w.channels == 1 && w.sampleRate == 22050 && w.FrameCount() == 3);
    CHECK(w.samples[0] == 0.0f && w.samples[2] == -1.0f);
}

int main() {
    TestLevel();
    TestXFile();
    TestDib();
    TestWave();
    std::printf(g_fail ? "%d check(s) failed\n" : "all format tests passed\n", g_fail);
    return g_fail ? 1 : 0;
}
