#include "gl.h"
#include "../core/log.h"
#include <SDL3/SDL.h>
#include <cstring>

namespace hgl {
#define MBN_GL_DEFINE(ret, name, args) PFN_##name name = nullptr;
MBN_GL_FUNCS(MBN_GL_DEFINE)
#undef MBN_GL_DEFINE

bool Load() {
    bool ok = true;
#define MBN_GL_LOAD(ret, name, args)                                       \
    name = (PFN_##name)SDL_GL_GetProcAddress(#name);                          \
    if (!name && strcmp(#name, "glProvokingVertex") != 0) {                   \
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
