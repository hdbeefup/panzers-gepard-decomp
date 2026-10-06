// src/3dengine/pz/pzscene_m3.cpp
// pz::SScene parts the Training Camp mission reaches (M3-E): smoke trails
// (scene +0x1e0), the skybox (+0x280..+0x29c), telephone wires (+0x238)
// and the debug line list (+0x270).
//
// HD draws the trails and wires into a Gepard dynamic vertex buffer
// (0x67f150 lock, 0x67a490 draw); the recompile builds the same vertices
// (FVF 0x142: xyz, diffuse, uv) in memory and draws them with
// DrawPrimitiveUP, as DrawGroundTrails does.

#include <d3d9.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "pzscene.h"
#include "pzviewport.h"
#include "pzgepard.h"
#include "pzterrain.h"
#include "pzmodel.h"
#include "parcel.h"
#include "mesh.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

namespace {

struct SVertex142 { float x, y, z; unsigned color; float u, v; };   // FVF 0x142, 0x18 bytes
static_assert(sizeof(SVertex142) == 0x18, "FVF 0x142");

// HD SDArray growth (0x6a0d50 and its siblings): 16, then * 6 / 5; the new
// slots zeroed. Returns the new last index.
template <class T>
int SDArrayAdd(T*& data, int& count, int& max)
{
    if (count == max) {
        int n = max < 0x10 ? 0x10 : (max * 6) / 5;
        data = (T*)realloc(data, (size_t)n * sizeof(T));
        memset(data + max, 0, (size_t)(n - max) * sizeof(T));
        max = n;
    }
    return count++;
}

// 0x6a2440: set the count, grow to exactly that, zero the whole capacity.
template <class T>
void SDArrayResize(T*& data, int& count, int& max, int n)
{
    if (count != 0 && data == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SWirePoint");
    count = n;
    if (max < n) {
        max = n;
        data = (T*)realloc(data, (size_t)n * sizeof(T));
    }
    if (data)
        memset(data, 0, (size_t)max * sizeof(T));
}

// (double)(unsigned)x, the HD int -> float conversion with the 2^32 fix-up
// table 0x7ea790.
inline double U2D(int x) { return (double)(unsigned)x; }

void DrawStrip(IDirect3DDevice9* dev, const std::vector<SVertex142>& v, int count)
{
    if (count >= 3)
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, count - 2, v.data(), sizeof(SVertex142));   // 0x67a490(5, n - 2)
}

} // namespace

// ---------------------------------------------------------------------------
// Smoke trails
// ---------------------------------------------------------------------------

// PANZERS 0x6a9850
// STrailEffect 0x6ee120 makes one per effect: two points at the start, the
// prototype's colour, scales, blend mode and its two tracks (alpha and
// width over age / Duration).
int SScene::CreateSmokeTrail(int texture, float x, float y, float z, unsigned color, float strength,
                             float fadeSpeed, float uScale, float vScale, bool additive,
                             const void* alphaTrack, const void* track2, float duration,
                             float p14, float p15)
{
    PZ_TRACE("SScene::CreateSmokeTrail (0x6a9850)");
    if (SmokeTrailVB < 0)
        SmokeTrailVB = 0x142;                     // 0x6787a0(0x142); the recompile draws with DrawPrimitiveUP
    int i = SmokeTrails.Add();                    // 0x6a1320
    SSmokeTrail& t = SmokeTrails[i];
    t.AlphaCursor = 0;
    t.WidthCursor = 0;
    t.AlphaTrack = (const STrackFloat*)alphaTrack;
    t.WidthTrack = (const STrackFloat*)track2;
    t.InvDuration = 1.0f / duration;
    t.Texture = TerrainAddRefTexture(texture);    // Gepard 0x677f20
    t.Points = new SSmokeTrailPoints();           // new 0xc {0, 0, 0}
    t.Points->Data = nullptr;
    t.Points->Count = t.Points->Max = 0;
    t.Closed = false;
    t.DrawType = additive ? 1 : 0;
    t.Color = color;
    t.Strength = strength * 256.0f;               // 0x87c7d8
    t.FadeSpeed = fadeSpeed;
    t.UScale = uScale;
    t.VScale = vScale;
    for (int k = 0; k < 2; ++k) {
        SSmokeTrailPoints* pts = SmokeTrails[i].Points;
        int n = SDArrayAdd(pts->Data, pts->Count, pts->Max);   // 0x6a0d50
        SSmokeTrailPoint& p = pts->Data[n];
        p.Pos[0] = x;
        p.Pos[1] = y;
        p.Pos[2] = z;
        p.V = 0.0f;
        p.Alpha = p14;
        p.Width = p15;
        p.Time = TimeMs;                          // +0xa4
    }
    return i;
}

