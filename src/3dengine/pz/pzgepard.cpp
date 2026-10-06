// src/3dengine/pz/pzgepard.cpp
// The Gepard facade: HD SGepard slots (vftable 0x816da8) over the SWINE
// SGepard, plus the HD SGepard members the HD 3D path calls. OWNER: agent A.
//
// Shared with the SWINE 2D path:
//   - the D3D9 device (SGepard::lpD3DDev): the HD scene draws inside the
//     SWINE frame through SGepard::PanzersScenePass (pzviewport.cpp);
//   - textures: LoadTexture/ReleaseTexture forward to the SWINE texture table,
//     so a handle from the 3D path is valid for the board and the reverse.
// Owned here: the primary viewport facade, the SPixie (HD SGepard+0x7f4),
// the model prototype heap (+0x550), the animation heap (+0x564) and the
// dynamic vertex buffer (+0x5d4).
//
// Device creation is unchanged (SWINE SGepard::Initialize). HD creates the
// device with BehaviorFlags 0x40 (HARDWARE_VERTEXPROCESSING) or 0x20
// (SOFTWARE), never D3DCREATE_FPU_PRESERVE (0x67d1c0 writes SGepard+0x458,
// 0x68ada0 passes it to CreateDevice); SWINE does the same. See
// docs/MENU3D_INTERFACES.md "FPU".

#include <d3d9.h>
#include <string.h>
#include <stdlib.h>
#include "pzgepard.h"
#include "pzviewport.h"
#include "pzscene.h"
#include "pzpixie.h"
#include "pzterrain.h"
#include "panim.h"
#include "pzmodel.h"
#include <sys/stat.h>
#include "mesh.h"
#include "gepard.h"
#include "stream.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)

namespace pz {

void ViewportScenePass(IDirect3DDevice9* dev);   // pzviewport.cpp
void ReleaseMeshVertexDecls();                   // mesh.cpp
void SetExtension(SString* s, const char* ext);  // pmodel.cpp

static SPzGepard* s_Facade = nullptr;
static SGepardHDState s_HD;

static SGepard* SwineGepard() { return static_cast<SGepard*>(::Gepard); }

SGepardHDState& HD()
{
    if (!s_HD.Device && ::Gepard) {
        IDirect3DDevice9* dev = SwineGepard()->lpD3DDev;
        if (dev) {
            s_HD.Device = dev;
            D3DCAPS9 caps;
            memset(&caps, 0, sizeof(caps));
            dev->GetDeviceCaps(&caps);
            unsigned blend = caps.MaxTextureBlendStages ? caps.MaxTextureBlendStages : 1;
            unsigned tex = caps.MaxSimultaneousTextures ? caps.MaxSimultaneousTextures : 1;
            s_HD.TransformStages = blend < 5 ? blend : 5;
            s_HD.BlendStages = blend < 5 ? blend : 5;
            s_HD.SamplerStages = tex < 5 ? tex : 5;
            // HD: PS id 2 exists on PS 1.1+ cards (0x67df80). The recompile
            // does not create the HD pixel shaders, so it takes HD's PS-less
            // paths (two-pass reflection; see SMaterial::Begin).
            s_HD.PixelShader1x = false;
            s_HD.DynVBSize = 0x1fffe0;   // 0x678730
            s_HD.DynType112 = 0;
            s_HD.DynTypeFvf[0] = 0x112;
            s_HD.DynTypeStride[0] = 0x20;
            s_HD.DynTypeCount = 1;
            // SYSTEMMEM: survives SWINE's device reset (HD uses DEFAULT).
            if (FAILED(dev->CreateVertexBuffer(s_HD.DynVBSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
                                               D3DPOOL_SYSTEMMEM, &s_HD.DynVB, nullptr)))
                Logger.g->Log(0, "SGepard::CreateDynamicVB: CreateVertexBuffer failed");
        }
    }
    return s_HD;
}

// PANZERS 0x6f33c0 (flora instance part, 0x6f3880..0x6f3a40)
// Grass instances (agent B's flora pass). HD draws only node 0's mesh
// (prototype -> Nodes[0] +0x44, mesh vtbl +4) with
//   node 0 transform (+0x14, 3x4) [x Y<->Z when FlyZ] x scale 0.005
//   (the constant 3x4 at 0x886ed0) x the instance rotation and position.
// Without the node transform and the scale the grass was drawn 200 times
// too large and covered the whole menu scene.
static void DrawFloraInstance(SScene* scene, int proto, const float world[16])
{
    SPModel* p = GepardModelPrototype(proto);
    if (!p || !HD().Device || p->NodeCount < 1 || !p->Nodes[0].Mesh)
        return;
    float m[12];
    memcpy(m, p->Nodes[0].Transform, sizeof(m));
    if (p->FlyZ) {
        static const float kFlyZ[12] = { 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 };   // 0x8df834 (Y<->Z)
        Mat34Mul(m, m, kFlyZ);
    }
    static const float kScale[12] = { 0.005f, 0, 0, 0, 0.005f, 0, 0, 0, 0.005f, 0, 0, 0 };   // 0x886ed0
    Mat34Mul(m, m, kScale);
    const float inst[12] = { world[0], world[1], world[2], world[4], world[5], world[6],
                             world[8], world[9], world[10], world[12], world[13], world[14] };
    Mat34Mul(m, m, inst);
    float w44[16];
    Mat34To44(w44, m);
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w44);
    p->Nodes[0].Mesh->Draw(scene);
}

