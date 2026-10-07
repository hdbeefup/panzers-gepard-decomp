// src/3dengine/pz/pzmodel.cpp
// pz::SModel: HD SModel (ctor 0x6d4ae0, Update 0x6dba80, node transforms
// 0x6dc7b0, Render 0x6d8830) and its slots. OWNER: agent A.
//
// Matrices are HD 3x4 row-vector matrices (rows x, y, z, translation);
// "a * b" applies a first (HD 0x7c5530: thiscall a, push out, push b).

#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "pzmodel.h"
#include "hdmath.h"
#include "pzscene.h"
#include "pzviewport.h"
#include "pzgepard.h"
#include "pzterrain.h"
#include "ipixie.h"
#include "mesh.h"
#include "tracks.h"
#include "logger.h"
#include "stub_log.h"
#include "../../world/blockmaprefresh.h"   // SBlockBitmap (HD 0x661a50 layout)

namespace pz {

// ---------------------------------------------------------------------------
// matrix helpers
// ---------------------------------------------------------------------------

// PANZERS 0x7c5530
void Mat34Mul(float* o, const float* a, const float* b)
{
    float r[12];
    r[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    r[1] = b[1] * a[0] + b[4] * a[1] + b[7] * a[2];
    r[2] = b[2] * a[0] + b[5] * a[1] + b[8] * a[2];
    r[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    r[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    r[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
    r[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    r[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    r[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
    r[9] = a[9] * b[0] + a[10] * b[3] + a[11] * b[6] + b[9];
    r[10] = a[9] * b[1] + a[10] * b[4] + a[11] * b[7] + b[10];
    r[11] = a[9] * b[2] + a[10] * b[5] + a[11] * b[8] + b[11];
    memcpy(o, r, sizeof(r));
}

// PANZERS 0x7c59c0 (determinant out-parameter not used by the callers here)
void Mat34Inverse(float* o, const float* m)
{
    float det = (m[4] * m[8] - m[5] * m[7]) * m[0] - (m[3] * m[8] - m[6] * m[5]) * m[1] +
                (m[3] * m[7] - m[6] * m[4]) * m[2];
    if (det == 0.0f) {
        memset(o, 0, 48);
        o[0] = o[4] = o[8] = 1.0f;   // 0x6d4a80
        return;
    }
    float inv = (float)(1.0 / (double)det);
    float r[12];
    r[0] = (m[4] * m[8] - m[5] * m[7]) * inv;
    r[1] = -((m[1] * m[8] - m[2] * m[7]) * inv);
    r[2] = (m[5] * m[1] - m[4] * m[2]) * inv;
    r[3] = -((m[3] * m[8] - m[6] * m[5]) * inv);
    r[4] = (m[0] * m[8] - m[6] * m[2]) * inv;
    r[5] = -((m[0] * m[5] - m[3] * m[2]) * inv);
    r[6] = (m[3] * m[7] - m[6] * m[4]) * inv;
    r[7] = -((m[0] * m[7] - m[6] * m[1]) * inv);
    r[8] = (m[0] * m[4] - m[3] * m[1]) * inv;
    float a = m[11] * m[7] - m[10] * m[8];
    float b = m[11] * m[6] - m[9] * m[8];
    float c = m[10] * m[6] - m[9] * m[7];
    r[9] = -(((a * m[3] - b * m[4]) + c * m[5]) * inv);
    r[10] = ((a * m[0] - b * m[1]) + c * m[2]) * inv;
    r[11] = -((((m[11] * m[4] - m[10] * m[5]) * m[0] - (m[11] * m[3] - m[9] * m[5]) * m[1]) +
               (m[10] * m[3] - m[9] * m[4]) * m[2]) * inv);
    memcpy(o, r, sizeof(r));
}

// PANZERS 0x6d7a20
// Pose {pos, scale, quat} -> 3x4 matrix (rotation * scale, translation).
void PoseToMatrix(float* o, const float* pos, float s, const float* q)
{
    float x = q[0], y = q[1], z = q[2], w = q[3];
    float y2 = y + y, z2 = z + z, xx2 = x * (x + x), wx2 = w * (x + x);
    o[0] = (float)((1.0 - (double)(z * z2 + y * y2)) * (double)s);
    o[1] = (w * z2 + x * y2) * s;
    o[3] = (x * y2 - w * z2) * s;
    o[2] = (x * z2 - w * y2) * s;
    o[6] = (w * y2 + x * z2) * s;
    o[4] = (float)((1.0 - (double)(z * z2 + xx2)) * (double)s);
    o[5] = (wx2 + y * z2) * s;
    o[7] = (y * z2 - wx2) * s;
    o[8] = (float)((1.0 - (double)(y * y2 + xx2)) * (double)s);
    o[9] = pos[0];
    o[10] = pos[1];
    o[11] = pos[2];
}

// PANZERS 0x676f60
void Mat34To44(float* o, const float* m)
{
    o[0] = m[0]; o[1] = m[1]; o[2] = m[2]; o[3] = 0.0f;
    o[4] = m[3]; o[5] = m[4]; o[6] = m[5]; o[7] = 0.0f;
    o[8] = m[6]; o[9] = m[7]; o[10] = m[8]; o[11] = 0.0f;
    o[12] = m[9]; o[13] = m[10]; o[14] = m[11]; o[15] = 1.0f;
}

static void Identity34(float* m)
{
    memset(m, 0, 48);
    m[0] = m[4] = m[8] = 1.0f;
}

// Quaternion (x,y,z,w) -> 3x4 with translation, the inline form 0x6dc7b0 uses.
static void QuatTransToMatrix(float* o, const float* q, const float* t)
{
    float x = q[0], y = q[1], z = q[2], w = q[3];
    float y2 = y + y, z2 = z + z, xx2 = (x + x) * x, xw2 = (x + x) * w;
    o[0] = (float)(1.0 - (double)(z2 * z + y2 * y));
    o[1] = z2 * w + y2 * x;
    o[3] = y2 * x - z2 * w;
    o[2] = z2 * x - y2 * w;
    o[4] = (float)(1.0 - (double)(z2 * z + xx2));
    o[5] = xw2 + z2 * y;
    o[7] = z2 * y - xw2;
    o[6] = y2 * w + z2 * x;
    o[8] = (float)(1.0 - (double)(y2 * y + xx2));
    o[9] = t[0];
    o[10] = t[1];
    o[11] = t[2];
}

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

// PANZERS 0x6d4ae0
SModel::SModel(SScene* scene, SPModel* proto, SPModel* proto2, bool flag, int index)
{
    memset(static_cast<void*>(&AttachParent), 0, sizeof(SModel) - offsetof(SModel, AttachParent));
    AttachParent = nullptr;
    AttachNode = -1;
    RefCount = 1;
    Identity34(World);
    Identity34(PrevWorld);
    Pos[0] = Pos[1] = Pos[2] = 0.0f;   // 0x6d5000 pose {0, 1, quat identity}
    Scl = 1.0f;
    Quat[0] = Quat[1] = Quat[2] = 0.0f;
    Quat[3] = 1.0f;
    PrevPos[0] = PrevPos[1] = PrevPos[2] = 0.0f;
    PrevScl = 1.0f;
    PrevQuat[0] = PrevQuat[1] = PrevQuat[2] = 0.0f;
    PrevQuat[3] = 1.0f;
    Scene = scene;
    Device = HD().Device;
    Initialize(proto, proto2);
    // HD: pixie +0x28 attaches the looped effect of every ':' node
    // (Effect >= 0, EffectOnce false) to this model; the pixie slot is
    // agent C's (unnamed), so the attachment is left out here.
    Scl = Proto->Scale;
    PrevScl = Proto->Scale;
    Frame = 0;
    Visible = true;
    FadeState = 0;
    MainHeap = !flag;
    Index = index;
    Flag = flag;
    FadeAlpha = 1.0f;
    FadeStart = 0.0f;
    NodeFadeState = 0;
    NodeFadeAlpha = 1.0f;
    NodeFadeStart = 0.0f;
    NodeFadeNode = -1;
    NodeFadeNode2 = -1;
    Highlight = 0;
    Anim.Seq = 0;
    Anim.Time = 0.0f;
    Anim.PrevSeq = -1;
    Anim.PrevTime = 0.0f;
    Anim.Blend = 1.0f;
    AnimDelta = 0.0f;
    Flags = 0;
    ExtraFrame = 0;
    Drawn = true;
    DrawnDeferred = true;
    FlagBit8 = true;
    ShadowDecal2 = -1;
    ShadowDecal = -1;
    SwayPhase = SwayX = SwayZ = 0.0f;
    ColorOverride = false;
    Color = 0;
    Color2Override = false;
    Color2 = 0;
}

// PANZERS 0x6d82b0 (SModel::Initialize)
void SModel::Initialize(SPModel* proto, SPModel* proto2)
{
    Proto = proto;
    Proto2 = proto2;
    proto->AddRef();
    Param = proto->Param;
    int n = proto->NodeCount;
    Nodes = (SModelNode*)operator new(sizeof(SModelNode) * (n ? n : 1));
    memset(Nodes, 0, sizeof(SModelNode) * (n ? n : 1));
    for (int i = 0; i < n; ++i) {
        SModelNode& m = Nodes[i];
        m.Scale = 1.0f;
        m.Quat[3] = 1.0f;
        m.PrevScale = 1.0f;
        m.PrevQuat[3] = 1.0f;
        Identity34(m.World);
        m.Visible = true;
        m.EffVisible = true;
        m.Light = -1;
        m.Effect = -1;
    }
    if (proto2) {
        if (proto->NodeCount != proto2->NodeCount)
            Logger.g->Panic("SModel::Initialize: Incompatible low-poly model (node count doesn't match)");
        if (proto->SequenceCount != proto2->SequenceCount)
            Logger.g->Panic("SModel::Initialize: Incompatible low-poly model (sequence count doesn't match)");
        if (proto->FrameSequences != proto2->FrameSequences)
            Logger.g->Panic("SModel::Initialize: Incompatible low-poly model (sequence type doesn't match)");
        for (int i = 0; i < proto->SequenceCount; ++i)
            if (proto->FrameSequences && proto->Sequences[i].FrameCount != proto2->Sequences[i].FrameCount)
                Logger.g->Panic("SModel::Initialize: Incompatible low-poly model (frame count doesn't match)");
    }
    Dirty = true;
    PrevDirty = true;
}

// PANZERS 0x6d9c20
// Swaps the prototype (SScene::ReplaceModel 0x6ba810): new node instances
// (0x6d82b0); every old node whose name the new prototype has passes on its
// local transform, visibility, texture animation, light, effect and attached
// children; the others let their children go (parent 0, node -1, +0x08) and
// drop their light (scene +0x38) and effect (pixie +0x34). Then the old
// prototype is released (0x693f40), the old nodes freed and the second
// shadow decal destroyed (terrain +0x64).
void SModel::SetPrototype(SPModel* proto, SPModel* proto2)
{
    SModelNode* old = Nodes;
    SPModel* oldProto = Proto;
    Initialize(proto, proto2);
    for (int i = 0; i < oldProto->NodeCount; ++i) {
        SModelNode& o = old[i];
        const char* name = oldProto->Nodes[i].Name.buf ? oldProto->Nodes[i].Name.buf : "";
        int j = 0;
        for (; j < Proto->NodeCount; ++j) {
            const char* nn = Proto->Nodes[j].Name.buf ? Proto->Nodes[j].Name.buf : "";
            if (!_stricmp(nn, name))
                break;
        }
        if (j < Proto->NodeCount) {
            SModelNode& n = Nodes[j];
            memcpy(n.Pos, o.Pos, 0x40);                          // +0x60..+0x9f
            n.Visible = o.Visible;
            n.EffVisible = true;
            n.TexAnimType = o.TexAnimType;
            memcpy(n.TexAnim, o.TexAnim, sizeof(n.TexAnim));
            n.Light = o.Light;
            n.Effect = o.Effect;
            if (n.AttachedMax < o.AttachedCount) {
                n.AttachedMax = o.AttachedCount;
                n.Attached = (SIAttachable**)realloc(n.Attached, o.AttachedCount * sizeof(SIAttachable*));
            }
            n.AttachedCount = o.AttachedCount;
            for (int k = 0; k < o.AttachedCount; ++k) {
                n.Attached[k] = o.Attached[k];
                SAttachable* c = static_cast<SAttachable*>(o.Attached[k]);
                c->AttachParent = this;
                c->AttachNode = j;
            }
        } else {
            for (int k = 0; k < o.AttachedCount; ++k) {
                SAttachable* c = static_cast<SAttachable*>(o.Attached[k]);
                c->AttachParent = nullptr;
                c->AttachNode = -1;
                c->Attach_08();
            }
            if (o.Light >= 0 && Scene)
                Scene->DestroyLight(o.Light);                    // scene +0x38
            if (o.Effect >= 0 && GepardPixie())
                GepardPixie()->StopEffect(o.Effect);             // pixie +0x34
        }
    }
    --oldProto->RefCount;                                        // 0x693f40
    for (int i = 0; i < oldProto->NodeCount; ++i)
        free(old[i].Attached);                                   // node dtor 0x6d5300
    operator delete(old);
    if (ShadowDecal2 >= 0) {
        if (Scene && Scene->Terrain)
            Scene->Terrain->DestroyEffectDecal(ShadowDecal2);    // scene +0x1c8, terrain +0x64
        ShadowDecal2 = -1;
    }
}

SModel::~SModel()
{
    if (Scene)
        Scene->RemoveModel(this);
    for (int i = 0; Proto && i < Proto->NodeCount; ++i) {
        // PANZERS 0x6d50c0: the children of the node are let go (parent 0,
        // node -1) and told to stop (+0x08; an effect set stops its births).
        for (int k = 0; k < Nodes[i].AttachedCount; ++k) {
            SAttachable* c = static_cast<SAttachable*>(Nodes[i].Attached[k]);
            c->AttachParent = nullptr;
            c->AttachNode = -1;
            c->Attach_08();
        }
        if (Nodes[i].Light >= 0 && Scene)
            Scene->DestroyLight(Nodes[i].Light);
        free(Nodes[i].Attached);
    }
    operator delete(Nodes);
    if (Proto) {
        --Proto->RefCount;   // released by PurgeModelPrototypes
    }
    // The SAttachable base dtor (0x6d5710) leaves the parent's node.
}

// PANZERS 0x6d5710
SAttachable::~SAttachable()
{
    if (AttachParent)
        static_cast<SModel*>(AttachParent)->DetachChild(AttachNode, this);   // 0x6d7340
}

// PANZERS 0x6d5900
void SModel::AddRef()
{
    PZ_TRACE("SModel::AddRef (0x6d5900)");
    ++RefCount;
}

// PANZERS 0x6d8790
void SModel::Release()
{
    PZ_TRACE("SModel::Release (0x6d8790)");
    if (--RefCount == 0)
        delete static_cast<SIAttachable*>(this);
}

// PANZERS 0x6d7e90
void SModel::Slot_08() { PZ_TRACE("SModel::Slot_08 (0x6d7e90)"); }
// PANZERS 0x6d7ea0
void SModel::Slot_0C() { PZ_TRACE("SModel::Slot_0C (0x6d7ea0)"); }

// PANZERS 0x6d7eb0
void SModel::GetPosition(float* xyz)
{
    xyz[0] = Pos[0];
    xyz[1] = Pos[1];
    xyz[2] = Pos[2];
}

// PANZERS 0x6d7950
void SModel::GetRenderPosition(float* xyz)
{
    if (Flags & 1) {
        float t = (float)Scene->Interpolation;
        xyz[0] = (PrevPos[0] - Pos[0]) * t + Pos[0];
        xyz[1] = (PrevPos[1] - Pos[1]) * t + Pos[1];
        xyz[2] = (PrevPos[2] - Pos[2]) * t + Pos[2];
        return;
    }
    xyz[0] = Pos[0];
    xyz[1] = Pos[1];
    xyz[2] = Pos[2];
}

// PANZERS 0x6da8d0
void SModel::SetPosition(float x, float y, float z)
{
    PZ_TRACE("SModel::SetPosition (0x6da8d0)");
    Pos[0] = x;
    Pos[1] = y;
    Pos[2] = z;
    Dirty = true;
    PrevDirty = true;
    if (MainHeap == 1)
        Scene->ModelsMoved();   // 0x6bbbc0
}

// PANZERS 0x6daab0
// Yaw "angle" about Y, tilted by (tiltX, tiltZ): the tilt axis is
// (-tiltX, 0, tiltZ)/len and its angle atan(len).
void SModel::SetRotation(float angle, float tiltX, float tiltZ)
{
    PZ_TRACE("SModel::SetRotation (0x6daab0)");
    float q[4];
    float l2 = tiltX * tiltX + tiltZ * tiltZ;
    if (1e-05 <= (double)l2) {
        float len = (float)sqrt((double)l2);
        double h = (double)angle * 0.5;
        float s = (float)HdSin(h);
        float s0 = s * 0.0f;
        float c = (float)HdCos(h);
        float ax = -(tiltX / len);
        double h2 = (double)HdFastAtan(len) * 0.5;
        float ts = (float)HdSin(h2);
        float bz = (tiltZ / len) * ts;
        float by = ts * 0.0f;
        float bx = ax * ts;
        float tc = (float)HdCos(h2);
        q[0] = (bz * c + s0 * tc + by * s0) - bx * s;
        q[1] = (s * tc + by * c + bx * s0) - bz * s0;
        q[2] = (bx * c + s0 * tc + bz * s) - by * s0;
        q[3] = ((tc * c - bz * s0) - by * s) - bx * s0;
    } else {
        double h = (double)angle * 0.5;
        q[1] = (float)HdSin(h);
        q[2] = q[1] * 0.0f;
        q[3] = (float)HdCos(h);
        q[0] = q[2];
    }
    Dirty = true;
    PrevDirty = true;
    memcpy(Quat, q, 16);
}

// PANZERS 0x6daa60
void SModel::SetRotationYawPitchRoll(float pitch, float yaw, float roll)
{
    D3DXQuaternionRotationYawPitchRoll((D3DXQUATERNION*)Quat, yaw, pitch, roll);
    Dirty = true;
    PrevDirty = true;
}

// PANZERS 0x6da9e0
void SModel::RotateAxis(float x, float y, float z, float angle)
{
    D3DXVECTOR3 axis(x, y, z);
    D3DXQUATERNION r;
    D3DXQuaternionRotationAxis(&r, &axis, angle);
    D3DXQuaternionMultiply((D3DXQUATERNION*)Quat, (D3DXQUATERNION*)Quat, &r);
    D3DXQuaternionNormalize((D3DXQUATERNION*)Quat, (D3DXQUATERNION*)Quat);
    Dirty = true;
    PrevDirty = true;
}

// PANZERS 0x6dad20
void SModel::SetScale(float scale)
{
    Scl = scale;
    Dirty = true;
    PrevDirty = true;
}

// PANZERS 0x6d7ee0
float SModel::GetScale()
{
    return Scl;
}

// PANZERS 0x6dafd0
// HD SetVisible(show, fade): +0xd4 visible; with fade, a 1 s alpha fade
// (FadeState/FadeAlpha, finished in UpdateFade 0x6dc4f0).
void SModel::SetVisible(bool show, bool fade)
{
    PZ_TRACE("SModel::SetVisible (0x6dafd0)");
    if (!fade) {
        if (Visible != show) {
            Visible = show;
            if (MainHeap == 1)
                Scene->ModelsMoved();
            if (!Visible && ShadowDecal2 >= 0) {
                // terrain decal removal (terrain +0x64, agent B)
                ShadowDecal2 = -1;
            }
            // attached objects get +0xc(show)
            for (int i = 0; i < Proto->NodeCount; ++i)
                for (int k = 0; k < Nodes[i].AttachedCount; ++k)
                    Nodes[i].Attached[k]->Attach_0C(show);
        }
        FadeState = 0;
        FadeAlpha = (float)(unsigned char)Visible;
        return;
    }
    bool cur = Visible && FadeState != -1;
    if (cur == show)
        return;
    UpdateFade();
    float now = Scene->Seconds;
    if (!show) {
        if (!Visible)
            return;
        if (FadeState == 1) {
            FadeState = -1;
            FadeStart = (now + FadeAlpha) - 1.0f;
            return;
        }
        if (FadeState == 0) {
            ShadowDecal2 = -1;   // 0x6da0c0
            FadeState = -1;
            FadeAlpha = 1.0f;
            FadeStart = now;
        }
        return;
    }
    if (!Visible) {
        Visible = true;
        if (MainHeap == 1)
            Scene->ModelsMoved();
        for (int i = 0; i < Proto->NodeCount; ++i)
            for (int k = 0; k < Nodes[i].AttachedCount; ++k)
                Nodes[i].Attached[k]->Attach_0C(show);
        FadeState = 1;
        FadeAlpha = 0.0f;
        FadeStart = now;
        return;
    }
    if (FadeState == -1) {
        FadeState = 1;
        FadeStart = now - FadeAlpha;
    }
}

// PANZERS 0x6d86d0
bool SModel::GetVisible()
{
    return Visible;
}

// PANZERS 0x6db2a0
// Fades a node (a building roof) out or in over one second; node2 is shown
// while the node fades.
void SModel::SetNodeFade(bool show, int node, int node2)
{
    if (node < 0 || node >= Proto->NodeCount)
        return;
    bool cur = Nodes[node].Visible && NodeFadeState != -1;
    if (cur == show)
        return;
    UpdateFade();                                                 // 0x6dc4f0
    NodeFadeNode = node;
    NodeFadeNode2 = node2;
    float now = Scene->Seconds;                                   // scene +0xb0
    if (!show) {
        if (!Nodes[node].Visible)
            return;
        SetNodeVisible(node2, true);                              // +0x60
        if (NodeFadeState == 1) {
            NodeFadeState = -1;
            NodeFadeStart = (now + NodeFadeAlpha) - 1.0f;
            return;
        }
        if (NodeFadeState != 0)
            return;
        NodeFadeState = -1;
        NodeFadeAlpha = 1.0f;
    } else {
        if (Nodes[node].Visible) {
            if (NodeFadeState != -1)
                return;
            NodeFadeState = 1;
            NodeFadeStart = now - NodeFadeAlpha;
            return;
        }
        SetNodeVisible(node, true);
        SModelNode& fn = Nodes[NodeFadeNode];
        for (int i = 0; i < fn.AttachedCount; ++i)
            fn.Attached[i]->Attach_0C(show);                      // attachment +0x0c(show)
        for (int j = 0; j < Proto->NodeCount; ++j) {
            if (Proto->Nodes[j].Parent != NodeFadeNode)
                continue;
            SModelNode& cn = Nodes[j];
            for (int i = 0; i < cn.AttachedCount; ++i)
                cn.Attached[i]->Attach_0C(show);
        }
        NodeFadeState = 1;
        NodeFadeAlpha = 0.0f;
    }
    NodeFadeStart = now;
}

// PANZERS 0x6da0f0
void SModel::StoreInterpolationState()
{
    PZ_TRACE("SModel::StoreInterpolationState (0x6da0f0)");
    unsigned f = Flags;
    if (f & 1) {
        memcpy(PrevPos, Pos, 12);
        PrevScl = Scl;
        memcpy(PrevQuat, Quat, 16);
    }
    if (f & 4) {
        Anim = Advance(Anim, AnimDelta);
        AnimDelta = 0.0f;
    }
    if (f & 2) {
        for (int i = 0; i < Proto->NodeCount; ++i) {
            SModelNode& m = Nodes[i];
            memcpy(m.PrevPos, m.Pos, 12);
            m.PrevScale = m.Scale;
            memcpy(m.PrevQuat, m.Quat, 16);
            if (m.TexAnimType != 0) {
                m.TexAnim[1] = m.TexAnim[0];
                m.TexAnim[3] = m.TexAnim[2];
                m.TexAnim[5] = m.TexAnim[4];
            }
        }
    }
}

// PANZERS 0x6d7b90
int SModel::FindNode(const char* name)
{
    PZ_TRACE("SModel::FindNode (0x6d7b90)");
    for (int i = 0; i < Proto->NodeCount; ++i) {
        const char* n = Proto->Nodes[i].Name.buf ? Proto->Nodes[i].Name.buf : "";
        if (!_stricmp(n, name))
            return i;
    }
    return -1;
}

void SModel::Slot_44() { STUB_LOG("SModel::Slot_44 (0x6d7c20)"); }
// PANZERS 0x6da4d0
// Node user transform: position (in model units), yaw about y and a tilt
// towards (tiltX, tiltZ) by the fast atan 0x661440 of its length.
void SModel::SetNodeTilt(int node, float x, float y, float z, float yaw, float tiltX, float tiltZ)
{
    if (node < 0 || node >= Proto->NodeCount)
        return;
    SModelNode& n = Nodes[node];
    float inv = 1.0f / Scl;
    n.Pos[0] = inv * x;
    n.Pos[1] = inv * y;
    n.Pos[2] = inv * z;
    float q[4];
    double len = (double)(tiltX * tiltX + tiltZ * tiltZ);
    if (1e-05 <= len) {
        len = sqrt(len);
        double h = (double)yaw * 0.5;
        float sA = (float)HdSin(h);
        float f9 = sA * 0.0f;
        float cA = (float)HdCos(h);
        float nz = -tiltZ;
        double t = (double)HdFastAtan((float)len) * 0.5;
        float sT = (float)HdSin(t);
        float f4 = (float)((double)nz / len) * sT;
        float f5 = (float)((double)tiltX / len) * sT;
        float f10 = sT * 0.0f;
        float cT = (float)HdCos(t);
        q[0] = (f4 * cA + f9 * cT + f5 * sA) - f10 * f9;
        q[1] = (f5 * cA + f9 * cT + f10 * f9) - f4 * sA;
        q[3] = ((cT * cA - f4 * f9) - f5 * f9) - f10 * sA;
        q[2] = (sA * cT + f10 * cA + f4 * f9) - f5 * f9;
    } else {
        double h = (double)yaw * 0.5;
        float sA = (float)HdSin(h);
        q[0] = sA * 0.0f;
        q[1] = q[0];
        q[2] = sA;
        q[3] = (float)HdCos(h);
    }
    memcpy(n.Quat, q, sizeof(q));
    Dirty = true;                                                 // +0xc8 = 0x101
    PrevDirty = true;
}

// PANZERS 0x6da330
// Node user transform: position and two rotations (a about z, then b about
// x); the wheels of the running gear.
void SModel::SetNodeRotation(int node, float x, float y, float z, float a, float b)
{
    if (node < 0 || node >= Proto->NodeCount)
        return;
    SModelNode& n = Nodes[node];
    float inv = 1.0f / Scl;
    n.Pos[0] = inv * x;
    n.Pos[1] = inv * y;
    n.Pos[2] = inv * z;
    double ha = (double)a * 0.5;
    float sA = (float)HdSin(ha);
    float f6 = sA * 0.0f;
    float cA = (float)HdCos(ha);
    double hb = (double)b * 0.5;
    float sB = (float)HdSin(hb);
    float f8 = sB * 0.0f;
    float cB = (float)HdCos(hb);
    float f9 = f8 * f6;
    n.Quat[0] = (sB * cA + f6 * cB + f8 * sA) - f9;
    n.Quat[1] = (f8 * cA + f6 * cB + f9) - sB * sA;
    n.Quat[2] = (sA * cB + f8 * cA + sB * f6) - f9;
    n.Quat[3] = ((cB * cA - sB * f6) - f9) - f8 * sA;
    Dirty = true;
    PrevDirty = true;
}
// PANZERS 0x6d7d70
void SModel::GetNodePositionAxis(int node, float* pos, float* axisY)
{
    float m[12];
    GetNodeMatrix(m, node);                                       // +0x58
    pos[0] = m[9];
    pos[1] = m[10];
    pos[2] = m[11];
    axisY[0] = m[3];
    axisY[1] = m[4];
    axisY[2] = m[5];
}

// PANZERS 0x6d7d30
void SModel::GetNodePosition(int node, float* pos)
{
    float m[12];
    GetNodeMatrix(m, node);                                       // +0x58
    pos[0] = m[9];
    pos[1] = m[10];
    pos[2] = m[11];
}

// PANZERS 0x6d7c60
// The logic-pose node matrix (node +0x30), recomputed from the current pose
// on demand (the same set GetWorldBounds uses).
void SModel::GetNodeMatrix(float* m34, int node)
{
    if (node < 0 || node >= Proto->NodeCount) {
        static const float kIdentity[12] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
        memcpy(m34, kIdentity, 48);
        return;
    }
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    memcpy(m34, Nodes[node].PrevWorld, 48);
}
void SModel::Slot_5C() { STUB_LOG("SModel::Slot_5C (0x6d7dd0)"); }

// PANZERS 0x6da8a0
void SModel::SetNodeVisible(int node, bool visible)
{
    PZ_TRACE("SModel::SetNodeVisible (0x6da8a0)");
    if (node >= 0 && node < Proto->NodeCount)
        Nodes[node].Visible = visible;
}

// PANZERS 0x6da810
void SModel::SetNodeTexRotation(int node, float u, float v, float angle)
{
    if (node < 0 || node >= Proto->NodeCount)
        return;
    SModelNode& n = Nodes[node];
    n.TexAnim[0] = u;
    n.TexAnim[2] = v;
    n.TexAnim[4] = angle;
    if (n.TexAnimType != 2) {
        n.TexAnimType = 2;
        n.TexAnim[1] = u;
        n.TexAnim[3] = v;
        n.TexAnim[5] = angle;
    }
}

// PANZERS 0x6da7a0
void SModel::SetNodeTexScroll(int node, float u, float v)
{
    if (node < 0 || node >= Proto->NodeCount)
        return;
    SModelNode& n = Nodes[node];
    n.TexAnim[0] = u;
    n.TexAnim[2] = v;
    if (n.TexAnimType != 1) {
        n.TexAnimType = 1;
        n.TexAnim[1] = u;
        n.TexAnim[3] = v;
    }
}

// PANZERS 0x6da200
void SModel::PlaySequence(const char* name, bool blend)
{
    PZ_TRACE("SModel::PlaySequence (0x6da200)");
    int s = Proto->FindSequence(name);   // 0x6d7f70
    if (s < 0)
        return;
    if (!blend) {
        Anim.PrevSeq = -1;
    } else if (Anim.PrevSeq < 0 || 0.5f < Anim.Blend) {
        Anim.PrevSeq = Anim.Seq;
        Anim.PrevTime = Anim.Time;
    }
    Anim.Seq = s;
    Anim.Time = 0.0f;
    AnimDelta = 0.0f;
    Anim = Advance(Anim, 0.0f);
}

// PANZERS 0x6da960
void SModel::AdvanceAnimation(float seconds)
{
    PZ_TRACE("SModel::AdvanceAnimation (0x6da960)");
    if (!Proto->Sequences)
        return;
    if (Flags & 4) {
        AnimDelta = seconds + AnimDelta;
        return;
    }
    Anim = Advance(Anim, seconds);
}

// PANZERS 0x6da920
void SModel::AdvanceAnimationByDistance(float distance)
{
    AdvanceAnimation(distance / (Proto->Sequences[Anim.Seq].Speed * Proto->Scale));
}

// PANZERS 0x6d78c0 (HD indexes the playing sequence without a check)
int SModel::GetSequenceType()
{
    return Proto->Sequences[Anim.Seq].Type;
}

void SModel::Slot_7C() { STUB_LOG("SModel::Slot_7C (0x6d7850)"); }

// PANZERS 0x6d7f00
float SModel::GetSequenceLengthAt(int seq)
{
    if (seq >= 0 && seq < Proto->SequenceCount)
        return Proto->Sequences[seq].Length;
    return 0.0f;
}

// PANZERS 0x6d7f30
float SModel::GetSequenceLength(const char* name)
{
    int i = Proto->FindSequence(name);                            // 0x6d7f70
    if (i >= 0)
        return Proto->Sequences[i].Length;
    return 0.0f;
}
// PANZERS 0x6d8010
float SModel::GetSequenceBlendTime(const char* name)
{
    int i = Proto->FindSequence(name);                            // 0x6d7f70
    if (i >= 0)
        return Proto->Sequences[i].BlendTime;
    return 0.0f;
}
void SModel::Slot_8C() { STUB_LOG("SModel::Slot_8C (0x6d7e30)"); }
void SModel::Slot_90() { STUB_LOG("SModel::Slot_90 (0x6d8050)"); }

// PANZERS 0x6da2e0
void SModel::SetFlags(unsigned flags)
{
    PZ_TRACE("SModel::SetFlags (0x6da2e0)");
    if (Proto->SequenceCount == 0)
        flags &= ~4u;
    Flags = flags;
    StoreInterpolationState();   // vtbl +0x3c
}

void SModel::Slot_98() { STUB_LOG("SModel::Slot_98 (0x6d7910)"); }
void SModel::Slot_9C() { STUB_LOG("SModel::Slot_9C (0x6dad50)"); }
void SModel::Slot_A0() { STUB_LOG("SModel::Slot_A0 (0x6d6ce0)"); }

// PANZERS 0x6d61c0
// The node bitmap of BuildNodeBlockBitmap with the model put at pos /
// angle / tilt for the call (no interpolation); the pose (+0x88..+0xc4)
// and the flags are restored afterwards.
SBlockBitmap* SModel::BuildNodeBlockBitmapAt(int cellsPerUnit, const char* node, const float* pos,
                                             float angle, float tiltX, float tiltZ)
{
    float savePos[3], saveScl, saveQuat[4], savePrevPos[3], savePrevScl, savePrevQuat[4];
    memcpy(savePos, Pos, sizeof(savePos));
    saveScl = Scl;
    memcpy(saveQuat, Quat, sizeof(saveQuat));
    memcpy(savePrevPos, PrevPos, sizeof(savePrevPos));
    savePrevScl = PrevScl;
    memcpy(savePrevQuat, PrevQuat, sizeof(savePrevQuat));
    unsigned saveFlags = Flags;
    Flags = saveFlags & ~7u;
    PrevDirty = true;
    SetPosition(pos[0], pos[1], pos[2]);                          // +0x18
    SetRotation(angle, tiltX, tiltZ);                             // +0x1c
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    SBlockBitmap* bm = BuildNodeBlockBitmap(cellsPerUnit, node);  // +0xa8
    Flags = saveFlags;
    memcpy(Pos, savePos, sizeof(savePos));
    Scl = saveScl;
    memcpy(Quat, saveQuat, sizeof(saveQuat));
    memcpy(PrevPos, savePrevPos, sizeof(savePrevPos));
    PrevScl = savePrevScl;
    memcpy(PrevQuat, savePrevQuat, sizeof(savePrevQuat));
    return bm;
}

// PANZERS 0x661a50
// SBlockBitmap ctor (the caller does operator new(0x1c)): w x h cells at
// (0, 0), one bit per cell, rows of (w + 7) >> 3 zeroed bytes.
static SBlockBitmap* NewBlockBitmap(int w, int h)
{
    SBlockBitmap* bm = new SBlockBitmap;
    bm->W = w;
    bm->Stride = (w + 7) >> 3;
    bm->H = h;
    bm->Z = 0;
    bm->X = 0;
    bm->_14 = bm->Stride * h;
    bm->Bits = new unsigned char[bm->_14];                        // 0x766b87
    memset(bm->Bits, 0, bm->_14);
    return bm;
}

// PANZERS 0x6ce9a0
// SMesh::LockIndexBuffer: the index data at +0x48.
static void LockIndexBuffer(SMesh* m)
{
    m->IndexBuffer->Lock(0, 0, (void**)&m->Indices, 0);
}

// PANZERS 0x6ce790
// SMesh::GetFaces: a new[] copy of the mesh's faces as an index list (3 per
// face); strips are unrolled with the odd faces' winding swapped.
static void GetFaces(SMesh* m, int* count, unsigned short** out)
{
    if (m->IndexCount == 0)
        Logger.g->Panic("SMesh::GetFaces: No index buffer");
    if (m->Indices == nullptr)
        Logger.g->Panic("SMesh::GetFaces: Index buffer is not locked");
    if (m->MaterialCount == 0) {
        *count = (int)((unsigned)m->IndexCount / 3);
        *out = new unsigned short[m->IndexCount];
        memcpy(*out, m->Indices, (size_t)m->IndexCount * 2);
        return;
    }
    *count = 0;
    for (unsigned i = 0; i < m->MaterialCount; ++i)
        *count += m->Materials[i].PrimCount;
    *out = new unsigned short[(size_t)*count * 3];
    int src = 0;
    int dst = 0;
    for (unsigned i = 0; i < m->MaterialCount; ++i) {
        const SMaterial& mat = m->Materials[i];
        if (mat.PrimType == 4) {                                  // D3DPT_TRIANGLELIST
            memcpy(*out + dst, m->Indices + src, (size_t)mat.PrimCount * 6);
            src += mat.PrimCount * 3;
            dst += mat.PrimCount * 3;
            continue;
        }
        unsigned short a = m->Indices[src];
        unsigned short b = m->Indices[src + 1];
        src += 2;
        for (int k = 0; k < mat.PrimCount; ++k) {
            unsigned short c = m->Indices[src];
            ++src;
            if (k & 1) {
                (*out)[dst] = b;
                (*out)[dst + 1] = a;
            } else {
                (*out)[dst] = a;
                (*out)[dst + 1] = b;
            }
            (*out)[dst + 2] = c;
            dst += 3;
            a = b;
            b = c;
        }
    }
}

// PANZERS 0x6d5ca0
// The node's mesh in the logic pose (node +0x30 matrix), projected on XZ and
// rasterised at cellsPerUnit cells per unit: a cell is set when its centre is
// strictly inside a face (all three edge functions > 0). The bitmap spans
// floor(min * cells) .. ceil(max * cells) of the projected vertices.
SBlockBitmap* SModel::BuildNodeBlockBitmap(int cellsPerUnit, const char* name)
{
    int node = FindNode(name);                                    // +0x40
    if (node < 0)
        return nullptr;
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    float minX = 10000.0f;                                        // 0x7fd710
    float minZ = 10000.0f;
    float m[12];
    memcpy(m, Nodes[node].PrevWorld, 48);
    float maxX = -10000.0f;                                       // 0x7f5aa8
    float maxZ = -10000.0f;
    if (node >= Proto->NodeCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPModelNode", node);
    SMesh* mesh = Proto->Nodes[node].Mesh;                        // proto node +0x44
    int count = mesh->VertexCount;                                // 0x6ce990
    float* pts = new float[(size_t)count * 2]();                  // (x, z) per vertex
    mesh->Lock();                                                 // mesh +0x30
    for (int i = 0; i < count; ++i) {
        const float* v = (const float*)(mesh->Vertices + mesh->OffPosition + mesh->Stride * i);
        float x = v[0] * m[0];
        x = x + v[1] * m[3];
        x = x + v[2] * m[6];
        x = x + m[9];
        float z = v[0] * m[2];
        z = z + v[1] * m[5];
        z = z + v[2] * m[8];
        z = z + m[11];
        pts[i * 2] = x;
        pts[i * 2 + 1] = z;
        if (minX > x)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (minZ > z)
            minZ = z;
        if (z > maxZ)
            maxZ = z;
    }
    mesh->Unlock();                                               // mesh +0x34
    // x87 frndint under the control words 0x47f (down) / 0x87f (up) of the
    // SSE products, stored as floats: exact floor / ceil.
    float cells = (float)cellsPerUnit;
    float x0 = floorf(cells * minX);
    float x1 = ceilf(cells * maxX);
    float z0 = floorf(cells * minZ);
    float z1 = ceilf(cells * maxZ);
    int w = (int)(x1 - x0);                                       // cvttss2si
    int h = (int)(z1 - z0);
    float inv = 1.0f / cells;                                     // 0x7f1b58
    SBlockBitmap* bm = NewBlockBitmap(w, h);                      // new(0x1c), 0x661a50
    LockIndexBuffer(mesh);                                        // 0x6ce9a0
    int faces;
    unsigned short* idx;
    GetFaces(mesh, &faces, &idx);                                 // 0x6ce790
    for (int cz = 0; cz < h; ++cz) {
        float pz = (((float)cz + z0) + 0.5f) * inv;               // 0x7f453c
        for (int cx = 0; cx < w; ++cx) {
            float px = (((float)cx + x0) + 0.5f) * inv;
            for (int f = 0; f < faces; ++f) {
                const float* p0 = pts + idx[f * 3] * 2;
                const float* p1 = pts + idx[f * 3 + 1] * 2;
                const float* p2 = pts + idx[f * 3 + 2] * 2;
                float e = (pz - p0[1]) * (p1[0] - p0[0]) + (px - p0[0]) * (p0[1] - p1[1]);
                if (!(e > 0.0f))
                    continue;
                e = (px - p1[0]) * (p1[1] - p2[1]) + (pz - p1[1]) * (p2[0] - p1[0]);
                if (!(e > 0.0f))
                    continue;
                e = (pz - p2[1]) * (p0[0] - p2[0]) + (px - p2[0]) * (p2[1] - p0[1]);
                if (!(e > 0.0f))
                    continue;
                bm->Bits[bm->Stride * cz + (cx >> 3)] |= (unsigned char)(1u << (cx & 7));
            }
        }
    }
    mesh->UnlockIndexBuffer();                                    // 0x6cea30
    delete[] pts;
    delete[] idx;
    bm->X = (int)x0;                                              // cvttss2si
    bm->Z = (int)z0;
    return bm;
}
// PANZERS 0x6d8090
// The vertices of a node's mesh in the logic pose (node +0x30 matrix) into
// `out` (SDArray::SetSize(0) 0x5dd0c0 first, one 0x6d5890 add per vertex).
void SModel::GetNodePoints(const char* name, SVec3Array* out)
{
    if (out->Size != 0 && out->Array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SVector");
    out->Size = 0;
    if (out->Array)
        memset(out->Array, 0, (size_t)out->Max * 12);
    int node = FindNode(name);                                    // +0x40
    if (node < 0)
        return;
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    float m[12];
    memcpy(m, Nodes[node].PrevWorld, 48);
    if (node >= Proto->NodeCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPModelNode", node);
    SMesh* mesh = Proto->Nodes[node].Mesh;                        // proto node +0x44
    mesh->Lock();                                                 // +0x30
    for (int i = 0; i < mesh->VertexCount; ++i) {                 // 0x6ce990
        const float* v = (const float*)(mesh->Vertices + mesh->OffPosition + mesh->Stride * i);
        float x = ((v[0] * m[0] + v[1] * m[3]) + v[2] * m[6]) + m[9];
        float y = ((v[0] * m[1] + v[1] * m[4]) + v[2] * m[7]) + m[10];
        float z = ((v[0] * m[2] + v[1] * m[5]) + v[2] * m[8]) + m[11];
        if (out->Size == out->Max) {                              // 0x6d5890
            int nmax = out->Max < 0x10 ? 0x10 : out->Max * 6 / 5;
            out->Array = (float(*)[3])realloc(out->Array, (size_t)nmax * 12);
            memset(out->Array + out->Max, 0, (size_t)(nmax - out->Max) * 12);
            out->Max = nmax;
        }
        int k = out->Size++;
        out->Array[k][0] = x;
        out->Array[k][1] = y;
        out->Array[k][2] = z;
    }
    mesh->Unlock();                                               // +0x34
}

// PANZERS 0x6d6700
// The "Platform" node's mesh (a bridge deck) in the logic pose, sampled at the
// terrain vertices it covers: per vertex the height of the first face whose
// XZ projection strictly contains it (plane through the face), -FLT_MAX
// elsewhere. SWorld::FixBridges 0x5e65f0 takes the max into the second height
// map. The span is the terrain vertices ceil(min) .. floor(max) of the
// projected mesh (x87 frndint under 0x8de160 = 0x87f up / 0x8de15c = 0x47f down).
SHeightPatch* SModel::GetHeightPatch()
{
    int node = FindNode("Platform");                              // +0x40 (0x7f4350)
    if (node < 0)
        return nullptr;
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    float minX = 10000.0f, minZ = 10000.0f;                       // 0x7fd710
    float m[12];
    memcpy(m, Nodes[node].PrevWorld, 48);                         // node +0x30
    float maxX = -10000.0f, maxZ = -10000.0f;                     // 0x7f5aa8
    if (node >= Proto->NodeCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPModelNode", node);
    SMesh* mesh = Proto->Nodes[node].Mesh;                        // proto node +0x44
    int count = mesh->VertexCount;                                // 0x6ce990
    float* pts = new float[(size_t)count * 3]();                  // (x, y, z) per vertex
    mesh->Lock();                                                 // mesh +0x30
    for (int i = 0; i < count; ++i) {
        const float* v = (const float*)(mesh->Vertices + mesh->OffPosition + mesh->Stride * i);
        float x = v[0] * m[0];
        x = x + v[1] * m[3];
        x = x + v[2] * m[6];
        x = x + m[9];
        float y = v[0] * m[1];
        y = y + v[1] * m[4];
        y = y + v[2] * m[7];
        y = y + m[10];
        float z = v[0] * m[2];
        z = z + v[1] * m[5];
        z = z + v[2] * m[8];
        z = z + m[11];
        pts[i * 3] = x;
        pts[i * 3 + 1] = y;
        pts[i * 3 + 2] = z;
        if (minX > x)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (minZ > z)
            minZ = z;
        if (z > maxZ)
            maxZ = z;
    }
    mesh->Unlock();                                               // mesh +0x34
    minX = ceilf(minX);                                           // CW 0x8de160 = 0x87f (up)
    maxX = floorf(maxX);                                          // CW 0x8de15c = 0x47f (down)
    minZ = ceilf(minZ);
    maxZ = floorf(maxZ);
    int w = (int)((maxX - minX) + 1.0f);                          // 0x7f1b58, cvttss2si
    int h = (int)((maxZ - minZ) + 1.0f);
    if (w <= 0 && h <= 0) {
        delete[] pts;
        return nullptr;
    }
    SHeightPatch* p = new SHeightPatch;                           // new(0x14), 0x6619f0
    p->X = 0;
    p->Z = 0;
    p->W = w;
    p->H = h;
    p->Data = (float*)operator new((size_t)w * h * 4);
    memset(p->Data, 0, (size_t)w * h * 4);
    LockIndexBuffer(mesh);                                        // 0x6ce9a0
    int faces;
    unsigned short* idx;
    GetFaces(mesh, &faces, &idx);                                 // 0x6ce790
    for (int r = 0; r < h; ++r) {
        float pz = (float)r + minZ;
        for (int c = 0; c < w; ++c) {
            float px = (float)c + minX;
            float* out = &p->Data[p->W * r + c];
            unsigned bits = 0xff7fffffu;                          // -FLT_MAX
            memcpy(out, &bits, 4);
            for (int f = 0; f < faces; ++f) {
                const float* p0 = pts + idx[f * 3] * 3;
                const float* p1 = pts + idx[f * 3 + 1] * 3;
                const float* p2 = pts + idx[f * 3 + 2] * 3;
                float e = (p1[0] - p0[0]) * (pz - p0[2]) + (p0[2] - p1[2]) * (px - p0[0]);
                if (!(e > 0.0f))
                    continue;
                e = (pz - p1[2]) * (p2[0] - p1[0]) + (p1[2] - p2[2]) * (px - p1[0]);
                if (!(e > 0.0f))
                    continue;
                e = (p2[2] - p0[2]) * (px - p2[0]) + (p0[0] - p2[0]) * (pz - p2[2]);
                if (!(e > 0.0f))
                    continue;
                // 0x6d6bd1: the plane through the face at (px, pz).
                float d2y = p2[1] - p0[1], d1y = p1[1] - p0[1];
                float d1z = p1[2] - p0[2], d2z = p2[2] - p0[2];
                float d1x = p1[0] - p0[0], d2x = p2[0] - p0[0];
                float a = d2z * d1y - d2y * d1z;
                a = a * (px - p0[0]);
                float den = d1x * d2z - d2x * d1z;
                float b = d1x * d2y - d2x * d1y;
                b = b * (pz - p0[2]);
                *out = (a + b) / den + p0[1];
                break;
            }
        }
    }
    mesh->UnlockIndexBuffer();                                    // 0x6cea30
    delete[] pts;
    delete[] idx;
    p->X = (int)minX;                                             // cvttss2si
    p->Z = (int)minZ;
    return p;
}

void SModel::Slot_B4() { STUB_LOG("SModel::Slot_B4 (0x6d59e0)"); }
void SModel::Slot_B8() { STUB_LOG("SModel::Slot_B8 (0x6d5ae0)"); }
void SModel::Slot_BC() { STUB_LOG("SModel::Slot_BC (0x6dae10)"); }

// PANZERS 0x6dad40
void SModel::SetHighlight(int mode)
{
    Highlight = mode;
}

void SModel::Slot_C4() { STUB_LOG("SModel::Slot_C4 (0x6d7ef0)"); }
void SModel::Slot_C8() { STUB_LOG("SModel::Slot_C8 (0x6d7920)"); }
void SModel::Slot_CC() { STUB_LOG("SModel::Slot_CC (0x6dad80)"); }
// World point -> node space of collision node `node`: the inverse (0x7c59c0)
// of the logic-pose node matrix (+0x58, 0x6d7c60), row-vector convention,
// ((m0 x + m3 y) + m6 z) + m9 per axis (0x6db5df / 0x6db7c0).
static void ToNodeSpace(SModel* m, int node, const float* p, float* out)
{
    float mat[12], inv[12];
    m->GetNodeMatrix(mat, node);
    Mat34Inverse(inv, mat);
    float x = p[0], y = p[1], z = p[2];
    out[0] = ((inv[0] * x + inv[3] * y) + inv[6] * z) + inv[9];
    out[1] = ((inv[1] * x + inv[4] * y) + inv[7] * z) + inv[10];
    out[2] = ((inv[2] * x + inv[5] * y) + inv[8] * z) + inv[11];
}

static SCollisionBase* PolyNodeCollision(SPModel* proto, int i, int* node)
{
    if (i < 0 || i >= proto->PolyNodeCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", i);
    int n = proto->PolyNodes[i];
    if (n < 0 || n >= proto->NodeCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPModelNode", n);
    *node = n;
    return proto->Nodes[n].Collision;                             // proto node +0x48
}

// PANZERS 0x6db7c0
// Whether the world point lies inside one of the collision nodes (+0x08 of
// the collision object, in node space).
bool SModel::HitTestPoint(const float* p)
{
    for (int i = 0; i < Proto->PolyNodeCount; ++i) {
        int node;
        SCollisionBase* c = PolyNodeCollision(Proto, i, &node);
        float q[3];
        ToNodeSpace(this, node, p, q);
        if (c->TestPoint(q))
            return true;
    }
    return false;
}

void SModel::Slot_D4() { STUB_LOG("SModel::Slot_D4 (0x6db960)"); }
// PANZERS 0x6db550
// Whether the world segment a-b crosses one of the collision nodes (+0x10 of
// the collision object, in node space; b is transformed first, as in HD).
bool SModel::HitTestSegment(const float* a, const float* b)
{
    for (int i = 0; i < Proto->PolyNodeCount; ++i) {
        int node;
        SCollisionBase* c = PolyNodeCollision(Proto, i, &node);
        float qb[3], qa[3];
        ToNodeSpace(this, node, b, qb);
        ToNodeSpace(this, node, a, qa);
        if (c->TestLineSection(qa, qb))
            return true;
    }
    return false;
}
// PANZERS 0x6d5910
void SModel::AttachTo(SIModel* parent, int node)
{
    static_cast<SModel*>(parent)->AttachChild(node, static_cast<SIAttachable*>(this));   // 0x6d5940
}

// PANZERS 0x6d5940
void SModel::AttachChild(int node, SIAttachable* child)
{
    if (node < 0 || node >= Proto->NodeCount)
        Logger.g->Panic("SModel::AttachChild: Invalid node index");
    SModelNode& n = Nodes[node];
    // PANZERS 0x6d57b0 (SDArray<SIAttachable*>::Add)
    if (n.AttachedCount == n.AttachedMax) {
        int nmax = n.AttachedMax < 0x10 ? 0x10 : (n.AttachedMax * 6) / 5;
        n.Attached = (SIAttachable**)realloc(n.Attached, nmax * sizeof(SIAttachable*));
        memset(n.Attached + n.AttachedMax, 0, (nmax - n.AttachedMax) * sizeof(SIAttachable*));
        n.AttachedMax = nmax;
    }
    n.Attached[n.AttachedCount++] = child;
    SAttachable* c = static_cast<SAttachable*>(child);            // SModel (+0x04) or SEffectSet
    c->AttachParent = this;                                       // attachable +0x04
    c->AttachNode = node;                                         // attachable +0x08
    child->Attach_0C(Visible);                                    // +0x0c(this +0xd4 visible)
}

// PANZERS 0x6d7340
void SModel::DetachChild(int node, SIAttachable* child)
{
    if (node < 0 || node >= Proto->NodeCount)
        Logger.g->Panic("SModel::DetachChild: Invalid node index");
    SModelNode& n = Nodes[node];
    for (int i = 0; i < n.AttachedCount; ++i) {
        if (n.Attached[i] != child)
            continue;
        // PANZERS 0x6d87b0 (SDArray<SIAttachable*>::Remove)
        --n.AttachedCount;
        if (n.AttachedCount - i != 0)
            memmove(&n.Attached[i], &n.Attached[i + 1], (n.AttachedCount - i) * sizeof(SIAttachable*));
        n.Attached[n.AttachedCount] = nullptr;
        SAttachable* c = static_cast<SAttachable*>(child);
        c->AttachParent = nullptr;
        c->AttachNode = -1;
        return;
    }
    Logger.g->Panic("SModel::DetachChild: Child was not attached");
}
void SModel::Slot_E0() { STUB_LOG("SModel::Slot_E0 (0x6d7310)"); }
void SModel::Slot_E4() { STUB_LOG("SModel::Slot_E4 (0x6d62e0)"); }

// PANZERS 0x6dade0
void SModel::SetSway(float phase, float p2, float p3)
{
    PZ_TRACE("SModel::SetSway (0x6dade0)");
    SwayPhase = phase;
    SwayX = p2;
    SwayZ = p3;
}

// PANZERS 0x6da310
void SModel::SetColor(bool on, unsigned color)
{
    ColorOverride = on;
    Color = (int)color;
}
// PANZERS 0x6da2c0
void SModel::SetColor2(bool on, unsigned color)
{
    Color2Override = on;
    Color2 = (int)color;
}
void SModel::Slot_F4() { STUB_LOG("SModel::Slot_F4 (0x6d7900)"); }
void SModel::Slot_F8() { STUB_LOG("SModel::Slot_F8 (0x6d85d0)"); }
// PANZERS 0x6d6f40
void SModel::GetWorldBounds(float* minX, float* maxX, float* minY, float* maxY, float* minZ, float* maxZ)
{
    ExtraFrame = Scene->FrameCount;
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);
    }
    *minX = 10000.0f; *maxX = -10000.0f;
    *minY = 10000.0f; *maxY = -10000.0f;
    *minZ = 10000.0f; *maxZ = -10000.0f;
    for (int i = 0; i < Proto->NodeCount; ++i) {
        const SPModelNode& pn = Proto->Nodes[i];
        if (!pn.Mesh)
            continue;
        const float* m = Nodes[i].PrevWorld;
        for (int c = 0; c < 8; ++c) {
            float x = pn.BBox[c * 3], y = pn.BBox[c * 3 + 1], z = pn.BBox[c * 3 + 2];
            float wx = m[3] * y + x * m[0] + m[6] * z + m[9];
            float wy = m[1] * x + m[4] * y + m[7] * z + m[10];
            float wz = m[2] * x + m[5] * y + m[8] * z + m[11];
            if (wx < *minX) *minX = wx;
            if (*maxX < wx) *maxX = wx;
            if (wy < *minY) *minY = wy;
            if (*maxY < wy) *maxY = wy;
            if (wz < *minZ) *minZ = wz;
            if (*maxZ < wz) *maxZ = wz;
        }
    }
}
// PANZERS 0x6d7150
// XZ extent of the BBOX corners of every mesh node in the logic pose (node
// +0x30 matrix): ((m3 y + m0 x) + m6 z) + m9 and ((m2 x + m5 y) + m8 z) + m11.
void SModel::GetLogicBoundsXZ(float* minX, float* maxX, float* minZ, float* maxZ)
{
    ExtraFrame = Scene->FrameCount;                               // scene +0xa0
    if (PrevDirty) {
        PrevDirty = false;
        ComputeNodes(ExtraFrame, nullptr, true);                  // 0x6dc7b0(frame, 0, 1)
    }
    *minX = 10000.0f;
    *maxX = -10000.0f;
    *minZ = 10000.0f;
    *maxZ = -10000.0f;
    for (int i = 0; i < Proto->NodeCount; ++i) {
        const SPModelNode& pn = Proto->Nodes[i];
        if (!pn.Mesh)                                             // proto node +0x44
            continue;
        const float* m = Nodes[i].PrevWorld;                      // node +0x30
        for (int k = 0; k < 8; ++k) {
            const float* c = &pn.BBox[k * 3];                     // proto node +0x54
            float x = ((m[3] * c[1] + m[0] * c[0]) + m[6] * c[2]) + m[9];
            float z = ((m[2] * c[0] + m[5] * c[1]) + m[8] * c[2]) + m[11];
            if (x < *minX)
                *minX = x;
            if (*maxX < x)
                *maxX = x;
            if (z < *minZ)
                *minZ = z;
            if (*maxZ < z)
                *maxZ = z;
        }
    }
}
void SModel::Slot_104() { STUB_LOG("SModel::Slot_104 (0x6d7400)"); }

// PANZERS 0x6d86e0
void SModel::Attach_08() {}
// PANZERS 0x6d86f0
void SModel::Attach_0C(bool) {}

// ---------------------------------------------------------------------------
// animation
// ---------------------------------------------------------------------------

// PANZERS 0x6dae20
// Advances the playback state by dt. At the end of a sequence: Next == -2
// loops (fmod), Next == -1 holds the last frame, else the next sequence
// starts and the old one is blended out over the new one's BlendTime.
SAnimState SModel::Advance(const SAnimState& in, float dt) const
{
    SAnimState s = in;
    const SPSequence* seqs = Proto->Sequences;
    int seq = s.Seq;
    float time = s.Time;
    int prev = s.PrevSeq;
    float prevTime = s.PrevTime;
    float blend;
    if (dt < 0.0f) {
        time = time + dt;
        while (time < 0.0f)
            time = time + seqs[seq].Length;
        blend = 1.0f;
        prev = -1;
    } else {
        prevTime = prevTime + dt;
        time = time + dt;
        float len = seqs[seq].Length;
        if (len <= time) {
            for (;;) {
                const SPSequence& cur = seqs[seq];
                float t = time;
                if (cur.Next == -2) {
                    time = (float)fmod((double)t, (double)len);   // 0x793cba
                    prev = -1;
                    break;
                }
                time = len;
                if (cur.Next < 0)
                    break;
                time = t - len;
                prev = seq;
                prevTime = t;
                seq = cur.Next;
                len = seqs[seq].Length;
                if (!(len <= time))
                    break;
            }
        }
        float bt = seqs[seq].BlendTime;
        if (prev >= 0 && time < bt) {
            blend = time / bt;
            if (seqs[prev].Next == -2)
                prevTime = (float)fmod((double)prevTime, (double)seqs[prev].Length);
        } else {
            blend = 1.0f;
            prev = -1;
        }
    }
    s.Seq = seq;
    s.Time = time;
    s.PrevSeq = prev;
    s.PrevTime = prevTime;
    s.Blend = blend;
    return s;
}

// PANZERS 0x6dc4f0
void SModel::UpdateFade()
{
    if (!Visible)
        return;
    float now = Scene->Seconds;
    if (FadeState == 1) {
        float a = now - FadeStart;
        FadeAlpha = a;
        if (1.0f <= a) {
            FadeState = 0;
            FadeAlpha = 1.0f;
        }
    } else if (FadeState == -1) {
        float a = 1.0f - (now - FadeStart);
        FadeAlpha = a;
        if (a <= 0.0f)
            SetVisible(false, false);   // vtbl +0x30: hide
    }
    if (NodeFadeState == 1) {
        float a = now - NodeFadeStart;
        NodeFadeAlpha = a;
        if (1.0f <= a) {
            NodeFadeState = 0;
            NodeFadeAlpha = 1.0f;
            SetNodeVisible(NodeFadeNode2, false);
        }
    } else if (NodeFadeState == -1) {
        float a = 1.0f - (now - NodeFadeStart);
        NodeFadeAlpha = a;
        if (a <= 0.0f)
            SetNodeVisible(NodeFadeNode, false);
    }
}

// PANZERS 0x6dc7b0
// Model and node world matrices. prev selects the second set (+0x58 model,
// node +0x30) HD builds from the current pose for the shadow pass.
void SModel::ComputeNodes(int frame, const float* attach, bool prev)
{
    (void)frame;
    float* world = prev ? PrevWorld : World;
    if (!prev) {
        if (attach) {
            memcpy(World, attach, 48);
            goto nodes;
        }
        float pose[8];
        const float* src;
        if (Flags & 1) {
            float t = (float)Scene->Interpolation;   // 0x6d4f30
            for (int i = 0; i < 4; ++i)
                pose[i] = ((&PrevPos[0])[i] - (&Pos[0])[i]) * t + (&Pos[0])[i];
            QuatSlerp(pose + 4, Quat, PrevQuat, t);
            src = pose;
        } else {
            memcpy(pose, Pos, 32);
            src = pose;
        }
        if (Flags & 0x20) {
            // Sway (trees): tilt by sin/cos of the scene time.
            int ms = Scene->TimeMs;
            float t = (float)(((double)ms + (ms < 0 ? 4294967296.0 : 0.0)) * 0.001);
            float a = (float)(HdSin((double)t * 1.1 + (double)SwayPhase) * (double)SwayX);
            float b = (float)(HdCos((double)t * 0.9 + (double)SwayPhase) * (double)SwayZ);
            double len = sqrt((double)(b * b + a * a));
            if (1e-05 <= len) {
                float ax = (float)((double)b / len);
                float az = (float)((double)-a / len);
                double h = (double)HdFastAtan((float)len) * 0.5;
                float s = (float)HdSin(h);
                float tq[4] = { ax * s, s * 0.0f, az * s, (float)HdCos(h) };
                const float* q = src + 4;
                float r[4];   // 0x6d5380: tq * q
                r[0] = (tq[0] * q[3] + q[0] * tq[3] + tq[1] * q[2]) - tq[2] * q[1];
                r[1] = (tq[1] * q[3] + q[1] * tq[3] + tq[2] * q[0]) - q[2] * tq[0];
                r[2] = (tq[2] * q[3] + q[2] * tq[3] + q[1] * tq[0]) - tq[1] * q[0];
                r[3] = ((q[3] * tq[3] - tq[0] * q[0]) - q[1] * tq[1]) - tq[2] * q[2];
                memcpy(pose + 4, r, 16);
            }
        }
        PoseToMatrix(World, src, src[3], src + 4);
    } else {
        PoseToMatrix(PrevWorld, Pos, Scl, Quat);
    }
    if (Proto->FlyZ) {
        static const float kFlyZ[12] = { 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 };   // 0x8df834 (Y<->Z)
        Mat34Mul(world, kFlyZ, world);
    }
nodes:
    // Frame-sequence models on the main heap (doodads) play their first
    // sequence on the scene clock.
    if (Proto->SequenceCount != 0 && MainHeap == 1) {
        const SPSequence& s0 = Proto->Sequences[0];
        float t = (float)fmod((double)Scene->Seconds, (double)s0.Length);
        Dirty = true;
        Anim.Seq = 0;
        Anim.Time = t;
        if (s0.Next == -2 || s0.BlendTime <= t) {
            Anim.PrevSeq = -1;
        } else {
            Anim.PrevSeq = 0;
            Anim.PrevTime = s0.Length + t;
            Anim.Blend = t / s0.BlendTime;
        }
    }
    const SPSequence* seqA = nullptr;
    const SPSequence* seqB = nullptr;
    SAnimState st = Anim;
    if (Proto->SequenceCount != 0 && !Proto->FrameSequences) {
        if (Flags & 4) {
            float dt = AnimDelta;
            if (!prev)
                dt = (float)((1.0 - Scene->Interpolation) * (double)dt);
            st = Advance(Anim, dt);
        }
        seqA = &Proto->Sequences[st.Seq];
        if (st.PrevSeq >= 0)
            seqB = &Proto->Sequences[st.PrevSeq];
    }

    for (int i = 0; i < Proto->NodeCount; ++i) {
        SPModelNode& pn = Proto->Nodes[i];
        SModelNode& mn = Nodes[i];
        float* out = prev ? mn.PrevWorld : mn.World;
        // User local transform of the node.
        float L[12];
        if (!prev && (Flags & 2)) {
            float t = (float)Scene->Interpolation;
            float p[4];
            for (int k = 0; k < 4; ++k)
                p[k] = ((&mn.PrevPos[0])[k] - (&mn.Pos[0])[k]) * t + (&mn.Pos[0])[k];
            float q[4];
            QuatSlerp(q, mn.Quat, mn.PrevQuat, t);
            PoseToMatrix(L, p, p[3], q);
        } else {
            PoseToMatrix(L, mn.Pos, mn.Scale, mn.Quat);
        }
        if (!seqA) {
            float M[12];
            Mat34Mul(M, L, pn.Transform);
            if (pn.Parent < 0) {
                Mat34Mul(out, M, world);
            } else if (pn.Link >= 0) {
                const float* lw = prev ? Nodes[pn.Link].PrevWorld : Nodes[pn.Link].World;
                const float* pw = prev ? Nodes[pn.Parent].PrevWorld : Nodes[pn.Parent].World;
                float r[12];
                Mat34Mul(r, M, lw);
                r[9] = pw[3] * M[10] + pw[0] * M[9] + pw[6] * M[11] + pw[9];
                r[10] = pw[4] * M[10] + pw[1] * M[9] + pw[7] * M[11] + pw[10];
                r[11] = pw[5] * M[10] + pw[2] * M[9] + pw[8] * M[11] + pw[11];
                memcpy(out, r, 48);
            } else {
                const float* pw = prev ? Nodes[pn.Parent].PrevWorld : Nodes[pn.Parent].World;
                Mat34Mul(out, M, pw);
            }
            continue;
        }
        Dirty = true;
        PrevDirty = true;
        int parent = pn.Parent;
        const SPChannel& ca = seqA->Channels[i];
        float block[19];   // 3x4 channel matrix, position, quaternion
        memcpy(block, ca.Matrix, 48);
        memcpy(block + 12, ca.Pos, 12);
        memcpy(block + 15, ca.Quat, 16);
        if (ca.Link)
            ca.Link->Evaluate(st.Time, &parent, block);
        if (ca.PosTrack)
            ca.PosTrack->Evaluate(block + 12, st.Time);
        if (ca.RotTrack)
            ca.RotTrack->Evaluate(block + 15, st.Time);
        float pos[3];
        float q[4];
        if (!seqB) {
            memcpy(pos, block + 12, 12);
            memcpy(q, block + 15, 16);
        } else {
            const SPChannel& cb = seqB->Channels[i];
            float bb[19];
            memcpy(bb, cb.Matrix, 48);
            memcpy(bb + 12, cb.Pos, 12);
            memcpy(bb + 15, cb.Quat, 16);
            if (cb.Link)
                cb.Link->Evaluate(st.PrevTime, &parent, bb);
            if (cb.PosTrack)
                cb.PosTrack->Evaluate(bb + 12, st.PrevTime);
            if (cb.RotTrack)
                cb.RotTrack->Evaluate(bb + 15, st.PrevTime);
            float w = st.Blend;
            for (int k = 0; k < 3; ++k)
                pos[k] = (block[12 + k] - bb[12 + k]) * w + bb[12 + k];
            QuatSlerp(q, bb + 15, block + 15, w);
        }
        float R[12], T[12], M[12];
        QuatTransToMatrix(R, q, pos);
        Mat34Mul(T, L, R);
        Mat34Mul(M, T, block);
        if (parent < 0) {
            if (i == 0) {
                memcpy(out, M, 48);
            } else {
                const float* root = prev ? Nodes[0].PrevWorld : Nodes[0].World;
                float inv[12], t2[12];
                Mat34Inverse(inv, root);
                Mat34Mul(t2, M, inv);
                Mat34Mul(out, t2, world);
            }
        } else if (pn.Link < 0) {
            const float* pw = prev ? Nodes[parent].PrevWorld : Nodes[parent].World;
            if (ca.Inherit != 0x38) {
                Mat34Mul(out, M, pw);
            } else {
                float r[12];
                Mat34Mul(r, M, world);
                r[9] = pw[3] * M[10] + pw[0] * M[9] + pw[6] * M[11] + pw[9];
                r[10] = pw[4] * M[10] + pw[1] * M[9] + pw[7] * M[11] + pw[10];
                r[11] = pw[5] * M[10] + pw[2] * M[9] + pw[8] * M[11] + pw[11];
                memcpy(out, r, 48);
            }
        } else {
            const float* lw = prev ? Nodes[pn.Link].PrevWorld : Nodes[pn.Link].World;
            const float* pw = prev ? Nodes[parent].PrevWorld : Nodes[parent].World;
            float r[12];
            Mat34Mul(r, M, lw);
            r[9] = pw[3] * M[10] + pw[0] * M[9] + pw[6] * M[11] + pw[9];
            r[10] = pw[4] * M[10] + pw[1] * M[9] + pw[7] * M[11] + pw[10];
            r[11] = pw[5] * M[10] + pw[2] * M[9] + pw[8] * M[11] + pw[11];
            memcpy(out, r, 48);
        }
        if (pn.Light && ca.Fov)
            pn.Light->Intensity = ca.Fov->Evaluate(st.Time);
        else if (pn.Camera && ca.Fov)
            pn.Camera->Data[0] = ca.Fov->Evaluate(st.Time);
        if (ca.Visibility)
            mn.Visible = ca.Visibility->Evaluate(st.Time);
    }
}

// PANZERS 0x6dba80 (SIAttachable +0x04; HD "this" is SModel+4)
int SModel::Update(int frame, int attachMatrix)
{
    Frame = frame;
    FlagBit8 = ((Flags >> 8) & 1) != 0;
    Drawn = false;
    DrawnDeferred = false;
    UpdateFade();
    if (!Visible)
        return 1;
    if (Dirty || (Flags & 0x23) || attachMatrix) {
        Dirty = false;
        ComputeNodes(frame, (const float*)(intptr_t)attachMatrix, false);
    }
    for (int i = 0; i < Proto->NodeCount; ++i) {
        SPModelNode& pn = Proto->Nodes[i];
        SModelNode& mn = Nodes[i];
        if (pn.Parent < 0)
            mn.EffVisible = mn.Visible;
        else
            mn.EffVisible = mn.Visible && Nodes[pn.Parent].EffVisible;
        // ':' effect nodes: HD starts (pixie +0x30) / stops (+0x34) the
        // node effect with the node visibility. Pixie slots are agent C's.
        for (int k = 0; k < mn.AttachedCount;) {
            if (mn.Attached[k]->Update(frame, (int)(intptr_t)mn.World))
                ++k;
        }
        if (pn.Light) {
            SPLight* l = pn.Light;
            float c = l->Intensity * 0.5f;   // 0x7f453c
            float col[3] = { l->Color[0] * c, l->Color[1] * c, l->Color[2] * c };
            if (l->Type == 1) {
                if (mn.Light < 0)
                    mn.Light = Scene->CreatePointLight(col[0], col[1], col[2], 1.0f, mn.World[9], mn.World[10], mn.World[11], l->Range, l->Atten2);   // 0x6a8db0
                Scene->SetLightColor(mn.Light, col);
                Scene->SetLightPosition(mn.Light, mn.World[9], mn.World[10], mn.World[11]);
            } else if (l->Type == 2 || l->Type == 3) {
                float dir[3];
                if (pn.Parent < 0) {
                    dir[0] = -mn.World[6]; dir[1] = -mn.World[7]; dir[2] = -mn.World[8];
                } else {
                    const float* pw = Nodes[pn.Parent].World;
                    dir[0] = pw[9] - mn.World[9]; dir[1] = pw[10] - mn.World[10]; dir[2] = pw[11] - mn.World[11];
                }
                if (mn.Light < 0)
                    mn.Light = Scene->CreateDirectionalLight(col, dir);
                Scene->SetLightColor(mn.Light, col);
                Scene->SetLightDirection(mn.Light, dir);
                if (l->Type == 2)
                    Scene->SetLightPosition(mn.Light, mn.World[9], mn.World[10], mn.World[11]);
            } else {
                Logger.g->Log(0, "SModel::Update: Light type is not yet implemented");
            }
        }
    }
    // Terrain shadow decal of the model (+0x12c texture, +0xe8 decal):
    // terrain slots +0x60/+0x74/+0x68 (agent B); not used by the menu.
    return 1;
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

// PANZERS 0x6d8830
void SModel::Render(SViewport* vp)
{
    if (!g_MeshDraw.DrawingDeferred) {
        if (Drawn)
            return;
        Drawn = true;
    } else {
        if (DrawnDeferred)
            return;
        DrawnDeferred = true;
    }
    if (!Visible)
        return;
    if (!g_MeshDraw.DrawingDeferred && (Flags & 0x40))
        g_MeshDraw.DeferAlpha = true;
    if (Highlight != 0) {
        if (Highlight == 3)
            g_MeshDraw.FogTint = 0x40ffffc0;
        else if (Highlight == 2)
            g_MeshDraw.FogTint = 0x20ffffff;
        else if (Highlight == 1)
            g_MeshDraw.FogTint = 0x40ffff00;
    }
    if (Highlight & 4) {
        g_MeshDraw.AlphaOverride = true;
        g_MeshDraw.Alpha = 0.5f;
    } else if (FadeState != 0) {
        g_MeshDraw.AlphaOverride = true;
        g_MeshDraw.Alpha = FadeAlpha;
    }
    g_MeshDraw.ColorOverride = ColorOverride;
    g_MeshDraw.Color = Color;
    // Fog of war darkening (flag 0x80, terrain visibility 0x6d85e0) needs the
    // terrain's visibility map; without it the model's own colour is used.
    g_MeshDraw.Color2Override = Color2Override;
    g_MeshDraw.Color2 = Color2;

    bool lowPoly = false;
    if (Proto2) {
        float cx, cy, cz, yaw, pitch;
        vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);
        float dy = World[10] - cy, dx = World[9] - cx, dz = World[11] - cz;
        lowPoly = 1600.0f < dy * dy + dx * dx + dz * dz;
    }
    SPModel* pm = lowPoly ? Proto2 : Proto;
    static float s_Bones[0x1c * 12];   // 0x93cf28
    for (int i = 0; i < Proto->NodeCount; ++i) {
        SMesh* mesh = pm->Nodes[i].Mesh;
        SModelNode& mn = Nodes[i];
        if (!mesh || !mn.EffVisible)
            continue;
        if (NodeFadeState != 0) {
            if (i == NodeFadeNode) {
                g_MeshDraw.AlphaOverride = true;
                g_MeshDraw.Alpha = NodeFadeAlpha;
            } else {
                g_MeshDraw.AlphaOverride = false;
            }
        }
        int ta = mn.TexAnimType;
        if (ta == 1 || ta == 2) {
            if (!(Flags & 2)) {
                g_MeshDraw.TexAnim[0] = mn.TexAnim[0];
                g_MeshDraw.TexAnim[1] = mn.TexAnim[2];
                if (ta == 2)
                    g_MeshDraw.TexAnim[2] = mn.TexAnim[4];
            } else {
                double t = Scene->Interpolation;
                g_MeshDraw.TexAnim[0] = (float)((double)(mn.TexAnim[1] - mn.TexAnim[0]) * t + (double)mn.TexAnim[0]);
                g_MeshDraw.TexAnim[1] = (float)((double)(mn.TexAnim[3] - mn.TexAnim[2]) * t + (double)mn.TexAnim[2]);
                if (ta == 2)
                    g_MeshDraw.TexAnim[2] = (float)((double)(mn.TexAnim[5] - mn.TexAnim[4]) * t + (double)mn.TexAnim[4]);
            }
            g_MeshDraw.TexAnimType = ta;
        }
        if (Proto->SequenceCount == 0 || !Proto->FrameSequences) {
            SPModelNode& pn = Proto->Nodes[i];
            if (pn.BoneCount == 0) {
                GepardSetWorld(mn.World);
                mesh->Draw(Scene);
            } else {
                int n = pn.BoneCount;
                if (0x1c < n)
                    Logger.g->Panic("SModel::Render: Too many bones (%d)", n);
                for (int b = 0; b < n; ++b)
                    Mat34Mul(s_Bones + b * 12, pn.Bones[b].Matrix, Nodes[pn.Bones[b].Node].World);
                GepardSetWorldIdentity();
                mesh->DrawSkinned(Scene, n, s_Bones);
            }
        } else {
            GepardSetWorld(mn.World);
            SAnimState st = Anim;
            if (Flags & 4)
                st = Advance(Anim, (float)((1.0 - Scene->Interpolation) * (double)AnimDelta));
            const SPSequence* seqs = Proto->Sequences;
            const SPSequence& s = seqs[st.Seq];
            if (st.PrevSeq < 0) {
                if (s.FrameCount < 2 || s.Length <= st.Time) {
                    mesh->DrawFrame(Scene, s.FrameCount + s.FirstFrame - 1);
                } else {
                    int k = 0;
                    for (int j = 1; j < s.FrameCount - 1 && !(st.Time < s.FrameTimes[j]); ++j)
                        k = j;
                    float a = s.FrameTimes[k];
                    int f = s.FirstFrame + k;
                    mesh->DrawFramesLerp(Scene, f, f + 1, (st.Time - a) / (s.FrameTimes[k + 1] - a));
                }
            } else {
                const SPSequence& p = seqs[st.PrevSeq];
                if (p.BlendTime <= st.PrevTime) {
                    if (p.FrameCount < 2 || p.Length <= st.PrevTime) {
                        mesh->DrawFramesLerp(Scene, p.FrameCount + p.FirstFrame - 1, s.FirstFrame, st.Blend);
                    } else {
                        int k = 0;
                        for (int j = 1; j < p.FrameCount - 1 && !(st.PrevTime < p.FrameTimes[j]); ++j)
                            k = j;
                        float a = p.FrameTimes[k];
                        int f = p.FirstFrame + k;
                        mesh->DrawFramesBlend(Scene, f, f + 1, (st.PrevTime - a) / (p.FrameTimes[k + 1] - a),
                                              s.FirstFrame, st.Blend);
                    }
                } else {
                    mesh->DrawFramesLerp(Scene, p.FirstFrame, s.FirstFrame, st.Blend);
                }
            }
        }
        g_MeshDraw.TexAnimType = 0;
    }
    g_MeshDraw.FogTint = 0;
    g_MeshDraw.AlphaOverride = false;
    g_MeshDraw.ColorOverride = false;
    g_MeshDraw.Color2Override = false;
    if (!g_MeshDraw.DrawingDeferred && (Flags & 0x40)) {
        g_MeshDraw.DeferAlpha = false;
        Scene->AddDeferredModel(this);   // 0x6d5820 on scene +0x2a0
    }
}

// PANZERS 0x6d9570
// The model into the shadow buffer (SScene::GenerateShadowBuffer): once per
// frame, visible and not fading out, every visible node mesh of the full
// prototype through the shadow variants of the mesh draws.
void SModel::RenderShadow(SViewport* vp)
{
    (void)vp;   // HD 0x68d1e0(vp, node) sets VS c12 for the node; DrawShadow sets it again
    if (FlagBit8)
        return;
    FlagBit8 = true;
    if (FadeState < 0 || !Visible)
        return;
    static float s_Bones[0x1c * 12];   // 0x93cf28 (shared with Render)
    for (int i = 0; i < Proto->NodeCount; ++i) {
        SMesh* mesh = Proto->Nodes[i].Mesh;
        SModelNode& mn = Nodes[i];
        if (!mesh || !mn.EffVisible)
            continue;
        if (Proto->SequenceCount == 0 || !Proto->FrameSequences) {
            SPModelNode& pn = Proto->Nodes[i];
            if (pn.BoneCount == 0) {
                GepardSetWorld(mn.World);             // 0x680fb0
                mesh->DrawShadow(Scene);               // +0x08
            } else {
                int n = pn.BoneCount;
                if (0x1c < n)
                    Logger.g->Panic("SModel::RenderShadow: Too many bones (%d)", n);
                // Without SGepard vertex shader 1 (never created): CPU bones.
                for (int b = 0; b < n; ++b)
                    Mat34Mul(s_Bones + b * 12, pn.Bones[b].Matrix, Nodes[pn.Bones[b].Node].World);
                GepardSetWorldIdentity();              // 0x680fe0
                mesh->DrawShadowSkinned(Scene, n, s_Bones);   // +0x18
            }
            continue;
        }
        GepardSetWorld(mn.World);
        SAnimState st = Anim;
        if (Flags & 4)
            st = Advance(Anim, (float)((1.0 - Scene->Interpolation) * (double)AnimDelta));   // 0x6dae20
        const SPSequence* seqs = Proto->Sequences;
        const SPSequence& s = seqs[st.Seq];
        if (st.PrevSeq < 0) {
            if (s.FrameCount < 2 || s.Length <= st.Time) {
                mesh->DrawShadowFrame(Scene, s.FrameCount + s.FirstFrame - 1);   // +0x14
            } else {
                int k = 0;
                for (int j = 1; j < s.FrameCount - 1 && !(st.Time < s.FrameTimes[j]); ++j)
                    k = j;
                float a = s.FrameTimes[k];
                int f = s.FirstFrame + k;
                mesh->DrawShadowFramesLerp(Scene, f, f + 1, (st.Time - a) / (s.FrameTimes[k + 1] - a));   // +0x10
            }
        } else {
            const SPSequence& p = seqs[st.PrevSeq];
            if (p.BlendTime <= st.PrevTime) {
                if (p.FrameCount < 2 || p.Length <= st.PrevTime) {
                    mesh->DrawShadowFramesLerp(Scene, p.FrameCount + p.FirstFrame - 1, s.FirstFrame, st.Blend);
                } else {
                    int k = 0;
                    for (int j = 1; j < p.FrameCount - 1 && !(st.PrevTime < p.FrameTimes[j]); ++j)
                        k = j;
                    float a = p.FrameTimes[k];
                    int f = p.FirstFrame + k;
                    mesh->DrawShadowFramesBlend(Scene, f, f + 1, (st.PrevTime - a) / (p.FrameTimes[k + 1] - a),
                                                s.FirstFrame, st.Blend);   // +0x0c
                }
            } else {
                mesh->DrawShadowFramesLerp(Scene, p.FirstFrame, s.FirstFrame, st.Blend);
            }
        }
    }
}

} // namespace pz