// PANZERS 0x6bb680
// The head point follows the effect. Every 0.5 units from the second-last
// point (which stays where it is) the head is left behind, interpolated
// between that point and the new position (V, time, alpha, width too), and
// a new head is appended. Then the head takes the new position.
void SScene::TrackSmokeTrail(int trail, float x, float y, float z, float p5, float p6)
{
    PZ_TRACE("SScene::TrackSmokeTrail (0x6bb680)");
    if (!SmokeTrails.Valid(trail))
        return;
    SSmokeTrail& t = SmokeTrails[trail];
    SSmokeTrailPoints* pts = t.Points;
    int prev = pts->Count - 2;
    int last = pts->Count - 1;
    if (last < 0 || prev < 0)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSmokeTrailPoint", prev < 0 ? prev : last);
    float dx = x - pts->Data[prev].Pos[0];
    float dy = y - pts->Data[prev].Pos[1];
    float dz = z - pts->Data[prev].Pos[2];
    float dist = (float)sqrt((double)(dy * dy + dx * dx + dz * dz));
    for (float f = 0.5f; f < dist; f += 0.5f) {
        const SSmokeTrailPoint& a = pts->Data[prev];
        SSmokeTrailPoint& h = pts->Data[last];
        float frac = f / dist;
        h.Pos[0] = (x - a.Pos[0]) * frac + a.Pos[0];
        h.Pos[1] = (y - a.Pos[1]) * frac + a.Pos[1];
        h.Pos[2] = (z - a.Pos[2]) * frac + a.Pos[2];
        h.V = t.VScale * f + a.V;
        h.Time = (int)((float)U2D(TimeMs - a.Time) * frac + (float)U2D(a.Time));   // _ftol2 0x766810
        h.Alpha = (p5 - a.Alpha) * frac + a.Alpha;
        h.Width = (p6 - a.Width) * frac + a.Width;
        last = SDArrayAdd(pts->Data, pts->Count, pts->Max);          // 0x6a0d50
    }
    SSmokeTrailPoint& h = pts->Data[last];
    const SSmokeTrailPoint& a = pts->Data[prev];
    h.Pos[0] = x;
    h.Pos[1] = y;
    h.Pos[2] = z;
    h.V = t.VScale * dist + a.V;
    h.Alpha = p5;
    h.Time = TimeMs;
    h.Width = p6;
}

// PANZERS 0x6aa9e0
void SScene::DestroySmokeTrail(int trail)
{
    PZ_TRACE("SScene::DestroySmokeTrail (0x6aa9e0)");
    if (!SmokeTrails.Valid(trail))
        return;
    SSmokeTrail& t = SmokeTrails[trail];
    PzGepard()->ReleaseTexture(t.Texture);        // Gepard +0x48
    if (t.Points) {
        free(t.Points->Data);
        t.Points->Data = nullptr;
        t.Points->Count = t.Points->Max = 0;
        delete t.Points;                          // 0xc
        t.Points = nullptr;
    }
    SmokeTrails.Remove(trail);                    // 0x6aca00
}

// PANZERS 0x6aaad0
void SScene::DestroyAllSmokeTrails()
{
    PZ_TRACE("SScene::DestroyAllSmokeTrails (0x6aaad0)");
    for (int i = SmokeTrails.Next(-1); i >= 0; i = SmokeTrails.Next(i))
        DestroySmokeTrail(i);
}

