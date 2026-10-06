// src/3dengine/pz/shadowmath.h
// The light-space fit of the HD shadow buffer (SScene::GenerateShadowBuffer
// 0x6aac20): the trapezoidal fit 0x6a3c80, the receiver bounds 0x6a27f0 and
// the row-vector 4x4 helpers they use (HD SMatrix44 0x7c5fb0 / 0x7c6a90 /
// 0x7c71b0). Pure functions over plain data, so they can be checked against
// the HD code under an emulator. OWNER: agent SH.

#ifndef PZ_SHADOWMATH_H
#define PZ_SHADOWMATH_H

namespace pz {

// Row-vector 4x4 matrices (D3D layout), as HD.
void M44Identity(float* m);                                   // 0x67cbc0
void M44Mul(float* out, const float* a, const float* b);      // 0x7c5fb0 out = a * b
void M44Inverse(float* out, const float* m);                  // 0x7c6a90 (singular: identity)
void M44Transpose(float* out, const float* m);                // 0x7c71b0
void M34Inverse44(float* out, const float* m34);              // 0x7c59c0 + 0x676f60

// The camera the shadow fit reads from the HD viewport.
struct SShadowCamera {
    float View[12];   // SViewport +0x90 (0x68b740), 3x4
    float Proj[16];   // SViewport +0xd8 (0x68bd80)
    float Near;       // SViewport +0xd0 (0x68b850)
    float Far;        // SViewport +0xd4 (0x68b7a0)
};

// One terrain cell (8x8 tiles) as the fit reads it: STerrain parcel info
// (+0x04 visible, +0x08 min y, +0x0c max y) and the scene's highest model
// top in the cell (SScene +0x1a8, a float in an int array).
struct SShadowCell {
    bool  Visible;
    float MinY;
    float MaxY;
    float ModelTop;
};

// PANZERS 0x6a3c80
// Trapezoidal shadow-map matrix (Martin & Tan) for the sun direction: the
// camera frustum is seen along the sun, its centre line and hull are
// fitted with a trapezoid and the trapezoid is mapped to the unit square.
// Returns false when the near and far quads overlap on both axes (HD then
// keeps its own projection unless that one is too large).
bool ShadowTrapezoidMatrix(const SShadowCamera& cam, const float sun[3], float out[16]);

// PANZERS 0x6a27f0
// Bounds of the visible terrain cells (each box clipped by the camera
// frustum) after projecting with m (divided by w).
void ShadowReceiverBounds(const SShadowCamera& cam, const SShadowCell* cells, int cellsX, int cellsZ,
                          const float* m, float mn[3], float mx[3]);

// The matrices of one SScene::GenerateShadowBuffer 0x6aac20 call.
struct SShadowFrame {
    // in
    SShadowCamera      Cam;
    float              CamY;           // viewport GetCamera (+0x24)
    float              Yaw;
    float              SunDir[3];      // scene +0x110
    float              SunAzimuth;     // scene +0x108
    float              SunElevation;   // scene +0x10c
    const SShadowCell* Cells;          // terrain parcels + scene +0x1a8
    int                CellsX, CellsZ;
    int                Technique;      // Gepard option 2
    int                BufferSize;     // Gepard option 3
    bool               DepthTextureFlag; // SGepard +0x499
    // out
    float FocusHeight;                 // scene +0x98: 2 * lowest visible ground
    int   Cull;                        // scene +0x9c
    bool  Trapezoid;                   // the trapezoid (0x6a3c80) fit is used
    float Projection[16];              // scene +0x18, device PROJECTION of the pass
    float PassMatrix[16];              // scene +0x58 during the pass (techniques 3/4)
    float TexMatrix[16];               // scene +0x58 after the pass
};

// PANZERS 0x6aac20 (the matrix part)
// The light projection: every point is moved along the sun onto the plane
// y = FocusHeight and seen by the camera (L * View * Proj), with the height
// (top 0, bottom 1) as depth; then either the trapezoidal fit or this one
// is scaled to the bounds of the visible ground.
void ShadowFrameMatrices(SShadowFrame& f);

} // namespace pz

#endif // PZ_SHADOWMATH_H
