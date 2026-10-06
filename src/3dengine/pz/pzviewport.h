// src/3dengine/pz/pzviewport.h
// pz::SViewport: the primary HD viewport as a facade over the SWINE device.
// OWNER: agent A. Skeleton from P0.
//
// Not HD-sized on purpose: HD SViewport (0x244) owns the device, the swap
// chain and the board; in the recompile the SWINE SGepard owns all three
// (one D3D9 device for 2D and 3D). This class keeps the HD slot order and
// the camera/projection state the scene needs.

#ifndef PZ_PZVIEWPORT_H
#define PZ_PZVIEWPORT_H

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
    void Slot_34() override;
    void Slot_38() override;
    void Slot_3C() override;
    void Slot_40() override;
    void Slot_44() override;
    void Slot_48() override;
    SIBoard* GetBoard() override;
    void Render(SIScene* scene, unsigned clearColor) override;
    int Slot_54_SelectSubport(int p1, int p2, int p3, int p4) override;
    void Slot_58_SelectSubport(int index) override;
    void Slot_5C() override;
    void Slot_60() override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Slot_70() override;
    void Slot_74() override;
    void FrontBufferScreenshot(int p1, int p2, int p3, int p4) override;
    void Slot_7C() override;
    void Slot_80() override;

    SCameraParams Camera;        // HD keeps these in the viewport (+0x7c.., +0xcc fov, +0xd0 near, +0xd4 far)
    bool          ProjectionSet; // SetProjection was called at least once
};

} // namespace pz

#endif // PZ_PZVIEWPORT_H