SIGepardHD* PzGepard()
{
    if (!s_Facade) {
        s_Facade = new SPzGepard();
        SGepard::PanzersScenePass = &ViewportScenePass;
        g_FloraDraw = &DrawFloraInstance;
        if (Logger.g)
            Logger.g->Log(0, "pz: Gepard facade created (HD scene pass hooked into SGepard::RenderScene)");
    }
    return s_Facade;
}

void PzGepardShutdown()
{
    if (!s_Facade)
        return;
    SGepard::PanzersScenePass = nullptr;
    g_FloraDraw = nullptr;
    delete s_Facade;
    s_Facade = nullptr;
    ReleaseMeshVertexDecls();
    if (s_HD.DynVB)
        s_HD.DynVB->Release();
    memset(&s_HD, 0, sizeof(s_HD));
}

SPzGepard::SPzGepard()
    : RefCount(1), Primary(nullptr), Pixie(nullptr)
{
    memset(Options, 0, sizeof(Options));
}

SPzGepard::~SPzGepard()
{
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        Prototypes[i]->RefCount = 0;
        delete Prototypes[i];
    }
    Prototypes.Free();
    for (int i = Anims.Next(-1); i >= 0; i = Anims.Next(i))
        delete Anims[i];
    Anims.Free();
    if (Pixie) {
        Pixie->Release();   // the facade's own reference (HD SGepard dtor)
        Pixie = nullptr;
    }
    delete Primary;
    Primary = nullptr;
}

void SPzGepard::AddRef()
{
    PZ_TRACE("SGepard::AddRef (0x677f10)");
    ++RefCount;   // HD 0x677f10; the facade is never deleted by Release
}

void SPzGepard::Release()
{
    PZ_TRACE("SGepard::Release (0x67f640)");
    // HD 0x67f640 deletes SGepard at 0; the SWINE Gepard is released by
    // SDXWindow::OnDestroy, the facade by PzGepardShutdown.
    if (RefCount > 0)
        --RefCount;
}

SIScene* SPzGepard::CreateScene()
{
    PZ_TRACE("SGepard::CreateScene (0x678ed0)");
    // HD 0x678ed0: index = scene heap +0x4e4 alloc; new SScene(index) (0x2b0,
    // ctor 0x69faf0 logs "Scene created"); stored in the heap. The heap is
    // not kept here.
    HD();
    return new SScene(0);
}

