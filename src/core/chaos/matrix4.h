#ifndef CORE_CHAOS_MATRIX4_H
#define CORE_CHAOS_MATRIX4_H
struct SMatrix4 {
    union {
        struct { float m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,m23,m30,m31,m32,m33; };
        float m[4][4];
    };
};
#endif