// PANZERS 0x6a2780
void SScene::CloseSmokeTrail(int trail)
{
    PZ_TRACE("SScene::CloseSmokeTrail (0x6a2780)");
    if (SmokeTrails.Valid(trail))
        SmokeTrails[trail].Closed = true;
}

// PANZERS 0x6b7b30
// "SScene::RenderSmokeTrails". One triangle strip per trail, a camera-facing
// ribbon: per point the side vector is normalize(dir x (point - eye)) *
// (point width + width track), dir from the neighbours. Each segment adds 8
// vertices (two degenerate): prev-side, prev-side, cur-side, prev, cur,
// prev+side, cur+side, cur+side; U 0 / 0.5 / 1 across, V the point's V.
// Alpha = point alpha * alpha track (age in s / Duration) * 255, chopped
// and clamped; colour = trail colour (type 0) or grey (type 1, additive,
// black fog). A closed trail whose points all faded to 0 is destroyed.
void SScene::DrawSmokeTrails(SViewport* vp)
{
    PZ_TRACE("SScene::DrawSmokeTrails (0x6b7b30)");
    if (SmokeTrails.Count == 0)                   // +0x1f0
        return;
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    GepardSetWorldIdentity();                     // 0x680fe0
    dev->SetFVF(0x142);                           // device +0x164
    float cam[3] = { 0.0f, 0.0f, 0.0f }, yaw, pitch;
    vp->GetCamera(&cam[0], &cam[1], &cam[2], &yaw, &pitch);   // vp +0x24
    SRenderPass pass;
    pass.Init();                                  // 0x687730
    pass.Lighting = false;
    pass.SetAlphaOp(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);   // 0x688870(0, 4, 2, 1)
    std::vector<SVertex142> verts;
    for (int i = SmokeTrails.Next(-1); i >= 0; i = SmokeTrails.Next(i)) {
        SSmokeTrail& t = SmokeTrails[i];
        int n = t.Points->Count;
        if (n < 2)
            Logger.g->Panic("SScene::RenderSmokeTrails: Structure is damaged");
        verts.resize((size_t)n * 8);              // 0x67f150(+0x1f4, n << 3)
        int nv = 0;
        int alphaSum = 0;
        float prevP[3] = { 0, 0, 0 }, prevS[3] = { 0, 0, 0 }, prevV = 0.0f;
        unsigned prevColor = 0;
        for (int k = 0; k < n; ++k) {
            const SSmokeTrailPoint* pts = t.Points->Data;
            const SSmokeTrailPoint& p = pts[k];
            float age = (float)U2D(TimeMs - p.Time) * 0.001f * t.InvDuration;   // 0x87c78c
            float at = t.AlphaTrack ? t.AlphaTrack->Evaluate(age, &t.AlphaCursor) : 1.0f;   // 0x6abda0
            int a = (int)(p.Alpha * at * 255.0f);     // fistp under 0xc7f (chop)
            if (a > 0xff)
                a = 0xff;
            else if (a < 0)
                a = 0;
            float d[3];
            if (k == 0) {
                for (int c = 0; c < 3; ++c) d[c] = pts[1].Pos[c] - pts[0].Pos[c];
            } else if (k == n - 1) {
                for (int c = 0; c < 3; ++c) d[c] = p.Pos[c] - pts[k - 1].Pos[c];
            } else {
                for (int c = 0; c < 3; ++c) d[c] = pts[k + 1].Pos[c] - pts[k - 1].Pos[c];
            }
            float e[3] = { p.Pos[0] - cam[0], p.Pos[1] - cam[1], p.Pos[2] - cam[2] };
            float sx = d[1] * e[2] - d[2] * e[1];
            float sy = d[2] * e[0] - d[0] * e[2];
            float sz = d[0] * e[1] - d[1] * e[0];
            double inv = 1.0 / sqrt((double)(sx * sx + sy * sy + sz * sz));
            sx = (float)((double)sx * inv);
            sy = (float)((double)sy * inv);
            sz = (float)((double)sz * inv);
            float wt = t.WidthTrack ? t.WidthTrack->Evaluate(age, &t.WidthCursor) : 0.0f;
            alphaSum += a;
            float w = p.Width + wt;
            float S[3] = { sx * w, sy * w, sz * w };
            unsigned color = 0;
            if (t.DrawType == 0)
                color = ((unsigned)a << 24) + t.Color;
            else if (t.DrawType == 1)
                color = (unsigned)a * 0x10101u;
            if (k > 0) {
                SVertex142* o = &verts[(size_t)nv];
                o[0] = { prevP[0] - prevS[0], prevP[1] - prevS[1], prevP[2] - prevS[2], prevColor, 0.0f, prevV };
                o[1] = o[0];
                o[2] = { p.Pos[0] - S[0], p.Pos[1] - S[1], p.Pos[2] - S[2], color, 0.0f, p.V };
                o[3] = { prevP[0], prevP[1], prevP[2], prevColor, 0.5f, prevV };
                o[4] = { p.Pos[0], p.Pos[1], p.Pos[2], color, 0.5f, p.V };
                o[5] = { prevP[0] + prevS[0], prevP[1] + prevS[1], prevP[2] + prevS[2], prevColor, 1.0f, prevV };
                o[6] = { p.Pos[0] + S[0], p.Pos[1] + S[1], p.Pos[2] + S[2], color, 1.0f, p.V };
                o[7] = o[6];
                nv += 8;
            }
            memcpy(prevP, p.Pos, sizeof(prevP));
            memcpy(prevS, S, sizeof(prevS));
            prevV = p.V;
            prevColor = color;
        }
        pass.SetFogMode(t.DrawType == 1 ? 1 : 0, 0);  // 0x688aa0: black fog for additive
        pass.SetTexture(0, t.Texture, true);          // 0x688d80
        pass.SetBlendMode(t.DrawType, t.Texture);     // 0x688980
        pass.Apply();                                 // 0x687a50
        DrawStrip(dev, verts, nv);
        if (alphaSum == 0 && t.Closed)
            DestroySmokeTrail(i);                     // 0x6aa9e0
    }
}