void SPzGepard::SetOption(unsigned option, int value)
{
    PZ_TRACE("SGepard::SetOption (0x680910)");
    // HD 0x680910 also applies options (3 = shadow buffer size reloads
    // editor/shadow_buffer_*.tga ...). Stored only; the SWINE renderer has
    // no counterpart (see PzStub_ApplyGraphicsOptions).
    if (option < sizeof(Options) / sizeof(Options[0]))
        Options[option] = value;
}

// PANZERS 0x67c380
int SPzGepard::GetOption(unsigned option)
{
    PZ_TRACE("SGepard::GetOption (0x67c380)");
    return option < 0x12 ? Options[option] : 0;
}

int GepardOption(unsigned option)
{
    return s_Facade && option < 0x12 ? s_Facade->Options[option] : 0;
}

int SPzGepard::GetCap(unsigned cap)
{
    PZ_TRACE("SGepard::GetCap (0x67a7d0)");
    // HD: SGepard+0x540[cap]. Cap 0 = shadow technique, 2 on a PS 2.0 card
    // (the value optionsmenu.cpp assumes too).
    return cap == 0 ? 2 : 0;
}

// PANZERS 0x67db20
// Returns the prototype index of (file, scale[, sequence file]); loads it
// on first use (.4d -> SPModel::Load4DFile with the file's directory as the
// texture prefix; other files -> the .geo loader 0x693190, not lifted).
int SPzGepard::LoadModelPrototype(const char* file, float scale, const char* p3, int p4)
{
    PZ_TRACE("SGepard::LoadModelPrototype (0x67db20)");
    if (!file || !*file)
        return -1;
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        SPModel* p = Prototypes[i];
        const char* n = p->FileName.buf ? p->FileName.buf : "";
        if (strcmp(n, file) != 0)
            continue;
        if (p3) {
            const char* s = p->SequenceFile.buf ? p->SequenceFile.buf : "";
            if (strcmp(s, p3) != 0 || p->Scale != scale)
                continue;
        } else if (p->SequenceFile.size != 0 || p->Scale != scale) {
            continue;
        }
        p->AddRef();   // 0x68eb50
        return i;
    }
    SPModel* p = new SPModel(p4);   // 0x68e250
    // Directory of the file with a trailing '/' (0x5e4e00 + 0x52c580).
    char dir[512];
    strncpy(dir, file, sizeof(dir) - 2);
    dir[sizeof(dir) - 2] = 0;
    int slash = -1;
    for (int i = 0; dir[i] && dir[i + 1]; ++i)
        if (dir[i] == '/' || dir[i] == '\\')
            slash = i;
    if (slash >= 0)
        dir[slash] = 0;
    else
        strcpy(dir, ".");
    strcat(dir, "/");
    bool ok = false;
    if (strstr(file, "4D") || strstr(file, "4d") || strstr(file, "4DA") || strstr(file, "4da")) {   // 0x8183a8..
        try {
            ok = p->Load4DFile(dir, file, scale);
        } catch (const char* e) {
            Logger.g->Log(0, "SPModel::Load4DFile: %s: %s", file, e);
            ok = false;
        }
    } else {
        STUB_LOG("SPModel::LoadGeoFile (0x693190)");
    }
    if (!ok) {
        p->RefCount = 0;
        delete p;
        return -1;
    }
    p->FileName = file;
    p->SequenceFile = p3;
    p->Scale = scale;
    int i = Prototypes.Add();   // 0x677bc0
    Prototypes[i] = p;
    return i;
}

// PANZERS 0x67f6d0
void SPzGepard::ReleaseModelPrototype(int proto)
{
    PZ_TRACE("SPzGepard::ReleaseModelPrototype (0x67f6d0)");
    if (Prototypes.Valid(proto) && Prototypes[proto]->RefCount > 0)
        --Prototypes[proto]->RefCount;
}

