// Tests the bundled TimGM6mb bank without the game or a sound device: load it through the same
// TinySoundFont build the game uses, play a note and require that it renders sound.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define TSF_IMPLEMENTATION
#include "tsf.h"

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : MBN_SOUNDFONT_PATH;
    tsf* f = tsf_load_filename(path);
    if (!f) {
        std::fprintf(stderr, "FAIL: could not load %s\n", path);
        return 1;
    }

    tsf_set_output(f, TSF_STEREO_INTERLEAVED, 44100, 0.0f);
    tsf_channel_set_presetnumber(f, 0, 0, 0);  // Acoustic Grand Piano
    tsf_channel_note_on(f, 0, 60, 1.0f);       // middle C, fortissimo

    constexpr int kBlock = 2048;
    std::vector<short> buffer((size_t)kBlock * 2);
    int peak = 0;
    for (int block = 0; block < 8; block++) {
        tsf_render_short(f, buffer.data(), kBlock, 0);
        for (short sample : buffer) peak = std::max(peak, std::abs((int)sample));
    }
    tsf_close(f);

    if (peak <= 0) {
        std::fprintf(stderr, "FAIL: %s rendered silence\n", path);
        return 1;
    }
    std::printf("TimGM6mb rendered a note, peak %d\n", peak);
    return 0;
}
