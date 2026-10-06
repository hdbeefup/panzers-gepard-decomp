// src/3dengine/pz/iviewport.h
// SIViewport: the HD viewport interface (RTTI SIViewport vftable 0x87878c,
// SViewport vftable 0x878814, 33 slots). HD SViewport is 0x244
// bytes (ctor 0x689130, CreateWindowedViewport 0x68ada0) and owns the D3D9
// device (+0x08), the board (+0x28) and the present parameters (+0x30).
//
// SHARED HEADER (owner P0): same rules as iscene.h.
//
// The world gets the primary viewport with Gepard GetViewport(0) (no AddRef)
// and only sets the camera on it (SWorld::ComputeCamera 0x5ddc30) or asks it
// to draw a loading frame (Render(0, 0) in SWorld::LoadMap 0x5f1990).

#ifndef PZ_IVIEWPORT_H
#define PZ_IVIEWPORT_H

#include "pzcommon.h"

struct SIBoard;   // the SWINE-shared board (3dengine/iboard.h)

namespace pz {

struct SIScene;

// The camera the world hands to the viewport each frame:
//   SetCamera(x, y, z, yaw, pitch)  vp +0x20, 0x68d370: eye position (target
//       minus direction * distance), yaw = SWorld camera +0x44, pitch =
//       -(SWorld camera +0x50) (sign flipped by the caller).
//   SetProjection(fov, near, far)   vp +0x28, 0x68cfd0: fov in radians
//       (60 deg = 0x3f860a92), only when the camera's dirty flag (+0x34) is set.
// menu.map CAM chunk: target 97.65, 95.74, height 6.22, angle -0.638,
// distance 19.08 (docs/re/MENU3D_SCOPE.md).
struct SCameraParams {
    float X, Y, Z;
    float Yaw, Pitch;
    float Fov, NearZ, FarZ;
};

struct SIViewport {
    virtual void SetPosition(int x, int y, int w, int h) = 0; // +0x00 HD 0x68d0d0 (4 arg dwords) SViewport::SetPosition
    virtual void Resize(int width, int height) = 0;         // +0x04 HD 0x68c4e0 (2 arg dwords) SViewport::Resize (ResetDevice)
    virtual void SetFullScreenMode(int w, int h, int bpp, bool vsync, int refresh, int aa, int aaq, bool p8) = 0; // +0x08 HD 0x68ca00 (8 arg dwords) SViewport::SetFullScreenMode
    virtual void SetWindowedMode(int x, int y, int w, int h) = 0; // +0x0c HD 0x68d6d0 (4 arg dwords) SViewport::SetWindowedMode
    virtual void Slot_10() = 0;                             // +0x10 HD 0x68b800 (4 arg dwords)
    virtual bool NextFullScreenMode(int* w, int* h, int p3) = 0; // +0x14 HD 0x68b8b0 (3 arg dwords)
    virtual bool PrevFullScreenMode(int* w, int* h, int p3) = 0; // +0x18 HD 0x68bcc0 (3 arg dwords)
    virtual void Slot_1C() = 0;                             // +0x1c HD 0x68d310 (1 arg dword)
    virtual void SetCamera(float x, float y, float z, float yaw, float pitch) = 0; // +0x20 HD 0x68d370 (5 arg dwords) eye position + angles; SWorld::ComputeCamera
    virtual void GetCamera(float* x, float* y, float* z, float* yaw, float* pitch) = 0; // +0x24 HD 0x68be30 (5 arg dwords) (name guessed)
    virtual void SetProjection(float fovRadians, float nearZ, float farZ) = 0; // +0x28 HD 0x68cfd0 (3 arg dwords) 1/tan(fov/2); world passes 60 deg
    virtual void Slot_2C() = 0;                             // +0x2c HD 0x68cf00 (6 arg dwords)
    virtual void Slot_30() = 0;                             // +0x30 HD 0x68ce50 (4 arg dwords)
    virtual void ScreenToRay(float* out6, int x, int y) = 0; // +0x34 HD 0x689c20 (3 arg dwords) eye (out[0..2]) and the direction to the screen point on the near plane (out[3..5]); picking
    virtual void GetSelectionPlanes(float* out16, int x1, int y1, int x2, int y2) = 0; // +0x38 HD 0x6898b0 (5 arg dwords) the 4 side planes (a, b, c, d) of the screen box's pyramid from the eye; box selection
    virtual void ProjectToScreen(const float* pos, float size, float* x, float* y, float* screenSize, float* z, int* fogAlpha) = 0; // +0x3c HD 0x68dda0 (7 arg dwords) world point -> screen (x, y, z), size scaled by 1/w; screenSize -1 behind the camera; fogAlpha = fog visibility << 24
    virtual void Slot_40() = 0;                             // +0x40 HD 0x68da50 (8 arg dwords)
    virtual void GetGroundCorners(float height, float* out12) = 0; // +0x44 HD 0x68b940 (2 arg dwords) the screen corners projected on the plane y = height (minimap view frame)
    virtual void Slot_48() = 0;                             // +0x48 HD 0x68bbf0 (tail call)
    virtual SIBoard* GetBoard() = 0;                        // +0x4c HD 0x68b730 (0 arg dwords) HD AddRefs the board; the SWINE board has no refcount
    virtual void Render(SIScene* scene, unsigned clearColor) = 0; // +0x50 HD 0x68c220 (2 arg dwords) SViewport::Render 0x68c220
    virtual int CreateSubport(int x, int y, int w, int h) = 0; // +0x54 HD 0x68ab40 (4 arg dwords) new sub viewport (mode 3) with this camera and projection; returns its index (panic text names it SelectSubport)
    virtual void DestroySubport(int index) = 0;             // +0x58 HD 0x68b0f0 (1 arg dword) deletes the sub viewport and removes it from the array
    virtual SIViewport* GetSubport(int index) = 0;          // +0x5c HD 0x68bde0 (1 arg dword) the sub viewport (panic on a bad index)
    virtual int GetSubportCount() = 0;                      // +0x60 HD 0x68b930 (0 arg dwords) +0x1fc
    virtual void Slot_64() = 0;                             // +0x64 HD 0x68b470 (1 arg dword)
    virtual void Slot_68() = 0;                             // +0x68 HD 0x68b220 (3 arg dwords)
    virtual void Slot_6C() = 0;                             // +0x6c HD 0x68b3a0 (4 arg dwords)
    virtual void Slot_70() = 0;                             // +0x70 HD 0x68b350 (4 arg dwords)
    virtual void Slot_74() = 0;                             // +0x74 HD 0x68b520 (1 arg dword)
    virtual void FrontBufferScreenshot(int p1, int p2, int p3, int p4) = 0; // +0x78 HD 0x68b5b0 (4 arg dwords) SViewport::FrontBufferScreenshot
    virtual void SetDrawBoard(bool on) = 0;                 // +0x7c HD 0x68b200 (1 arg dword) byte +0x240: Render 0x68c220 draws the board in this (sub)port (else only the cursor)
    virtual void SetDrawScene(bool on) = 0;                 // +0x80 HD 0x68b210 (1 arg dword) byte +0x241: Render 0x68c220 draws the scene in this (sub)port (else a plain clear)

protected:
    ~SIViewport() {}
};

} // namespace pz

#endif // PZ_IVIEWPORT_H