// PANZERS 0x678210
// Deletes the prototypes nobody references any more.
int SPzGepard::PurgeModelPrototypes()
{
    PZ_TRACE("SPzGepard::PurgeModelPrototypes (0x678210)");
    int n = 0;
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        if (Prototypes[i]->RefCount == 0) {
            delete Prototypes[i];
            Prototypes.Remove(i);
            ++n;
        }
    }
    return n;
}

SPModel* GepardModelPrototype(int index)   // 0x6778a0
{
    if (!s_Facade || !s_Facade->Prototypes.Valid(index))
        return nullptr;
    return s_Facade->Prototypes[index];
}

// PANZERS 0x67d920
int GepardLoadAnim(const char* file)
{
    SPzGepard* g = static_cast<SPzGepard*>(PzGepard());
    if (!file || !*file)
        return -1;
    for (int i = g->Anims.Next(-1); i >= 0; i = g->Anims.Next(i)) {
        SPAnim* a = g->Anims[i];
        if (a->Name.buf && !strcmp(a->Name.buf, file)) {
            ++a->RefCount;
            return i;
        }
    }
    SPAnim* a = new SPAnim();
    bool ok;
    try {
        ok = a->LoadAnimFile(file);
    } catch (const char* e) {
        Logger.g->Log(0, "SPAnim::LoadAnimFile: %s: %s", file, e);
        ok = false;
    }
    if (!ok) {
        delete a;
        return -1;
    }
    a->Name = file;
    int i = g->Anims.Add();
    g->Anims[i] = a;
    return i;
}

// PANZERS 0x67a760
SPAnim* GepardAnim(int index)
{
    if (!s_Facade || !s_Facade->Anims.Valid(index))
        return nullptr;
    return s_Facade->Anims[index];
}

void GepardReleaseAnim(int index)
{
    if (s_Facade && s_Facade->Anims.Valid(index) && s_Facade->Anims[index]->RefCount > 0)
        --s_Facade->Anims[index]->RefCount;
}

SIViewport* SPzGepard::GetViewport(int index)
{
    PZ_TRACE("SGepard::GetViewport (0x67cb00)");
    if (index != 0) {
        if (Logger.g)
            Logger.g->Log(0, "pz: GetViewport(%d): only the primary viewport exists", index);
        return nullptr;
    }
    if (!Primary)
        Primary = new SViewport();
    return Primary;
}

// PANZERS 0x67ea30 (name handling)
// HD: with the alpha flag the extension becomes ".tga" (0x597390 appends it
// when the name has none); a missing file is retried as ".dxt", then logged
// "Texture is not found" and -1 returned. The decoding and the _hq DXT cache
// stay SWINE's (docs/ENGINE_DIFF.md section 2).
int SPzGepard::LoadTexture(const char* file, int mipmap, bool alpha)
{
    PZ_TRACE("SGepard::LoadTexture (0x67ea30)");
    if (!::Gepard || !file || !*file)
        return -1;
    SString name;
    name = file;
    if (alpha)
        SetExtension(&name, "tga");
    struct _stat st;
    if (FileSystem.Stat(name.buf, &st) != 0) {
        SString dxt;
        dxt = name;
        SetExtension(&dxt, "dxt");
        bool found = FileSystem.Stat(dxt.buf, &st) == 0;
        delete[] dxt.buf;
        if (!found) {
            Logger.g->Log(0, "SGepard::LoadTexture: %s: Texture is not found", name.buf);
            delete[] name.buf;
            return -1;
        }
    }
    bool hasDot = false;
    for (int i = 0; i < name.size; ++i) {
        if (name.buf[i] == '/' || name.buf[i] == '\\')
            hasDot = false;
        else if (name.buf[i] == '.')
            hasDot = true;
    }
    if (!hasDot)
        SetExtension(&name, "tga");   // the SWINE loader needs an extension to swap
    int h = ::Gepard->LoadTexture(name.buf, mipmap != 0, alpha);
    delete[] name.buf;
    return h;
}