// ---------------------------------------------------------------------------
// Skybox
// ---------------------------------------------------------------------------

// The cube (HD data 0x87c318: six faces of four FVF 0x142 vertices, a
// triangle strip each, half size 0.5, white; same order as the textures).
static const SVertex142 kSkyboxFaces[6][4] = {
    { { -0.5f, 0.5f, -0.5f, 0xffffffff, 0.0f, 0.0f }, { -0.5f, -0.5f, -0.5f, 0xffffffff, 0.0f, 1.0f },
      { 0.5f, 0.5f, -0.5f, 0xffffffff, 1.0f, 0.0f }, { 0.5f, -0.5f, -0.5f, 0xffffffff, 1.0f, 1.0f } },   // front
    { { 0.5f, 0.5f, -0.5f, 0xffffffff, 0.0f, 0.0f }, { 0.5f, -0.5f, -0.5f, 0xffffffff, 0.0f, 1.0f },
      { 0.5f, 0.5f, 0.5f, 0xffffffff, 1.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 0xffffffff, 1.0f, 1.0f } },     // right
    { { 0.5f, 0.5f, 0.5f, 0xffffffff, 0.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 0xffffffff, 0.0f, 1.0f },
      { -0.5f, 0.5f, 0.5f, 0xffffffff, 1.0f, 0.0f }, { -0.5f, -0.5f, 0.5f, 0xffffffff, 1.0f, 1.0f } },   // back
    { { -0.5f, 0.5f, 0.5f, 0xffffffff, 0.0f, 0.0f }, { -0.5f, -0.5f, 0.5f, 0xffffffff, 0.0f, 1.0f },
      { -0.5f, 0.5f, -0.5f, 0xffffffff, 1.0f, 0.0f }, { -0.5f, -0.5f, -0.5f, 0xffffffff, 1.0f, 1.0f } }, // left
    { { -0.5f, 0.5f, 0.5f, 0xffffffff, 0.0f, 0.0f }, { -0.5f, 0.5f, -0.5f, 0xffffffff, 0.0f, 1.0f },
      { 0.5f, 0.5f, 0.5f, 0xffffffff, 1.0f, 0.0f }, { 0.5f, 0.5f, -0.5f, 0xffffffff, 1.0f, 1.0f } },     // top
    { { -0.5f, -0.5f, -0.5f, 0xffffffff, 0.0f, 0.0f }, { -0.5f, -0.5f, 0.5f, 0xffffffff, 0.0f, 1.0f },
      { 0.5f, -0.5f, -0.5f, 0xffffffff, 1.0f, 0.0f }, { 0.5f, -0.5f, 0.5f, 0xffffffff, 1.0f, 1.0f } },   // bottom
};

