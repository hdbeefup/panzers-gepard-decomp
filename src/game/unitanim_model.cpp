// src/game/unitanim_model.cpp
// The model calls of the unit animations (see unitanim_model.h): thin
// wrappers over the named SIModel slots (bodies in pzmodel.cpp), and the
// SGepard prototype sequence queries.

#include <string.h>
#include "unitanim_model.h"
#include "pz/imodel.h"
#include "pz/pzmodel.h"
#include "pz/pzgepard.h"

namespace pz {

void AnimModelSetNodeFade(SIModel* m, bool show, int node, int node2)
{
    m->SetNodeFade(show, node, node2);                            // +0x38 0x6db2a0
}

void AnimModelSetNodeTilt(SIModel* m, int node, float x, float y, float z, float yaw, float tiltX, float tiltZ)
{
    m->SetNodeTilt(node, x, y, z, yaw, tiltX, tiltZ);             // +0x48 0x6da4d0
}

void AnimModelSetNodeRotation(SIModel* m, int node, float x, float y, float z, float a, float b)
{
    m->SetNodeRotation(node, x, y, z, a, b);                      // +0x4c 0x6da330
}

void AnimModelGetNodePosition(SIModel* m, int node, float* xyz)
{
    m->GetNodePosition(node, xyz);                                // +0x54 0x6d7d30
}

void AnimModelSetNodeTexRotation(SIModel* m, int node, float u, float v, float angle)
{
    m->SetNodeTexRotation(node, u, v, angle);                     // +0x64 0x6da810
}

void AnimModelSetNodeTexScroll(SIModel* m, int node, float u, float v)
{
    m->SetNodeTexScroll(node, u, v);                              // +0x68 0x6da7a0
}

int AnimModelSequenceType(SIModel* m)
{
    return m->GetSequenceType();                                  // +0x78 0x6d78c0
}

float AnimModelSequenceLength(SIModel* m, const char* name)
{
    return m->GetSequenceLength(name);                            // +0x84 0x6d7f30
}

void AnimModelSetColor2(SIModel* m, bool on, unsigned color)
{
    m->SetColor2(on, color);                                      // +0xf0 0x6da2c0
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