void SPzGepard::ReleaseTexture(int texture)
{
    PZ_TRACE("SGepard::ReleaseTexture (0x67f740)");
    if (::Gepard && texture >= 0)
        ::Gepard->ReleaseTexture(texture, false);
}

// PANZERS 0x67ca50
void SPzGepard::GetTextureSize(int texture, int* width, int* height)
{
    PZ_TRACE("SPzGepard::GetTextureSize (0x67ca50)");
    SGepard* g = SwineGepard();
    if (g && texture >= 0 && texture < g->Textures.size && g->Textures.array[texture].use == 0x7FFFFFFF) {
        *width = (int)g->Textures.array[texture].data.Width;
        *height = (int)g->Textures.array[texture].data.Height;
        return;
    }
    *width = 0;
    *height = 0;
}

// PANZERS 0x67c9e0
// HD texture record +0x20: 0 opaque, 1 1-bit alpha (test), 2 alpha blend.
int GepardGetTextureAlpha(int texture)
{
    SGepard* g = SwineGepard();
    if (!g || texture < 0)
        return 0;
    return g->GetTextureAlpha(texture);
}

// PANZERS 0x680c90
// Binds a texture handle; the sampler filters come from the texture's
// filter flags (HD record +0x24, from options 8/9 via 0x680bf0: bit 0-1
// min/mag 0 point, 1 linear, 2 anisotropic; bit 2 linear mip).
void GepardSetTexture(unsigned stage, int texture)
{
    IDirect3DDevice9* dev = HD().Device;
    SGepard* g = SwineGepard();
    if (!dev)
        return;
    if (!g || texture < 0 || texture >= g->Textures.size || g->Textures.array[texture].use != 0x7FFFFFFF) {
        dev->SetTexture(stage, nullptr);
        return;
    }
    dev->SetTexture(stage, g->Textures.array[texture].data.lpTexture);
    unsigned flags = (GepardOption(9) != 0) + 1 | (GepardOption(8) != 0 ? 4u : 0u) | 8u;   // 0x680bf0(idx, 8)
    switch (flags & 3) {
    case 0:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        break;
    case 1:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        break;
    case 2:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
        dev->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, 4);
        break;
    }
    dev->SetSamplerState(stage, D3DSAMP_MIPFILTER, (flags & 4) ? D3DTEXF_LINEAR : D3DTEXF_POINT);
}

IDirect3DVertexDeclaration9* MeshVertexDecl(int i);   // mesh.cpp

// PANZERS 0x680e80
void GepardSetVertexFormat(int decl, unsigned fvf)
{
    IDirect3DDevice9* dev = HD().Device;
    if (decl == -1) {
        dev->SetFVF(fvf);
        return;
    }
    IDirect3DVertexDeclaration9* d = MeshVertexDecl(decl);
    if (!d) {
        Logger.g->Panic("SGepard::SetVertexDeclaration: invalid index");
        return;
    }
    dev->SetVertexDeclaration(d);
}

// PANZERS 0x680fb0
void GepardSetWorld(const float* m)
{
    float w[16];
    Mat34To44(w, m);
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w);
}

// PANZERS 0x680fe0
void GepardSetWorldIdentity()
{
    float w[16];
    memset(w, 0, sizeof(w));
    w[0] = w[5] = w[10] = w[15] = 1.0f;
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w);
}

// PANZERS 0x680f40
void GepardSetVertexShader(int id)
{
    if (id != 0) {
        Logger.g->Panic("SGepard::SetVertexShader: invalid index");   // HD never creates ids 1..0x40
        return;
    }
    HD().Device->SetVertexShader(nullptr);
}