// PANZERS 0x6a96c0
// "<file>_<face>.tga" for front, right, back, left, top, bottom (table
// 0x8de3ec), Gepard LoadTexture(name, 1, 1).
void SScene::SetSkybox(const char* file, float radius)
{
    PZ_TRACE("SScene::SetSkybox (0x6a96c0)");
    ClearSkybox();                                // vtbl +0x100
    static const char* const kFaces[6] = { "front", "right", "back", "left", "top", "bottom" };
    for (int i = 0; i < 6; ++i) {
        std::string name = file ? file : "";
        name += "_";
        name += kFaces[i];
        name += ".tga";
        SkyboxTextures[i] = PzGepard()->LoadTexture(name.c_str(), 1, true);   // Gepard +0x44
    }
    SkyboxRadius = radius;                        // +0x29c
    SkyboxOn = true;                              // +0x280
}

// PANZERS 0x6aa9a0
void SScene::ClearSkybox()
{
    PZ_TRACE("SScene::ClearSkybox (0x6aa9a0)");
    if (!SkyboxOn)
        return;
    for (int i = 0; i < 6; ++i)
        PzGepard()->ReleaseTexture(SkyboxTextures[i]);   // Gepard +0x48
    SkyboxOn = false;
}

// PANZERS 0x6b7920
// The cube scaled by the radius around the eye, unlit, culled CCW, with the
// tinted fog when the scene fog colour has alpha (fog mode 3, else off), and
// the viewport's far-plane override (+0x11c, 0x68d8b0) around it.
void SScene::DrawSkybox(SViewport* vp, unsigned fogColor)
{
    PZ_TRACE("SScene::DrawSkybox (0x6b7920)");
    if (!SkyboxOn)
        return;
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    float cam[3], yaw, pitch;
    vp->GetCamera(&cam[0], &cam[1], &cam[2], &yaw, &pitch);   // vp +0x24
    float r = SkyboxRadius;
    const float scale[12] = { r, 0, 0, 0, r, 0, 0, 0, r, 0, 0, 0 };
    const float move[12] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, cam[0], cam[1], cam[2] };
    float world[12];
    Mat34Mul(world, scale, move);                 // 0x7c5530
    GepardSetWorld(world);                        // 0x680fb0
    SRenderPass pass;
    pass.Init();                                  // 0x687730
    pass.Lighting = false;
    pass.CullMode = 3;                            // +0x1d4 (D3DCULL_CCW)
    if ((fogColor & 0xff000000) != 0)
        pass.SetFogMode(3, fogColor);             // 0x688aa0
    else
        pass.SetFogMode(2, 0);
    dev->SetFVF(0x142);                           // device +0x164
    vp->ZBiasOff = true;                          // 0x68d8b0(1)
    vp->ApplyTransforms();
    for (int i = 0; i < 6; ++i) {
        pass.SetTexture(0, SkyboxTextures[i], false);   // 0x688d80(0, tex, 0)
        pass.Apply();                                   // 0x687a50
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, kSkyboxFaces[i], sizeof(SVertex142));   // device +0x14c
    }
    vp->ZBiasOff = false;                         // 0x68d8b0(0)
    vp->ApplyTransforms();
}

// ---------------------------------------------------------------------------
// Wires
// ---------------------------------------------------------------------------

