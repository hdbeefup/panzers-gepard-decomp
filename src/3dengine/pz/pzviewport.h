// src/3dengine/pz/pzviewport.h
// pz::SViewport: the primary HD viewport as a facade over the SWINE device.
// OWNER: agent A.
//
// Not HD-sized on purpose: HD SViewport (0x244) owns the device, the swap
// chain and the board; in the recompile the SWINE SGepard owns all three
// (one D3D9 device for 2D and 3D). The camera, view, projection and screen
// matrices are kept under their HD names (HD offsets in comments).

#ifndef PZ_PZVIEWPORT_H
#define PZ_PZVIEWPORT_H

#include <vector>
#include "iviewport.h"

namespace pz {

struct SViewport : SIViewport {
    SViewport();
    ~SViewport();

    void SetPosition(int x, int y, int w, int h) override;
    void Resize(int width, int height) override;
    void SetFullScreenMode(int w, int h, int bpp, bool vsync, int refresh, int aa, int aaq, bool p8) override;
    void SetWindowedMode(int x, int y, int w, int h) override;
    void Slot_10() override;
    bool NextFullScreenMode(int* w, int* h, int p3) override;
    bool PrevFullScreenMode(int* w, int* h, int p3) override;
    void Slot_1C() override;
    void SetCamera(float x, float y, float z, float yaw, float pitch) override;
    void GetCamera(float* x, float* y, float* z, float* yaw, float* pitch) override;
    void SetProjection(float fovRadians, float nearZ, float farZ) override;
    void Slot_2C() override;
    void Slot_30() override;
    void ScreenToRay(float* out6, int x, int y) override;
    void GetSelectionPlanes(float* out16, int x1, int y1, int x2, int y2) override;
    void ProjectToScreen(const float* pos, float size, float* x, float* y, float* screenSize, float* z, int* fogAlpha) override;
    void Slot_40() override;
    void GetGroundCorners(float height, float* out12) override;
    void Slot_48() override;
    SIBoard* GetBoard() override;
    void Render(SIScene* scene, unsigned clearColor) override;
    int CreateSubport(int x, int y, int w, int h) override;
    void DestroySubport(int index) override;
    SIViewport* GetSubport(int index) override;
    int GetSubportCount() override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Slot_70() override;
    void Slot_74() override;
    void FrontBufferScreenshot(int p1, int p2, int p3, int p4) override;
    void SetDrawBoard(bool on) override;
    void SetDrawScene(bool on) override;

    void Clear(unsigned color, float z, unsigned stencil);   // 0x689f10
    void UpdateScreenMatrix();                               // 0x68c070
    void ApplyTransforms();                                  // VIEW (0x68d370 tail) + PROJECTION (0x68d160)

    SCameraParams Camera;        // yaw +0x7c, pitch +0x80, eye +0x84..; fov +0xcc, near +0xd0, far +0xd4
    bool          ProjectionSet; // SetProjection was called at least once
    float         CamOffset[3];  // +0xc0 one-shot eye offset (added and cleared by SetCamera)
    float         View[12];      // +0x90 3x4
    float         Proj[16];      // +0xd8
    float         ZBias;         // +0x118
    bool          ZBiasOff;      // +0x11c
    int           Left, Top;     // +0x120, +0x124
    int           Width, Height; // +0x128, +0x12c
    float         ScreenM[16];   // +0x130 viewport scale/offset
    float         ViewProjScreen[16]; // +0x170
    float         SizeScale;     // +0x1f0 Proj[0] * Width / 2
    float         InvViewProjScreen[16]; // +0x1b0 inverse of +0x170 (screen -> world)
    int           Mode = 0;      // +0x78 0 primary windowed, 3 sub viewport
    std::vector<SViewport*> Subports; // +0x1f8 SDArray<SViewport*> (CreateSubport)
    bool          Selected = false;   // +0x71 sub viewport selected for drawing (0x68c8e0): its device viewport and transforms apply
    bool          DrawBoard = true;   // +0x240 (ctor 0x689130: 1)
    bool          DrawScene = true;   // +0x241 (ctor 0x689130: 1)
};

} // namespace pz

#endif // PZ_PZVIEWPORT_H
