// Platform probe for the R36S: answers "can this console run the game's SDL3 and GLES path"
// before any renderer work. It is not part of the game and is not shipped to players.
//
//   mbn_r36s_probe          initialise video/audio/gamepad, make an ES 3.0 context,
//                           print what the drivers gave us, play a short tone, clear the
//                           screen for a few seconds and exit 0
//
// Built by tools/r36s/build.sh with -DMBN_BUILD_R36S_PROBE=ON.
#include <SDL3/SDL.h>
#include "gfx/gl.h"
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
constexpr GLenum kGlSamples = 0x80A9;  // GL_SAMPLES, not in the game's loader
constexpr GLenum kGlShadingLanguageVersion = 0x8B8C;  // GL_SHADING_LANGUAGE_VERSION

// `--buttons`: print raw joystick button indices as they are pressed. The SDL mapping for an
// unlisted pad stops at the triggers, so this is how Select/Start (back:b8, start:b9) are
// confirmed on a new device.
int RunButtons() {
    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        std::printf("probe_failed: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    int n = 0;
    SDL_JoystickID* ids = SDL_GetJoysticks(&n);
    if (!ids || n < 1) {
        std::printf("probe_failed: no joystick\n");
        SDL_free(ids);
        return 1;
    }
    SDL_Joystick* joy = SDL_OpenJoystick(ids[0]);
    std::printf("buttons: press Start and Select now (20 s)\n");
    std::printf("joystick: %s, %d buttons\n", joy ? SDL_GetJoystickName(joy) : "?", joy ? SDL_GetNumJoystickButtons(joy) : 0);
    bool down[64] = {};
    for (int tick = 0; tick < 400; tick++) {  // 400 x 50 ms = 20 s
        int count = joy ? SDL_GetNumJoystickButtons(joy) : 0;
        for (int b = 0; b < count && b < 64; b++) {
            bool now = SDL_GetJoystickButton(joy, b);
            if (now != down[b]) {
                down[b] = now;
                std::printf("button %d %s\n", b, now ? "down" : "up");
                std::fflush(stdout);
            }
        }
        SDL_Delay(50);
    }
    if (joy) SDL_CloseJoystick(joy);
    SDL_free(ids);
    SDL_Quit();
    return 0;
}

void PrintDrivers() {
    int ver = SDL_GetVersion();
    std::printf("sdl_version: %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(ver), SDL_VERSIONNUM_MINOR(ver),
                SDL_VERSIONNUM_MICRO(ver));
    const char* video = SDL_GetCurrentVideoDriver();
    const char* audio = SDL_GetCurrentAudioDriver();
    std::printf("video_driver: %s\n", video ? video : "(none)");
    std::printf("audio_driver: %s\n", audio ? audio : "(none)");

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display))
        std::printf("display: %dx%d @ %.2f Hz\n", mode->w, mode->h, mode->refresh_rate);
}

void PrintGamepads() {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    std::printf("gamepads: %d\n", count);
    for (int i = 0; ids && i < count; i++) {
        SDL_Gamepad* pad = SDL_OpenGamepad(ids[i]);
        if (!pad) {
            std::printf("  [%d] could not open: %s\n", i, SDL_GetError());
            continue;
        }
        char guid[64] = "";
        SDL_GUIDToString(SDL_GetGamepadGUIDForID(ids[i]), guid, sizeof guid);
        char* mapping = SDL_GetGamepadMapping(pad);
        std::printf("  [%d] %s\n      guid: %s\n      mapping: %s\n", i, SDL_GetGamepadName(pad), guid,
                    mapping ? mapping : "(none)");
        SDL_free(mapping);
        SDL_CloseGamepad(pad);
    }
    SDL_free(ids);
}

// A short tone through the default device: the console's speaker path is part of the probe.
void PlayTone() {
    SDL_AudioSpec spec{SDL_AUDIO_F32, 2, 48000};
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream) {
        std::printf("audio: could not open the default device: %s\n", SDL_GetError());
        return;
    }
    std::printf("audio: %d Hz, %d channels, format %u\n", spec.freq, spec.channels, (unsigned)spec.format);
    const int frames = spec.freq * 7 / 10;  // 0.7 s
    std::vector<float> pcm((size_t)frames * spec.channels);
    for (int i = 0; i < frames; i++) {
        float s = 0.2f * std::sin(6.2831853f * 440.0f * (float)i / (float)spec.freq);
        for (int c = 0; c < spec.channels; c++) pcm[(size_t)i * spec.channels + c] = s;
    }
    SDL_ResumeAudioStreamDevice(stream);
    SDL_PutAudioStreamData(stream, pcm.data(), (int)(pcm.size() * sizeof(float)));
    SDL_Delay(800);
    SDL_DestroyAudioStream(stream);
}
}  // namespace

int main(int argc, char** argv) {
    bool draw = true;
    for (int i = 1; i < argc; i++) {
        if (SDL_strcmp(argv[i], "--buttons") == 0) return RunButtons();
        if (SDL_strcmp(argv[i], "--no-draw") == 0) draw = false;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        std::printf("probe_failed: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    PrintDrivers();
    PrintGamepads();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_Window* window = SDL_CreateWindow("MageBrosNative R36S probe", 640, 480, SDL_WINDOW_OPENGL);
    if (!window) {
        std::printf("probe_failed: SDL_CreateWindow: %s\n", SDL_GetError());
        return 2;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::printf("probe_failed: SDL_GL_CreateContext (ES 3.0): %s\n", SDL_GetError());
        return 3;
    }
    SDL_GL_MakeCurrent(window, context);

    // Load what the game's renderer needs, and say which of those calls exist here.
    bool complete = hgl::Load();
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* glsl = (const char*)glGetString(kGlShadingLanguageVersion);
    GLint samples = 0;
    glGetIntegerv(kGlSamples, &samples);
    std::printf("gl_version: %s\n", version ? version : "(none)");
    std::printf("gl_renderer: %s\n", renderer ? renderer : "(none)");
    std::printf("gl_vendor: %s\n", vendor ? vendor : "(none)");
    std::printf("glsl_version: %s\n", glsl ? glsl : "(none)");
    std::printf("msaa_samples: %d\n", (int)samples);
    std::printf("provoking_vertex: %s\n", hgl::glProvokingVertex ? "available" : "absent");
    std::printf("game_gl_functions: %s\n", complete ? "complete" : "some are missing (logged above)");

    PlayTone();

    if (draw) {
        for (int frame = 0; frame < 180; frame++) {  // about 3 seconds at 60 Hz
            float t = (float)frame / 60.0f;
            glClearColor(0.1f + 0.3f * (0.5f + 0.5f * std::sin(t * 2.0f)), 0.4f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            SDL_GL_SwapWindow(window);
            SDL_Delay(16);
        }
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::printf("probe_ok\n");
    return 0;
}