// PANZERS 0x680ac0
void GepardSetPixelShader(int id)
{
    if (id != 0) {
        Logger.g->Panic("SGepard::SetPixelShader: invalid index");
        return;
    }
    HD().Device->SetPixelShader(nullptr);
}

// PANZERS 0x680510 (colour 0x5aa900: clamped float4 -> ARGB)
void GepardSetAmbient(const float* c)
{
    auto ch = [](float v) -> unsigned {
        if (1.0f <= v) return 0xff;
        if (v < 0.0f) return 0;
        return (unsigned)lrintf(v * 255.0f);
    };
    unsigned a = ch(c[3]), r = ch(c[0]);
    unsigned g = c[3] < 0.0f ? 0 : ch(c[1]);
    unsigned b = c[3] < 0.0f ? 0 : ch(c[2]);
    HD().Device->SetRenderState(D3DRS_AMBIENT, b | ((a << 8 | r) << 8 | g) << 8);
}

// PANZERS 0x67f150
void* GepardLockDynamicVB(int type, int count)
{
    SGepardHDState& hd = HD();
    if (type < 0 || type >= hd.DynTypeCount) {
        Logger.g->Panic("SGepard::LockDynamicVB: Bad index (%d).", type);
        return nullptr;
    }
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::LockDynamicVB: Already locked.");
        return nullptr;
    }
    unsigned size = hd.DynTypeStride[type] * count;
    if (hd.DynVBSize < size) {
        Logger.g->Panic("SGepard::LockDynamicVB: VertexBuffer is too small.");
        return nullptr;
    }
    if (hd.DynVBSize < hd.DynVBOffset + size)
        hd.DynVBOffset = 0;
    DWORD lockFlags = hd.DynVBOffset ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
    void* p = nullptr;
    if (!hd.DynVB || FAILED(hd.DynVB->Lock(hd.DynVBOffset, size, &p, lockFlags))) {
        Logger.g->Panic("SGepard::LockDynamicVB: IDirect3DVertexBuffer8::Lock");
        return nullptr;
    }
    hd.DynVBType = type;
    hd.DynVBLocked = true;
    return p;
}

// PANZERS 0x681440
void GepardUnlockDynamicVB()
{
    SGepardHDState& hd = HD();
    if (!hd.DynVBLocked) {
        Logger.g->Panic("SGepard::UnlockDynamicVB: Not locked.");
        return;
    }
    hd.DynVB->Unlock();
    hd.DynVBLocked = false;
}

// PANZERS 0x67a650
void GepardDrawDynamicVB(int prim, int minIndex, int numVerts, int startIndex, int primCount, int withFvf)
{
    SGepardHDState& hd = HD();
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::DrawIndexedDynamicVB: Buffer is locked.");
        return;
    }
    if (primCount == 0)
        return;
    IDirect3DDevice9* dev = hd.Device;
    dev->SetStreamSource(0, hd.DynVB, hd.DynVBOffset, hd.DynTypeStride[hd.DynVBType]);
    if (withFvf)
        dev->SetFVF(hd.DynTypeFvf[hd.DynVBType]);
    dev->DrawIndexedPrimitive((D3DPRIMITIVETYPE)prim, 0, minIndex, numVerts, startIndex, primCount);
    hd.PolyCount += primCount;
    hd.VertexCount += numVerts;
}

// PANZERS 0x677fc0
void GepardAdvanceDynamicVB(int count)
{
    SGepardHDState& hd = HD();
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::AdvanceDynamicVB: Buffer is locked.");
        return;
    }
    hd.DynVBOffset += hd.DynTypeStride[hd.DynVBType] * count;
}

SIPixie* GepardPixie()
{
    SPzGepard* g = static_cast<SPzGepard*>(PzGepard());
    if (!g->Pixie) {
        g->GetPixie();
        g->Pixie->Release();
    }
    return g->Pixie;
}

