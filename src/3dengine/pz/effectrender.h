// src/3dengine/pz/effectrender.h
// Draw helpers for the effects, on the SWINE D3D9 device. OWNER: agent C.
//
// HD draws particles through SGepard services that the HD Gepard facade
// does not provide yet:
//   dynamic vertex buffers  CreateDynamicVBFormat 0x6787a0, LockDynamicVB
//                           0x67f150, UnlockDynamicVB 0x681440, DrawDynamicVB
//                           0x67a490, AdvanceDynamicVB 0x677fc0
//   SMaterial               a 0x300-byte render-state block (ctor 0x687730,
//                           defaults 0x6883a0, apply 0x687a50)
//   SViewport +0x3c         0x68dda0 world -> screen projection of a sphere
//                           (screen centre, screen radius, z, fog)
// They are re-implemented here with the same inputs and outputs. The
// vertex buffer is a CPU array drawn with DrawPrimitiveUP (HD locks a
// D3DPOOL_DEFAULT buffer with DISCARD/NOOVERWRITE); the projection reads
// the view/projection/viewport the scene left on the device instead of the
// HD SViewport matrices (+0x170 world->screen, +0x90 view, +0x1f0 size
// factor). When agent A lifts SViewport +0x3c, EffectProject should call it.

#ifndef PZ_EFFECTRENDER_H
#define PZ_EFFECTRENDER_H

namespace pz {

struct SIViewport;

void* EffectDevice();                            // IDirect3DDevice9* (HD SGepard+0x478)

// Dynamic vertex buffers. The handle is the FVF (0x1c4 = XYZRHW|DIFFUSE|
// SPECULAR|TEX1, 32 bytes; 0x142 = XYZ|DIFFUSE|TEX1, 24 bytes).
int   EffectCreateDynamicVB(unsigned fvf);       // 0x6787a0
float* EffectLockDynamicVB(int vb, int vertices);// 0x67f150
void  EffectUnlockDynamicVB();                   // 0x681440
void  EffectDrawDynamicVB(int primType, int primCount); // 0x67a490 (D3DPT_* , count)
void  EffectAdvanceDynamicVB(int vertices);      // 0x677fc0

// The part of HD SMaterial the particles set.
struct SEffectMaterial {
    int  Texture;          // +0x1d8 stage 0 (Gepard texture index, -1 none)
    int  Address;          // +0x1dc/+0x1e0 3 = clamp (SetTexture flag 0), 1 = wrap
    int  ColorOp;          // +0x220 stage 0 COLOROP (4 modulate, 5 modulate2x)
    bool AlphaBlend;       // +0x2c5
    int  SrcBlend;         // +0x2c8
    int  DestBlend;        // +0x2cc
    bool AlphaTest;        // +0x2d0
    int  AlphaRef;         // +0x2d1
    int  AlphaFunc;        // +0x2d4
    bool ZEnable;          // +0x2d8
    int  ZFunc;            // +0x2dc
    bool ZWrite;           // +0x2e0
    int  CullMode;         // +0x1d4 (1 none)
    int  FogMode;          // +0x300 0 scene fog, 1 black fog (additive)

    void Reset();                                // 0x687730 / 0x6883a0 defaults
    void SetTexture(int tex, bool wrap);         // 0x688d80
    void SetBlend(int mode, int tex);            // 0x688980 (0 from the texture alpha, 1 add)
    void SetBlendFromAlphaType(int alphaType);   // 0x6887d0
    void Apply();                                // 0x687a50
};

// Per frame: SPixie::Render brackets its draws with these. Begin sets the
// world transform to identity (0x680fe0) and reads the camera from the
// device; End restores what the effects changed.
void EffectBeginRender(SIViewport* vp);
void EffectEndRender();

// SViewport +0x3c 0x68dda0. Returns false (and *ssize = -1) behind the
// camera. *fog is the specular colour with the fog factor in alpha.
bool EffectProject(SIViewport* vp, const float* pos, float size,
                   float* sx, float* sy, float* ssize, float* z, unsigned* fog);

// Gepard texture alpha kind (0 opaque, 1 one-bit, 2 alpha), HD 0x67c9e0.
int EffectTextureAlphaType(int tex);

// HD SGepard sine/cosine tables (+0x7f8 / +0x804, 720 entries, degrees).
float EffectSinDeg(int deg);
float EffectCosDeg(int deg);

} // namespace pz

#endif // PZ_EFFECTRENDER_H
