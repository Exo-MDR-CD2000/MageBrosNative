#include "gl.h"
#include "../core/log.h"
#include <SDL3/SDL.h>
#include <cstring>

namespace hgl {
#define MBN_GL_DEFINE(ret, name, args) PFN_##name name = nullptr;
MBN_GL_FUNCS(MBN_GL_DEFINE)
#undef MBN_GL_DEFINE

namespace {
// The pair of depth-clear calls differs by backend: GLES has the f form, desktop GL the double
// one. Provoking-vertex control is optional everywhere: the game only shades smoothly.
bool OptionalOnThisBackend(const char* name) {
    if (strcmp(name, "glProvokingVertex") == 0) return true;
#if defined(MBN_GLES)
    // Desktop-only entry points that the ES backend replaces: the depth clear, the draw-buffer
    // selection for the shadow map (glDrawBuffers) and the 64-bit timer results (the ES 3.0
    // glGetQueryObjectuiv). The game supplies the ES calls where it uses them.
    return strcmp(name, "glClearDepth") == 0 || strcmp(name, "glDrawBuffer") == 0 ||
           strcmp(name, "glGetQueryObjectiv") == 0 || strcmp(name, "glGetQueryObjectui64v") == 0;
#else
    return strcmp(name, "glClearDepthf") == 0;
#endif
}
}  // namespace

bool Load() {
    bool ok = true;
#define MBN_GL_LOAD(ret, name, args)                                       \
    name = (PFN_##name)SDL_GL_GetProcAddress(#name);                          \
    if (!name && !OptionalOnThisBackend(#name)) {                             \
        Log("OpenGL: missing function %s", #name);                            \
        ok = false;                                                           \
    }
    MBN_GL_FUNCS(MBN_GL_LOAD)
#undef MBN_GL_LOAD
    return ok;
}

bool HasExtension(const char* name) {
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++) {
        const char* e = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
        if (e && strcmp(e, name) == 0) return true;
    }
    return false;
}
}  // namespace hgl