SIPixie* SPzGepard::GetPixie()
{
    PZ_TRACE("SGepard::GetPixie (0x67c3b0)");
    // HD: SPixie is created in SGepard::Initialize 0x67d1c0 (new 0x7c); the
    // facade creates it on first use. 0x67c3b0 AddRefs before returning.
    if (!Pixie)
        Pixie = new SPixie();
    Pixie->AddRef();
    return Pixie;
}

// ---- generated slot stubs (HD vtable order) ----

// HD SPzGepard vtbl +0x08 -> 0x67c3d0 (1 arg dword)
int* SPzGepard::Slot_08_Stats(int* out)
{
    STUB_LOG("SPzGepard::Slot_08_Stats (0x67c3d0)");
    PZ_TRACE("SPzGepard::Slot_08_Stats (0x67c3d0)");
    (void)out;
    return nullptr;
}

// HD SPzGepard vtbl +0x1c -> 0x680540 (1 arg dword)
void SPzGepard::SetBrightness(int level)
{
    STUB_LOG("SPzGepard::SetBrightness (0x680540)");
    PZ_TRACE("SPzGepard::SetBrightness (0x680540)");
    (void)level;
}

// HD SPzGepard vtbl +0x2c -> 0x67bf90 (2 arg dwords)
void SPzGepard::Slot_2C()
{
    STUB_LOG("SPzGepard::Slot_2C (0x67bf90)");
    PZ_TRACE("SPzGepard::Slot_2C (0x67bf90)");
}

// HD SPzGepard vtbl +0x30 -> 0x67c0a0 (2 arg dwords)
void SPzGepard::Slot_30()
{
    STUB_LOG("SPzGepard::Slot_30 (0x67c0a0)");
    PZ_TRACE("SPzGepard::Slot_30 (0x67c0a0)");
}

// HD SPzGepard vtbl +0x34 -> 0x67c170 (2 arg dwords)
void SPzGepard::Slot_34()
{
    STUB_LOG("SPzGepard::Slot_34 (0x67c170)");
    PZ_TRACE("SPzGepard::Slot_34 (0x67c170)");
}

// HD SPzGepard vtbl +0x38 -> 0x681010 (3 arg dwords)
void SPzGepard::SwitchModelPrototypeNodes(int proto, int node1, int node2)
{
    STUB_LOG("SPzGepard::SwitchModelPrototypeNodes (0x681010)");
    PZ_TRACE("SPzGepard::SwitchModelPrototypeNodes (0x681010)");
    (void)proto; (void)node1; (void)node2;
}

// HD SPzGepard vtbl +0x40 -> 0x67a3f0 (1 arg dword)
void SPzGepard::Slot_40()
{
    STUB_LOG("SPzGepard::Slot_40 (0x67a3f0)");
    PZ_TRACE("SPzGepard::Slot_40 (0x67a3f0)");
}

// HD SPzGepard vtbl +0x4c -> 0x681540 (4 arg dwords)
void SPzGepard::UpdateTexture(int texture, int p2, int p3, void* data)
{
    STUB_LOG("SPzGepard::UpdateTexture (0x681540)");
    PZ_TRACE("SPzGepard::UpdateTexture (0x681540)");
    (void)texture; (void)p2; (void)p3; (void)data;
}

// HD SPzGepard vtbl +0x54 -> 0x67fb50 (0 arg dwords)
void SPzGepard::Slot_54()
{
    STUB_LOG("SPzGepard::Slot_54 (0x67fb50)");
    PZ_TRACE("SPzGepard::Slot_54 (0x67fb50)");
}

// HD SPzGepard vtbl +0x58 -> 0x680b30 (1 arg dword)
void SPzGepard::SetCachePath(const char* path)
{
    STUB_LOG("SPzGepard::SetCachePath (0x680b30)");
    PZ_TRACE("SPzGepard::SetCachePath (0x680b30)");
    (void)path;
}

} // namespace pz
