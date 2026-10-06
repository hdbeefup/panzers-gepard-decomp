// src/game/unitanim_model.cpp
// SModel / SGepard slot bodies used by the unit animations (see
// unitanim_model.h). OWNER: agent A.

#include <math.h>
#include <string.h>
#include "unitanim_model.h"
#include "pz/pzmodel.h"
#include "pz/pzscene.h"
#include "pz/pzgepard.h"

namespace pz {

static SModel* AsModel(SIModel* m)
{
    return static_cast<SModel*>(m);
}

static bool ValidNode(SModel* m, int node)
{
    return m && node >= 0 && node < m->Proto->NodeCount;
}

// PANZERS 0x661440
// Fast atan: x / (1 + 0.280872 x^2), folded through pi/2 - atan(1/x) beyond 1.
static float FastAtan(float x)
{
    double d = (double)x;
    if (1.0f < x) {
        double r = (double)(float)(1.0 / d);
        return (float)(1.5707963705062866 - r / ((double)(float)(1.0 / d) * 0.280872 * r + 1.0));
    }
    if (x < -1.0f) {
        double r = (double)(float)(1.0 / d);
        return (float)(-1.5707963705062866 - r / ((double)(float)(1.0 / d) * 0.280872 * r + 1.0));
    }
    return (float)((double)x / (d * 0.280872 * (double)x + 1.0));
}

// PANZERS 0x6db2a0
void AnimModelSetNodeFade(SIModel* im, bool show, int node, int node2)
{
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    bool cur = m->Nodes[node].Visible && m->NodeFadeState != -1;
    if (cur == show)
        return;
    m->UpdateFade();                                              // 0x6dc4f0
    m->NodeFadeNode = node;
    m->NodeFadeNode2 = node2;
    float now = m->Scene->Seconds;                                // scene +0xb0
    if (!show) {
        if (!m->Nodes[node].Visible)
            return;
        m->SetNodeVisible(node2, true);                           // +0x60
        if (m->NodeFadeState == 1) {
            m->NodeFadeState = -1;
            m->NodeFadeStart = (now + m->NodeFadeAlpha) - 1.0f;
            return;
        }
        if (m->NodeFadeState != 0)
            return;
        m->NodeFadeState = -1;
        m->NodeFadeAlpha = 1.0f;
    } else {
        if (m->Nodes[node].Visible) {
            if (m->NodeFadeState != -1)
                return;
            m->NodeFadeState = 1;
            m->NodeFadeStart = now - m->NodeFadeAlpha;
            return;
        }
        m->SetNodeVisible(node, true);
        SModelNode& fn = m->Nodes[m->NodeFadeNode];
        for (int i = 0; i < fn.AttachedCount; ++i)
            fn.Attached[i]->Attach_0C();                          // attachment +0x0c(show)
        for (int j = 0; j < m->Proto->NodeCount; ++j) {
            if (m->Proto->Nodes[j].Parent != m->NodeFadeNode)
                continue;
            SModelNode& cn = m->Nodes[j];
            for (int i = 0; i < cn.AttachedCount; ++i)
                cn.Attached[i]->Attach_0C();
        }
        m->NodeFadeState = 1;
        m->NodeFadeAlpha = 0.0f;
    }
    m->NodeFadeStart = now;
}

// PANZERS 0x6da4d0
void AnimModelSetNodeTilt(SIModel* im, int node, float x, float y, float z, float yaw, float tiltX, float tiltZ)
{
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    SModelNode& n = m->Nodes[node];
    float inv = 1.0f / m->Scl;
    n.Pos[0] = inv * x;
    n.Pos[1] = inv * y;
    n.Pos[2] = inv * z;
    float q[4];
    double len = (double)(tiltX * tiltX + tiltZ * tiltZ);
    if (1e-05 <= len) {
        len = sqrt(len);
        double h = (double)yaw * 0.5;
        float sA = (float)sin(h);
        float f9 = sA * 0.0f;
        float cA = (float)cos(h);
        float nz = -tiltZ;
        double t = (double)FastAtan((float)len) * 0.5;
        float sT = (float)sin(t);
        float f4 = (float)((double)nz / len) * sT;
        float f5 = (float)((double)tiltX / len) * sT;
        float f10 = sT * 0.0f;
        float cT = (float)cos(t);
        q[0] = (f4 * cA + f9 * cT + f5 * sA) - f10 * f9;
        q[1] = (f5 * cA + f9 * cT + f10 * f9) - f4 * sA;
        q[3] = ((cT * cA - f4 * f9) - f5 * f9) - f10 * sA;
        q[2] = (sA * cT + f10 * cA + f4 * f9) - f5 * f9;
    } else {
        double h = (double)yaw * 0.5;
        float sA = (float)sin(h);
        q[0] = sA * 0.0f;
        q[1] = q[0];
        q[2] = sA;
        q[3] = (float)cos(h);
    }
    memcpy(n.Quat, q, sizeof(q));
    m->Dirty = true;                                              // +0xc8 = 0x101
    m->PrevDirty = true;
}

// PANZERS 0x6da330
void AnimModelSetNodeRotation(SIModel* im, int node, float x, float y, float z, float a, float b)
{
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    SModelNode& n = m->Nodes[node];
    float inv = 1.0f / m->Scl;
    n.Pos[0] = inv * x;
    n.Pos[1] = inv * y;
    n.Pos[2] = inv * z;
    double ha = (double)a * 0.5;
    float sA = (float)sin(ha);
    float f6 = sA * 0.0f;
    float cA = (float)cos(ha);
    double hb = (double)b * 0.5;
    float sB = (float)sin(hb);
    float f8 = sB * 0.0f;
    float cB = (float)cos(hb);
    float f9 = f8 * f6;
    n.Quat[0] = (sB * cA + f6 * cB + f8 * sA) - f9;
    n.Quat[1] = (f8 * cA + f6 * cB + f9) - sB * sA;
    n.Quat[2] = (sA * cB + f8 * cA + sB * f6) - f9;
    n.Quat[3] = ((cB * cA - sB * f6) - f9) - f8 * sA;
    m->Dirty = true;
    m->PrevDirty = true;
}

// PANZERS 0x6d7d30
void AnimModelGetNodePosition(SIModel* im, int node, float* xyz)
{
    // HD reads the translation of the node matrix that +0x58 (0x6d7c60)
    // returns; the recompile uses the node pose of the last Update.
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    xyz[0] = m->Nodes[node].World[9];
    xyz[1] = m->Nodes[node].World[10];
    xyz[2] = m->Nodes[node].World[11];
}

// PANZERS 0x6da810
void AnimModelSetNodeTexRotation(SIModel* im, int node, float u, float v, float angle)
{
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    SModelNode& n = m->Nodes[node];
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
void AnimModelSetNodeTexScroll(SIModel* im, int node, float u, float v)
{
    SModel* m = AsModel(im);
    if (!ValidNode(m, node))
        return;
    SModelNode& n = m->Nodes[node];
    n.TexAnim[0] = u;
    n.TexAnim[2] = v;
    if (n.TexAnimType != 1) {
        n.TexAnimType = 1;
        n.TexAnim[1] = u;
        n.TexAnim[3] = v;
    }
}

// PANZERS 0x6d78c0
int AnimModelSequenceType(SIModel* im)
{
    SModel* m = AsModel(im);
    if (!m || m->Anim.Seq < 0 || m->Anim.Seq >= m->Proto->SequenceCount)
        return 0;                                                 // HD indexes without a check
    return m->Proto->Sequences[m->Anim.Seq].Type;
}

// PANZERS 0x6d7f30
float AnimModelSequenceLength(SIModel* im, const char* name)
{
    SModel* m = AsModel(im);
    int i = m ? m->Proto->FindSequence(name) : -1;                // 0x6d7f70
    if (i >= 0)
        return m->Proto->Sequences[i].Length;
    return 0.0f;
}

// PANZERS 0x6da2c0
void AnimModelSetColor2(SIModel* im, bool on, unsigned color)
{
    SModel* m = AsModel(im);
    m->Color2Override = on;
    m->Color2 = (int)color;
}

// PANZERS 0x67c0a0
int AnimProtoFindSequence(int proto, const char* name)
{
    SPModel* p = proto >= 0 ? GepardModelPrototype(proto) : nullptr;
    if (!p)
        return -1;
    for (int i = 0; i < p->SequenceCount; ++i) {
        const char* s = p->Sequences[i].Name.buf ? p->Sequences[i].Name.buf : "";
        if (_stricmp(s, name) == 0)
            return i;
    }
    return -1;
}

// PANZERS 0x67c170
int AnimProtoSequenceCount(int proto)
{
    SPModel* p = proto >= 0 ? GepardModelPrototype(proto) : nullptr;
    return p ? p->SequenceCount : 0;
}

const char* AnimProtoSequenceName(int proto, int index)
{
    SPModel* p = proto >= 0 ? GepardModelPrototype(proto) : nullptr;
    if (!p || index < 0 || index >= p->SequenceCount)
        return "";
    return p->Sequences[index].Name.buf ? p->Sequences[index].Name.buf : "";
}

} // namespace pz