// PANZERS 0x6aa050
// Loads the wire textures on first use, stores the ends, the length and the
// two parameters, then builds the points (0x6c05d0).
int SScene::CreateWire(const float* from, const float* to, float sag, float width)
{
    PZ_TRACE("SScene::CreateWire (0x6aa050)");
    if (WireVB < 0)
        WireVB = 0x142;                           // 0x6787a0(0x142); DrawPrimitiveUP here
    if (WireDecl < 0)
        WireDecl = 0;                             // D3DXDeclaratorFromFVF(0x142) + 0x679d00: the shadow shader path is not ported
    if (WireTexture < 0)
        WireTexture = PzGepard()->LoadTexture("wire/wire_a.tga", 1, true);         // Gepard +0x44
    if (WireShadowTexture < 0)
        WireShadowTexture = PzGepard()->LoadTexture("wire/wire_shadow.tga", 1, true);
    int i = Wires.Add();                          // 0x6a1400
    SWire& w = Wires[i];
    memcpy(w.From, from, sizeof(w.From));
    memcpy(w.To, to, sizeof(w.To));
    float dx = from[0] - to[0], dy = from[1] - to[1], dz = from[2] - to[2];
    w.Length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
    w.SagParam = sag;
    w.Width = width;
    RebuildWire(i);
    return i;
}

// PANZERS 0x6aaba0
void SScene::DestroyWire(int wire)
{
    PZ_TRACE("SScene::DestroyWire (0x6aaba0)");
    if (!Wires.Valid(wire))
        return;
    SWire& w = Wires[wire];                       // 0x6aca60: frees the point array
    free(w.Points);
    w.Points = nullptr;
    w.Count = w.Max = 0;
    Wires.Remove(wire);
}

