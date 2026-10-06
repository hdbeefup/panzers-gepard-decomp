// src/3dengine/pz/shadowmath.cpp
// Light-space fit of the HD shadow buffer. See shadowmath.h. OWNER: agent SH.
//
// The arithmetic keeps HD's float/double mix: lengths are summed in float,
// square-rooted and inverted in double (1.0 / sqrt(double)), and the
// products are rounded back to float, as the SSE2 code does.

#include <math.h>
#include <float.h>
#include <string.h>
#include "shadowmath.h"

namespace pz {

// PANZERS 0x67cbc0
void M44Identity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// PANZERS 0x7c5fb0
void M44Mul(float* out, const float* a, const float* b)
{
    float r[16];
    for (int i = 0; i < 4; ++i) {
        const float* ai = a + i * 4;
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = ai[0] * b[j] + ai[1] * b[4 + j] + ai[2] * b[8 + j] + ai[3] * b[12 + j];
    }
    memcpy(out, r, 64);
}

// PANZERS 0x7c71b0
void M44Transpose(float* out, const float* m)
{
    float r[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r[j * 4 + i] = m[i * 4 + j];
    memcpy(out, r, 64);
}

// PANZERS 0x7c6a90
// Cofactor inverse; a zero determinant gives the identity.
void M44Inverse(float* out, const float* m)
{
    float c[16];
    c[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    c[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    c[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    c[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    c[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    c[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    c[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    c[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    c[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    c[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    c[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    c[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    c[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    c[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    c[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    c[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    float det = m[0] * c[0] + m[1] * c[4] + m[2] * c[8] + m[3] * c[12];
    if (det == 0.0f) {
        M44Identity(out);
        return;
    }
    float inv = (float)(1.0 / (double)det);
    for (int i = 0; i < 16; ++i)
        out[i] = c[i] * inv;
}

// PANZERS 0x7c59c0 (3x4 inverse), widened by 0x676f60
void M34Inverse44(float* out, const float* m)
{
    float a[16] = {
        m[0], m[1], m[2], 0.0f,
        m[3], m[4], m[5], 0.0f,
        m[6], m[7], m[8], 0.0f,
        m[9], m[10], m[11], 1.0f,
    };
    M44Inverse(out, a);
}

// PANZERS 0x676f60 (3x4 -> 4x4)
static void Mat34To44Local(float* o, const float* m)
{
    const float a[16] = {
        m[0], m[1], m[2], 0.0f,
        m[3], m[4], m[5], 0.0f,
        m[6], m[7], m[8], 0.0f,
        m[9], m[10], m[11], 1.0f,
    };
    memcpy(o, a, 64);
}

// (x * inv) rounded to float with inv = 1 / sqrt(len2) in double, HD's
// normalisation idiom.
static inline double InvLen(float len2)
{
    return 1.0 / sqrt((double)len2);
}

// PANZERS 0x6a3c80
bool ShadowTrapezoidMatrix(const SShadowCamera& cam, const float sun[3], float out[16])
{
    bool ok = true;
    const float* P = cam.Proj;
    const float n = cam.Near, f = cam.Far;
    // NDC depth of the near and far planes.
    float zn = (P[6] * 0.0f + P[2] * 0.0f + P[10] * n + P[14]) / (P[7] * 0.0f + P[3] * 0.0f + P[11] * n + P[15]);
    float zf = (P[6] * 0.0f + P[2] * 0.0f + P[10] * f + P[14]) / (P[7] * 0.0f + P[3] * 0.0f + P[11] * f + P[15]);

    // Camera axes from the inverse view; the light basis u, w, sun.
    float iv[16];
    M34Inverse44(iv, cam.View);
    double k = InvLen(iv[8] * iv[8] + iv[9] * iv[9] + iv[10] * iv[10]);
    float fx = (float)((double)iv[8] * k), fy = (float)((double)iv[9] * k), fz = (float)((double)iv[10] * k);
    float ux = fz * sun[1] - fy * sun[2];
    float uy = fx * sun[2] - fz * sun[0];
    float uz = fy * sun[0] - fx * sun[1];
    k = InvLen(uy * uy + ux * ux + uz * uz);
    ux = (float)((double)ux * k);
    uy = (float)((double)uy * k);
    uz = (float)((double)uz * k);
    float wx = uy * sun[2] - uz * sun[1];
    float wy = uz * sun[0] - ux * sun[2];
    float wz = ux * sun[1] - uy * sun[0];
    k = InvLen(wy * wy + wx * wx + wz * wz);
    wx = (float)((double)wx * k);
    wy = (float)((double)wy * k);
    wz = (float)((double)wz * k);
    float R[16] = {
        ux, uy, uz, 0.0f,
        wx, wy, wz, 0.0f,
        sun[0], sun[1], sun[2], 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    float lightView[16];   // HD +0x500 (its LookAt result is overwritten with this)
    M44Inverse(lightView, R);
    float focus = 20.0f / (f - n);
    float focusZ = focus * 0.0f + 0.0f;
    // (HD also builds a LookAt from 512 units up the sun and a 1x1 ortho;
    // both results are dead.)
    float V[16], VP[16], iVP[16];
    Mat34To44Local(V, cam.View);
    M44Mul(VP, V, P);
    M44Inverse(iVP, VP);

    // The frustum corners and the near/far centres, seen along the sun.
    const float* r = lightView;
    auto toLight = [&](float x, float y, float z, float* o) {
        float X = iVP[8] * z + (iVP[0] * x + iVP[4] * y) + iVP[12];
        float Y = iVP[9] * z + (iVP[1] * x + iVP[5] * y) + iVP[13];
        float Z = iVP[10] * z + (iVP[2] * x + iVP[6] * y) + iVP[14];
        float W = iVP[11] * z + (iVP[3] * x + iVP[7] * y) + iVP[15];
        float lx = r[4] * Y + r[0] * X + r[8] * Z + r[12] * W;
        float ly = r[5] * Y + r[1] * X + r[9] * Z + r[13] * W;
        float lw = r[7] * Y + r[3] * X + r[11] * Z + r[15] * W;
        o[0] = lx / lw;
        o[1] = ly / lw;
    };
    float pt[8][2];
    toLight(-1.0f, -1.0f, zn, pt[0]);
    toLight(1.0f, -1.0f, zn, pt[1]);
    toLight(1.0f, 1.0f, zn, pt[2]);
    toLight(-1.0f, 1.0f, zn, pt[3]);
    toLight(-1.0f, -1.0f, zf, pt[4]);
    toLight(1.0f, -1.0f, zf, pt[5]);
    toLight(1.0f, 1.0f, zf, pt[6]);
    toLight(-1.0f, 1.0f, zf, pt[7]);
    float nc[2], fc[2];
    toLight(0.0f, 0.0f, zn, nc);
    toLight(0.0f, 0.0f, zf, fc);

    // Centre line and its extreme points (top: nearest the near plane).
    float lx = fc[0] - nc[0], ly = fc[1] - nc[1];
    float top[2] = { 0.0f, 0.0f }, bot[2] = { 0.0f, 0.0f };
    float best = FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        float d = (pt[i][0] - nc[0]) * lx + (pt[i][1] - nc[1]) * ly + 0.0f;
        if (d < best) {
            best = d;
            top[0] = pt[i][0];
            top[1] = pt[i][1];
        }
    }
    best = -FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        float d = (pt[i][0] - nc[0]) * lx + (pt[i][1] - nc[1]) * ly + 0.0f;
        if (best < d) {
            best = d;
            bot[0] = pt[i][0];
            bot[1] = pt[i][1];
        }
    }
    float focX = focus * lx + nc[0];
    float focY = focus * ly + nc[1];
    float lambda = (float)sqrt((double)((bot[1] - top[1]) * (bot[1] - top[1]) + (bot[0] - top[0]) * (bot[0] - top[0]) + 0.0f));
    float delta = (float)sqrt((double)((focY - top[1]) * (focY - top[1]) + (focX - top[0]) * (focX - top[0]) +
                                       (focusZ - 0.0f) * (focusZ - 0.0f)));
    // xi: -0.6 (the "80% rule") when the camera looks across the sun, up to
    // 0.2 when it looks along it.
    double cosA = (double)(fx * sun[0] + fy * sun[1] + fz * sun[2]);
    float xi = 1.0f - (0.8f - (float)fabs(cosA) * 0.4f) * 2.0f;
    float ld = delta * lambda;
    double llen = sqrt((double)(lx * lx + ly * ly + 0.0f));
    float eta = (float)fabs((double)((ld * xi + ld) / ((lambda - delta * 2.0f) - xi * lambda))) / (float)llen;
    // Projection centre q, eta behind the top line.
    float qx = top[0] - eta * lx;
    float qy = top[1] - eta * ly;
    float qz = 0.0f - eta * 0.0f;
    float nqz = 0.0f - qz;
    float perpX = -ly;
    float along0 = (nc[0] - qx) * lx + (nc[1] - qy) * ly + nqz * 0.0f;
    // Side lines: the widest point on each side of the centre line.
    float left[2] = { 0.0f, 0.0f }, right[2] = { 0.0f, 0.0f };
    float leftBest = -FLT_MAX, rightBest = -FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        float perp = (pt[i][1] - qy) * lx + (pt[i][0] - qx) * perpX + nqz * 0.0f;
        float along = (pt[i][0] - qx) * lx + (pt[i][1] - qy) * ly + nqz * 0.0f;
        float t = (float)fabs((double)((perp / along) * along0));
        if (0.0f <= perp) {
            if (leftBest < t) {
                left[0] = pt[i][0];
                left[1] = pt[i][1];
                leftBest = t;
            }
        } else if (rightBest < t) {
            right[0] = pt[i][0];
            right[1] = pt[i][1];
            rightBest = t;
        }
    }
    // Near and far quads overlapping on both axes: no usable trapezoid.
    unsigned bits = 0;
    for (int i = 0; i < 4 && bits != 0xf; ++i)
        for (int j = 4; j < 8 && bits != 0xf; ++j) {
            if (pt[i][0] < pt[j][0]) bits |= 1;
            if (pt[i][1] < pt[j][1]) bits |= 2;
            if (pt[j][0] < pt[i][0]) bits |= 4;
            if (pt[j][1] < pt[i][1]) bits |= 8;
        }
    if (bits == 0xf)
        ok = false;

    // Trapezoid corners: the side lines cut by the top and bottom lines.
    double il = 1.0 / llen;
    float lnx = (float)((double)lx * il);
    float lny = (float)((double)ly * il);
    float lnz = (float)(il * 0.0);
    float rqx = right[0] - qx, rqy = right[1] - qy;
    float topd = (top[0] - qx) * lnx + (top[1] - qy) * lny + (0.0f - qz) * lnz;
    float rAlong = rqx * lnx + rqy * lny + (0.0f - qz) * lnz;
    float s = topd / rAlong;
    float trX = s * rqx + qx, trY = s * rqy + qy;
    float lqx = left[0] - qx, lqy = left[1] - qy;
    s = topd / (lqx * lnx + lqy * lny + (0.0f - qz) * lnz);
    float tlY = s * lqy + qy, tlX = s * lqx + qx;
    s = ((bot[0] - qx) * lnx + (bot[1] - qy) * lny + (0.0f - qz) * lnz) / rAlong;
    float brY = s * rqy + qy, brX = s * rqx + qx;

    // N_T = T1 R T2 H S1 N T3 S2 (row vectors).
    float sumX = tlX + trX, sumY = tlY + trY;
    float T1[16];
    M44Identity(T1);
    T1[12] = -(sumX * 0.5f);
    T1[13] = -(sumY * 0.5f);
    float dx = tlX - trX, dy = tlY - trY;
    float len = (float)sqrt((double)(dy * dy + dx * dx + 0.0f));
    float c = dx / len, sn = dy / len;
    float Rt[16] = {
        c, sn, 0.0f, 0.0f,
        sn, -c, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    float X[16];
    M44Mul(X, T1, Rt);
    float T2[16];
    M44Identity(T2);
    T2[12] = -(X[4] * qy + X[0] * qx + X[8] * 0.0f + X[12]);
    T2[13] = -(X[5] * qy + X[1] * qx + X[9] * 0.0f + X[13]);
    M44Mul(X, X, T2);
    float H[16];
    M44Identity(H);
    H[4] = -(((sumY * X[4] + X[0] * sumX + X[8] * 0.0f + X[12] * 2.0f) * 0.5f) /
             ((X[5] * sumY + X[1] * sumX + X[9] * 0.0f + X[13] * 2.0f) * 0.5f));
    M44Mul(X, X, H);
    float S1[16];
    M44Identity(S1);
    S1[0] = 1.0f / (tlX * X[0] + X[4] * tlY + X[8] * 0.0f + X[12]);
    S1[5] = 1.0f / (tlY * X[5] + X[1] * tlX + X[9] * 0.0f + X[13]);
    M44Mul(X, X, S1);
    float N[16];
    memset(N, 0, sizeof(N));
    N[0] = 1.0f;
    N[5] = 1.0f;
    N[7] = 1.0f;
    N[10] = 1.0f;
    N[13] = 1.0f;
    M44Mul(X, X, N);
    float T3[16];
    M44Identity(T3);
    {
        float yTL = X[5] * tlY + X[1] * tlX + X[9] * 0.0f + X[13];
        float wTL = X[7] * tlY + X[3] * tlX + X[11] * 0.0f + X[15];
        float yBR = X[5] * brY + X[1] * brX + X[9] * 0.0f + X[13];
        float wBR = X[7] * brY + X[3] * brX + X[11] * 0.0f + X[15];
        T3[13] = (yTL / wTL + yBR / wBR) * -0.5f;
    }
    M44Mul(X, X, T3);
    float S2[16];
    M44Identity(S2);
    {
        float yBR = X[1] * brX + brY * X[5] + X[9] * 0.0f + X[13];
        float wBR = X[7] * brY + X[3] * brX + X[11] * 0.0f + X[15];
        S2[5] = -(wBR / yBR);
    }
    M44Mul(X, X, S2);
    M44Mul(out, lightView, X);
    return ok;
}

// The six camera frustum planes of 0x6a27f0 (normal, d), inside where
// N.p + d <= 0: top, bottom, the two sides, near, far.
static void FrustumPlanes(const SShadowCamera& cam, float planes[6][4])
{
    float iv[16];
    M34Inverse44(iv, cam.View);
    const float* rt = iv;       // camera right
    const float* up = iv + 4;   // camera up
    const float* fw = iv + 8;   // camera forward
    const float* pos = iv + 12;
    float m00 = cam.Proj[0], m11 = cam.Proj[5];
    struct Def { float a, b, c; float pk; } defs[6] = {
        // n = right * a + up * b + fwd * c; point = pos + n * pk
        { 0.0f, m11, -1.0f, 0.0f },
        { 0.0f, -m11, -1.0f, 0.0f },
        { -m00, 0.0f, -1.0f, 0.0f },
        { m00, 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, -cam.Near },
        { 0.0f, 0.0f, 1.0f, cam.Far },
    };
    for (int i = 0; i < 6; ++i) {
        const Def& d = defs[i];
        float nv[3], p[3];
        for (int k = 0; k < 3; ++k) {
            nv[k] = up[k] * d.b + rt[k] * d.a + fw[k] * d.c;
            p[k] = nv[k] * d.pk + pos[k];
        }
        if (i < 4) {
            // HD: point = pos + n * (-0 / (m * m + 1)), i.e. the eye.
            for (int k = 0; k < 3; ++k)
                p[k] = pos[k];
        }
        double inv = InvLen(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2]);
        for (int k = 0; k < 3; ++k)
            planes[i][k] = (float)((double)nv[k] * inv);
        planes[i][3] = -(planes[i][0] * p[0] + planes[i][1] * p[1] + planes[i][2] * p[2]);
    }
}

// Solves the 3x3 system of three planes (N.p + d = 0).
static bool Intersect3(const float* a, const float* b, const float* c, double* p)
{
    double bc[3] = { (double)b[1] * c[2] - (double)b[2] * c[1], (double)b[2] * c[0] - (double)b[0] * c[2],
                     (double)b[0] * c[1] - (double)b[1] * c[0] };
    double det = a[0] * bc[0] + a[1] * bc[1] + a[2] * bc[2];
    if (fabs(det) < 1e-9)
        return false;
    double ca[3] = { (double)c[1] * a[2] - (double)c[2] * a[1], (double)c[2] * a[0] - (double)c[0] * a[2],
                     (double)c[0] * a[1] - (double)c[1] * a[0] };
    double ab[3] = { (double)a[1] * b[2] - (double)a[2] * b[1], (double)a[2] * b[0] - (double)a[0] * b[2],
                     (double)a[0] * b[1] - (double)a[1] * b[0] };
    for (int k = 0; k < 3; ++k)
        p[k] = -((double)a[3] * bc[k] + (double)b[3] * ca[k] + (double)c[3] * ab[k]) / det;
    return true;
}

// PANZERS 0x6a27f0
// HD builds each visible cell as a box polyhedron (8 vertices, 6 quads),
// clips it by the six frustum planes (0x7c7610, tolerance 0.001: vertices
// within it stay, a cell with no vertex strictly inside a plane is skipped)
// and projects the surviving vertices. Here the clipped polyhedron's
// vertices are found directly: every intersection of three of the twelve
// bounding planes that lies inside all of them.
void ShadowReceiverBounds(const SShadowCamera& cam, const SShadowCell* cells, int cellsX, int cellsZ,
                          const float* m, float mn[3], float mx[3])
{
    mn[0] = mn[1] = FLT_MAX;
    mn[2] = 3.4028235e+38f;
    mx[0] = mx[1] = -FLT_MAX;
    mx[2] = -3.4028235e+38f;
    float fr[6][4];
    FrustumPlanes(cam, fr);
    const double eps = 0.001;
    for (int z = 0; z < cellsZ; ++z)
        for (int x = 0; x < cellsX; ++x) {
            const SShadowCell& cell = cells[cellsX * z + x];
            if (!cell.Visible)
                continue;
            float top = cell.MaxY;
            if (top <= cell.ModelTop)
                top = cell.ModelTop;
            float lo[3] = { (float)(x * 8), cell.MinY, (float)(z * 8) };
            float hi[3] = { (float)(x * 8 + 8), top, (float)(z * 8 + 8) };
            float pl[12][4];
            for (int k = 0; k < 3; ++k) {
                float* a = pl[k * 2];
                float* b = pl[k * 2 + 1];
                a[0] = a[1] = a[2] = 0.0f;
                b[0] = b[1] = b[2] = 0.0f;
                a[k] = -1.0f;
                a[3] = lo[k];    // -p + lo <= 0
                b[k] = 1.0f;
                b[3] = -hi[k];   // p - hi <= 0
            }
            memcpy(pl[6], fr, sizeof(fr));
            // HD skips the cell when some plane leaves no vertex strictly inside.
            bool any = false;
            for (int i = 0; i < 12; ++i)
                for (int j = i + 1; j < 12; ++j)
                    for (int k = j + 1; k < 12; ++k) {
                        double p[3];
                        if (!Intersect3(pl[i], pl[j], pl[k], p))
                            continue;
                        bool in = true;
                        for (int q = 0; q < 12 && in; ++q)
                            if (pl[q][0] * p[0] + pl[q][1] * p[1] + pl[q][2] * p[2] + pl[q][3] > eps)
                                in = false;
                        if (!in)
                            continue;
                        any = true;
                        float vx = (float)p[0], vy = (float)p[1], vz = (float)p[2];
                        float iw = 1.0f / (m[7] * vy + m[3] * vx + m[11] * vz + m[15]);
                        float px = (m[4] * vy + vx * m[0] + m[8] * vz + m[12]) * iw;
                        float py = (m[5] * vy + m[1] * vx + m[9] * vz + m[13]) * iw;
                        float pz = (m[6] * vy + m[2] * vx + m[10] * vz + m[14]) * iw;
                        if (px < mn[0]) mn[0] = px;
                        if (mx[0] < px) mx[0] = px;
                        if (py < mn[1]) mn[1] = py;
                        if (mx[1] < py) mx[1] = py;
                        if (pz < mn[2]) mn[2] = pz;
                        if (mx[2] < pz) mx[2] = pz;
                    }
            (void)any;
        }
}

// PANZERS 0x6aac20 (matrix part; the passes are in pzscene.cpp)
void ShadowFrameMatrices(SShadowFrame& f)
{
    // Height range of the visible ground and models.
    float minY = FLT_MAX, maxY = -FLT_MAX;
    for (int i = 0, n = f.CellsX * f.CellsZ; i < n; ++i) {
        const SShadowCell& c = f.Cells[i];
        if (!c.Visible)
            continue;
        if (c.MinY < minY)
            minY = c.MinY;
        float v = c.MaxY;
        if (!(v > c.ModelTop))
            v = c.ModelTop;
        if (v > maxY)
            maxY = v;
    }
    const float H = minY * 2.0f;
    f.FocusHeight = H;
    const float* S = f.SunDir;
    const float* V = f.Cam.View;
    // A w offset when the camera looks away from the sun.
    float wOffset = 0.0f;
    if (!((V[5] * S[1] + V[2] * S[0]) + V[8] * S[2] > 0.0f)) {
        float s = (float)-sin((double)f.Yaw);
        float c = (float)-cos((double)f.Yaw);
        wOffset = ((S[1] * 0.0f + s * S[0]) + S[2] * c) * (maxY - minY);
    }
    f.Cull = (f.CamY <= H) ? 3 : 2;
    // L: along the sun onto y = H.
    double d = cos((double)f.SunElevation) / sin((double)f.SunElevation);
    double sa = sin((double)f.SunAzimuth);
    double ca = cos((double)f.SunAzimuth);
    float L[16];
    memset(L, 0, sizeof(L));
    L[0] = 1.0f;
    L[4] = (float)-(sa * d);
    L[6] = (float)-(ca * d);
    L[10] = 1.0f;
    L[12] = (float)((double)H * sa * d);
    L[13] = H;
    L[14] = (float)((double)H * ca * d);
    L[15] = 1.0f;
    float V4[16], M[16];
    Mat34To44Local(V4, V);
    M44Mul(M, L, V4);
    M44Mul(M, M, f.Cam.Proj);
    // Depth: the height, 0 at the top of the range, 1 at the bottom.
    M[2] = 0.0f;
    M[10] = 0.0f;
    M[6] = -1.0f / (maxY - minY);
    M[14] = maxY / (maxY - minY);
    M[15] = M[15] + wOffset;
    // Camera forward y (the bias of techniques 3/4).
    float iv[16];
    M34Inverse44(iv, V);
    float fy;
    {
        float x = iv[8], y = iv[9], z = iv[10];
        fy = (float)((1.0 / sqrt((double)(y * y + x * x + z * z))) * (double)y);
    }
    float B[16];
    memcpy(B, M, 64);
    float A[16];
    bool trapezoidFailed = !ShadowTrapezoidMatrix(f.Cam, S, A);
    if (!trapezoidFailed)
        memcpy(M, A, 64);
    float mn[3], mx[3];
    ShadowReceiverBounds(f.Cam, f.Cells, f.CellsX, f.CellsZ, M, mn, mx);
    if ((mx[0] - mn[0] > 15.0f || mx[1] - mn[1] > 15.0f) && trapezoidFailed) {
        memcpy(M, A, 64);
        trapezoidFailed = false;
        ShadowReceiverBounds(f.Cam, f.Cells, f.CellsX, f.CellsZ, M, mn, mx);
    }
    f.Trapezoid = !trapezoidFailed;
    // Scale the bounds to the buffer.
    float dx = mx[0] - mn[0], dy = mx[1] - mn[1];
    float F[16];
    memset(F, 0, sizeof(F));
    F[0] = 2.0f / dx;
    F[5] = 2.0f / dy;
    F[10] = 1.0f;
    F[12] = -((mn[0] + mx[0]) / dx);
    F[13] = -((mn[1] + mx[1]) / dy);
    F[15] = 1.0f;
    M44Mul(M, M, F);
    M[2] = B[2];
    M[6] = B[6];
    M[10] = B[10];
    M[14] = B[14];
    memcpy(f.Projection, M, 64);
    // Texture bias: clip space -> texel centres; z scale and depth bias by
    // technique.
    float BI[16];
    memset(BI, 0, sizeof(BI));
    BI[0] = 0.5f;
    BI[5] = -0.5f;
    BI[12] = (float)(0.5 / (double)f.BufferSize + 0.5);
    BI[13] = (float)(0.5 / (double)f.BufferSize + 0.5);
    switch (f.Technique) {
    case 2:
        BI[10] = (float)(f.DepthTextureFlag ? 0xffffff : 1);
        BI[14] = 0.0f;
        break;
    case 3:
    case 4: {
        BI[10] = 1.0f;
        if (trapezoidFailed) {
            BI[14] = -0.0001f;
        } else {
            float k = (float)pow((double)(1.0f - (float)fabs((double)fy)), 3.0);
            BI[14] = f.Technique == 4 ? -0.002f - k * 0.0003f : -1e-05f - k * 3e-05f;
        }
        break;
    }
    default:
        break;
    }
    BI[15] = 1.0f;
    memset(f.PassMatrix, 0, 64);
    if (f.Technique == 3 || f.Technique == 4) {
        f.PassMatrix[0] = M[2];
        f.PassMatrix[4] = M[6];
        f.PassMatrix[8] = M[10];
        f.PassMatrix[12] = M[14];
    }
    float T[16];
    M44Mul(T, iv, M);
    M44Mul(f.TexMatrix, T, BI);
}

} // namespace pz
