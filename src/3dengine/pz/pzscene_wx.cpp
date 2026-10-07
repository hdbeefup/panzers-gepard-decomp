// src/3dengine/pz/pzscene_wx.cpp
// SScene's two outline passes of RenderViewport 0x6acaf0 (agent M6-WX):
// the rectangle outlines of heap +0x1cc (0x6b04d0) and the line list
// +0x270 (0x6acf20). Both are line overlays in the Gepard dynamic VB +0x610
// (FVF 0x42, XYZ | DIFFUSE). Nothing on the campaign path fills either list
// (the scene slots that add to them are not called by the game code lifted
// so far), so in practice both draw nothing; they run every frame as in HD.

#include <d3d9.h>
#include <string.h>
#include "pzscene.h"
#include "pzviewport.h"
#include "pzgepard.h"
#include "pzterrain.h"
#include "mesh.h"
#include "effectrender.h"

namespace pz {

namespace {

const unsigned kLineFvf = 0x42;   // Gepard +0x610: XYZ | DIFFUSE (16 bytes)

// STerrain 0x6f4bc0: the height of grid vertex (x, z), 0 outside.
float TerrainVertexHeight(const STerrain* t, int x, int z)
{
    if (x < 0 || x > t->Width || z < 0 || z > t->Height)
        return 0.0f;
    return t->Heights[t->Stride * z + x];
}

void PutVertex(float*& v, float x, float y, float z, unsigned color)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    memcpy(&v[3], &color, 4);
    v += 4;
}

} // namespace

// PANZERS 0x6b04d0
// Every rectangle (x0, z0) - (x1, z1) of heap +0x1cc as one line strip
// along the terrain vertices: the z0 edge, the x1 edge, the z1 edge back and
// the x0 edge back, in the rectangle's colour, with the viewport depth bias
// 1/4096 (0x68d890) while drawing.
void SScene::DrawOutlines(SViewport* vp)
{
    if (Outlines.Count == 0)
        return;
    SRenderPass pass;
    pass.Init();                                                  // 0x687730
    pass.Apply();                                                 // 0x687a50
    vp->ZBias = 2.44140625e-4f;                                   // 0x68d890(0x39800000)
    for (int i = 0; i < Outlines.Size; ++i) {
        if (!Outlines.Valid(i))
            continue;
        const SOutlineRect& r = Outlines[i];
        int x0 = r.X0, z0 = r.Z0, x1 = r.X1, z1 = r.Z1;
        int n = (((z1 - z0) - x0) + x1) * 2;                      // line strip segments
        float* v = EffectLockDynamicVB(kLineFvf, n + 1);          // 0x67f150(+0x610, n + 1)
        for (int x = x0; x < x1; ++x)
            PutVertex(v, (float)x, Terrain ? TerrainVertexHeight(Terrain, x, z0) : 0.0f, (float)z0, r.Color);
        for (int z = z0; z < z1; ++z)
            PutVertex(v, (float)x1, Terrain ? TerrainVertexHeight(Terrain, x1, z) : 0.0f, (float)z, r.Color);
        for (int x = x1; x0 < x; --x)
            PutVertex(v, (float)x, Terrain ? TerrainVertexHeight(Terrain, x, z1) : 0.0f, (float)z1, r.Color);
        for (int z = z1; z0 <= z; --z)
            PutVertex(v, (float)x0, Terrain ? TerrainVertexHeight(Terrain, x0, z) : 0.0f, (float)z, r.Color);
        EffectUnlockDynamicVB();                                  // 0x681440
        EffectDrawDynamicVB(D3DPT_LINESTRIP, n);                  // 0x67a490(3, n)
        EffectAdvanceDynamicVB(n + 1);                            // 0x677fc0
    }
    vp->ZBias = 0.0f;                                             // 0x68d890(0)
}

// PANZERS 0x6acf20
// The +0x270 lines (0x20 each: from, to, colour) as a line list, 0x800
// lines per batch, world = identity.
void SScene::DrawLines(SViewport* vp)
{
    (void)vp;
    if (LineCount == 0)
        return;
    SRenderPass pass;
    pass.Init();                                                  // 0x687730
    pass.Apply();                                                 // 0x687a50
    GepardSetWorldIdentity();                                     // 0x680fe0
    const unsigned char* lines = (const unsigned char*)Lines;
    for (int start = 0; start < LineCount; start += 0x800) {
        int n = LineCount - start;
        if (n > 0x800)
            n = 0x800;
        float* v = EffectLockDynamicVB(kLineFvf, n * 2);          // 0x67f150(+0x610, n * 2)
        for (int k = 0; k < n; ++k) {
            const float* e = (const float*)(lines + (size_t)(start + k) * 0x20);
            unsigned color;
            memcpy(&color, &e[6], 4);
            PutVertex(v, e[0], e[1], e[2], color);
            PutVertex(v, e[3], e[4], e[5], color);
        }
        EffectUnlockDynamicVB();                                  // 0x681440
        EffectDrawDynamicVB(D3DPT_LINELIST, n);                   // 0x67a490(2, n)
        EffectAdvanceDynamicVB(n * 2);                            // 0x677fc0
    }
}

} // namespace pz