// PANZERS 0x6c0b40
// Explicit points (SDArray<float[3]>): colour 0xff, U the running length,
// no sway.
void SScene::UpdateWire(int wire, int* p2)
{
    PZ_TRACE("SScene::UpdateWire (0x6c0b40)");
    if (!Wires.Valid(wire))
        Logger.g->Panic("SScene::UpdateWire(): Invalid index specified.");
    const float* src = *(const float**)p2;
    int n = p2[1];
    SWire& w = Wires[wire];
    SDArrayResize(w.Points, w.Count, w.Max, n);   // 0x6a2440
    float dist = 0.0f;
    for (int i = 0; i < n; ++i) {
        SWirePoint& q = w.Points[i];
        memcpy(q.Pos, src + i * 3, 12);
        q.Color = 0xff;
        q.Dist = dist;
        q.Sag = 0.0f;
        if (i < n - 1) {
            float dx = src[i * 3] - src[i * 3 + 3];
            float dy = src[i * 3 + 1] - src[i * 3 + 4];
            float dz = src[i * 3 + 2] - src[i * 3 + 5];
            dist += (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        }
    }
}

// PANZERS 0x6c05d0
// (int)Length + 1 segments of the catenary: at t = 2f - 1 (f = i / segments)
// the point drops |To - From| * (cosh(sag) - cosh(sag * t)) below the
// straight line, but stays 0.01 above the terrain (0x6f4c00). Colour
// 0xffffff, U the running length, Sag = cosh(sag) - cosh(sag * t).
void SScene::RebuildWire(int wire)
{
    if (!Wires.Valid(wire))
        Logger.g->Panic("SScene::UpdateWire(): Invalid index specified.");
    SWire& w = Wires[wire];
    float d[3] = { w.To[0] - w.From[0], w.To[1] - w.From[1], w.To[2] - w.From[2] };
    float c0 = (float)cosh((double)w.SagParam);   // 0x794952 (cosh)
    int segs = (int)w.Length + 1;                 // cvttss2si
    SDArrayResize(w.Points, w.Count, w.Max, segs + 1);
    float acc = 0.0f;
    float prev[3] = { w.From[0], w.From[1], w.From[2] };
    float fsegs = (float)segs;
    for (int i = 0; i <= segs; ++i) {
        float f = (float)i / fsegs;
        float x = d[0] * f + w.From[0];
        float y = w.From[1] + d[1] * f;
        float z = w.From[2] + d[2] * f;
        float t = f * 2.0f - 1.0f;
        float len = (float)sqrt((double)(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
        float sag = c0 - (float)cosh((double)(w.SagParam * t));
        y = y - len * sag;
        if (Terrain) {
            float h = Terrain->HeightAt(x, z) + 0.01f;   // 0x7f1b48
            if (h > y)
                y = h;
        }
        SWirePoint& q = w.Points[i];
        q.Pos[0] = x;
        q.Pos[1] = y;
        q.Pos[2] = z;
        q.Color = 0xffffff;
        q.Dist = acc;
        q.Sag = c0 - (float)cosh((double)(w.SagParam * t));
        float ex = x - prev[0], ey = y - prev[1], ez = z - prev[2];
        acc = (float)sqrt((double)(ey * ey + ex * ex + ez * ez)) + acc;
        prev[0] = x;
        prev[1] = y;
        prev[2] = z;
    }
}

// PANZERS 0x6b8b10
// "SScene::RenderWires". A wire is drawn when the parcel under either end
// is visible (0x69db40), or under one of 7 points between them. Every point
// sways in the wind: w = (sin(x * 0.4 + t * 1.73) * cos(z * 0.4 + t * 1.23)
// + 1.5) * 0.8, moved by (sin a, cos a) * w * Sag with a = sin(t * 0.04) * 5
// (t = scene seconds). The ribbon faces the eye (the sun direction in the
// shadow pass), Width wide, 8 vertices per segment like the smoke trails,
// all wires in one strip. Shadow pass: wire_shadow.tga, alpha test, unlit.
// Recompile: technique 4's shadow pass (vertex shader 0x42 with the
// world-view-projection and texture-matrix constants) is not ported; with
// that technique the wires cast no shadow.
void SScene::DrawWires(SViewport* vp, bool shadowPass)
{
    PZ_TRACE("SScene::DrawWires (0x6b8b10)");
    int total = 0;
    for (int i = Wires.Next(-1); i >= 0; i = Wires.Next(i)) {
        SWire& w = Wires[i];
        w.Visible = false;
        if (w.Count != 0) {
            float x0 = w.Points[0].Pos[0], z0 = w.Points[0].Pos[2];
            float x1 = w.Points[w.Count - 1].Pos[0], z1 = w.Points[w.Count - 1].Pos[2];
            w.Visible = Terrain && (Terrain->ParcelVisibleAt(x0, z0) || Terrain->ParcelVisibleAt(x1, z1));
            if (!w.Visible && Terrain) {
                float dx = x1 - x0, dz = z1 - z0;
                for (int k = 1; !w.Visible && k < 8; ++k)
                    w.Visible = Terrain->ParcelVisibleAt((float)k * 0.125f * dx + x0, (float)k * 0.125f * dz + z0);
            }
        }
        if (w.Visible)
            total += w.Count * 8;
    }
    if (total == 0)
        return;
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    if (shadowPass && GepardOption(2) == 4)
        return;                                   // see above
    float cam[3] = { 0.0f, 0.0f, 0.0f };
    if (!shadowPass) {
        float yaw, pitch;
        vp->GetCamera(&cam[0], &cam[1], &cam[2], &yaw, &pitch);   // vp +0x24
    }
    GepardSetWorldIdentity();                     // 0x680fe0
    float secs = (float)U2D(TimeMs) * 0.001f;     // 0x87c78c
    float ang = (float)sin((double)(secs * 0.04f)) * 5.0f;   // 0x801ac8, 0x7f5a6c
    float swayX = (float)sin((double)ang);
    float swayZ = (float)cos((double)ang);
    std::vector<SVertex142> verts((size_t)total);  // 0x67f150(+0x260, total)
    int nv = 0;
    auto sway = [&](const SWirePoint& p, float phaseZ, float phaseX) {
        float s = (float)sin((double)(p.Pos[0] * 0.4f + phaseX));   // 0x7f5e20
        float c = (float)cos((double)(p.Pos[2] * 0.4f + phaseZ));
        return (s * c + 1.5f) * 0.8f;              // 0x7f1b84, 0x7f83dc
    };
    for (int i = Wires.Next(-1); i >= 0; i = Wires.Next(i)) {
        SWire& w = Wires[i];
        if (!w.Visible)
            continue;
        int n = w.Count;
        float prevC[3] = { 0, 0, 0 }, prevS[3] = { 0, 0, 0 };
        for (int k = 0; k < n; ++k) {
            const SWirePoint& p = w.Points[k];
            float phaseZ = secs * 1.23f;           // 0x801ad4
            float phaseX = secs * 1.73f;           // 0x801ae8
            float f = sway(p, phaseZ, phaseX);
            float C[3] = { swayX * f * p.Sag + p.Pos[0], p.Pos[1], swayZ * f * p.Sag + p.Pos[2] };
            float e[3];
            if (shadowPass)
                memcpy(e, SunDir, sizeof(e));     // scene +0x110
            else
                for (int c = 0; c < 3; ++c) e[c] = C[c] - cam[c];
            float d[3];
            if (k == 0 || k != n - 1) {
                if (k == 0 && n < 2)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SWirePoint", 1);
                const SWirePoint& q = w.Points[k + 1];
                float fq = sway(q, phaseZ, phaseX);
                float Q[3] = { fq * swayX * q.Sag + q.Pos[0], q.Pos[1], fq * swayZ * q.Sag + q.Pos[2] };
                const float* from = k == 0 ? C : prevC;
                for (int c = 0; c < 3; ++c) d[c] = Q[c] - from[c];
            } else {
                for (int c = 0; c < 3; ++c) d[c] = C[c] - prevC[c];
            }
            float sx = e[2] * d[1] - e[1] * d[2];
            float sy = e[0] * d[2] - e[2] * d[0];
            float sz = e[1] * d[0] - e[0] * d[1];
            double inv = 1.0 / sqrt((double)(sy * sy + sx * sx + sz * sz));
            float S[3] = { w.Width * (float)((double)sx * inv), w.Width * (float)((double)sy * inv),
                           w.Width * (float)((double)sz * inv) };
            if (k > 0) {
                const SWirePoint& a = w.Points[k - 1];
                SVertex142* o = &verts[(size_t)nv];
                o[0] = { prevC[0] - prevS[0], prevC[1] - prevS[1], prevC[2] - prevS[2], a.Color, a.Dist, 0.0f };
                o[1] = o[0];
                o[2] = { C[0] - S[0], C[1] - S[1], C[2] - S[2], p.Color, p.Dist, 0.0f };
                o[3] = { prevC[0], prevC[1], prevC[2], a.Color, a.Dist, 0.5f };
                o[4] = { C[0], C[1], C[2], p.Color, p.Dist, 0.5f };
                o[5] = { prevC[0] + prevS[0], prevC[1] + prevS[1], prevC[2] + prevS[2], a.Color, a.Dist, 1.0f };
                o[6] = { C[0] + S[0], C[1] + S[1], C[2] + S[2], p.Color, p.Dist, 1.0f };
                o[7] = o[6];
                nv += 8;
            }
            memcpy(prevC, C, sizeof(prevC));
            memcpy(prevS, S, sizeof(prevS));
        }
    }
    SRenderPass pass;
    pass.Init();                                  // 0x687730
    if (GepardOption(2) == 0 && GepardOption(0x10) != 0)
        pass.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);   // 0x6888f0(0, 5, 2, 1, 1)
    pass.CullMode = D3DCULL_NONE;                 // +0x1d4 = 1
    if (!shadowPass) {
        pass.SetTexture(0, WireTexture, true);    // 0x688d20(0, tex, 1, 1)
        pass.SetAlphaMode(2);                     // 0x6887d0: blend
    } else {
        pass.SetTexture(0, WireShadowTexture, true);
        pass.SetAlphaMode(1);                     // alpha test
        pass.Lighting = false;
    }
    pass.Apply();                                 // 0x687a50
    if (total < nv)
        Logger.g->Panic("SScene::RenderWires(): vertidx > sum_numvertices");
    dev->SetFVF(0x142);
    DrawStrip(dev, verts, nv);
}

// PANZERS 0x6a24e0
void SScene::ClearLines()
{
    PZ_TRACE("SScene::ClearLines (0x6a24e0)");
    // 0x6a2250(0) on +0x270: count 0, the capacity zeroed.
    if (LineCount != 0 && Lines == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SLine");
    LineCount = 0;
    if (Lines)
        memset(Lines, 0, (size_t)LineMax << 5);
}

} // namespace pz
