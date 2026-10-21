// Checks that ColDet builds and answers the kinds of query the game makes.
#include <cstdio>
#include "coldet.h"

static int g_fail = 0;
#define CHECK(c)                                                              \
    do {                                                                      \
        if (!(c)) {                                                           \
            std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #c); \
            g_fail++;                                                         \
        }                                                                     \
    } while (0)

// A unit cube centred on the origin, as 12 triangles.
static CollisionModel3D* Cube(float s) {
    float v[8][3];
    for (int i = 0; i < 8; i++) {
        v[i][0] = (i & 1) ? s : -s;
        v[i][1] = (i & 2) ? s : -s;
        v[i][2] = (i & 4) ? s : -s;
    }
    static const int f[12][3] = {{0, 1, 3}, {0, 3, 2}, {4, 6, 7}, {4, 7, 5}, {0, 4, 5}, {0, 5, 1},
                                 {2, 3, 7}, {2, 7, 6}, {0, 2, 6}, {0, 6, 4}, {1, 5, 7}, {1, 7, 3}};
    CollisionModel3D* m = newCollisionModel3D();
    m->setTriangleNumber(12);
    for (const auto& t : f) m->addTriangle(v[t[0]], v[t[1]], v[t[2]]);
    m->finalize();
    return m;
}

static void Translate(CollisionModel3D* m, float x, float y, float z) {
    float t[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};  // row-major, D3D style
    m->setTransform(t);
}

int main() {
    CollisionModel3D* a = Cube(1.0f);
    CollisionModel3D* b = Cube(1.0f);

    Translate(a, 0, 0, 0);
    Translate(b, 1.5f, 0, 0);
    CHECK(a->collision(b));  // overlapping

    Translate(b, 3.0f, 0, 0);
    CHECK(!a->collision(b));  // apart

    float origin[3] = {-5, 0, 0}, dir[3] = {1, 0, 0};
    CHECK(a->rayCollision(origin, dir));
    float p[3];
    CHECK(a->getCollisionPoint(p, true) && p[0] > -1.01f && p[0] < -0.99f);

    float c[3] = {0, 1.4f, 0};
    CHECK(a->sphereCollision(c, 0.5f));
    c[1] = 2.0f;
    CHECK(!a->sphereCollision(c, 0.5f));

    float c1[3] = {0, 0, 0}, c2[3] = {1.5f, 0, 0}, point[3];
    CHECK(SphereSphereCollision(c1, 1.0f, c2, 1.0f, point));

    delete a;
    delete b;
    std::printf(g_fail ? "%d check(s) failed\n" : "all collision tests passed\n", g_fail);
    return g_fail ? 1 : 0;
}
