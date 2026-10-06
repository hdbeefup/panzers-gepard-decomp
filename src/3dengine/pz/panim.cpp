// src/3dengine/pz/panim.cpp
// SPAnim (CANM v100 stand-alone animations). OWNER: agent A. See panim.h.

#include <string.h>
#include <stdlib.h>
#include "panim.h"
#include "tracks.h"
#include "stream.h"
#include "logger.h"

extern SFileSystem FileSystem;

namespace pz {

// PANZERS 0x68e760
// Channel dtor: the five track pointers.
void FreeChannelTracks(SPChannel* c)
{
    delete c->PosTrack;
    delete c->RotTrack;
    delete c->Link;
    delete c->Fov;
    delete c->Visibility;
    c->PosTrack = nullptr;
    c->RotTrack = nullptr;
    c->Link = nullptr;
    c->Fov = nullptr;
    c->Visibility = nullptr;
}

// PANZERS 0x68e210
SPAnim::SPAnim()
{
    memset(this, 0, sizeof(*this));   // HD leaves +8..+0x18 unset; zeroed here
    RefCount = 1;
}

// PANZERS 0x68e440
SPAnim::~SPAnim()
{
    for (int i = 0; i < NodeCount; ++i) {
        FreeChannelTracks(&Nodes[i].Channel);
        delete[] Nodes[i].Name.buf;
    }
    free(Nodes);
    delete[] Name.buf;
}

// PANZERS 0x68ea60
int SPAnim::AddNode()
{
    if (NodeCount == NodeMax) {
        int n = NodeMax < 0x10 ? 0x10 : (NodeMax * 6) / 5;
        Nodes = (SPAnimNode*)realloc(Nodes, (size_t)n * sizeof(SPAnimNode));
        memset(Nodes + NodeMax, 0, (size_t)(n - NodeMax) * sizeof(SPAnimNode));
        NodeMax = n;
    }
    return NodeCount++;
}

// Track factories shared with SPModel::LoadSequences (chunk ids are HD's).
SITrackPosition* LoadPositionTrack(int id, SStream* is)
{
    switch (id) {
    case 0x534f5043: return new STrackPositionBezier(is);       // CPOS 0x6712a0
    case 0x5a595843: return new STrackPositionIndependent(is);  // CXYZ 0x671420
    case 0x50535043: return new STrackPositionPacked(is);       // CPSP 0x6715b0
    }
    return nullptr;
}

SITrackRotation* LoadRotationTrack(int id, SStream* is)
{
    switch (id) {
    case 0x544f5243: return new STrackRotationTCB(is);          // CROT 0x671980
    case 0x4c554543: return new STrackRotationEuler(is);        // CEUL 0x6716d0
    case 0x50554543: return new STrackRotationEulerPacked(is);  // CEUP 0x671860
    }
    return nullptr;
}

// PANZERS 0x692d10
bool SPAnim::LoadAnimFile(const char* file)
{
    SStream* is = FileSystem.OpenRead(file, "SPAnim::LoadAnimFile");
    if (!is)
        return false;
    FlyZ = false;
    is->ReadSignature();                                   // 0x65d6a0
    if (is->ReadChunkHeader() != 0x4d4e4143)               // CANM
        throw "Not a scene file";
    if (is->ReadInt() != 0x30303176)                       // v100
        throw "Unsupported Animation file version (%s)";
    BlendTime = is->ReadFloat();
    Length = is->ReadFloat();
    Speed = is->ReadFloat();
    for (;;) {
        if (is->ReadChunkIsEnd()) {
            is->ReadChunkValidate(0);
            is->Release();
            return true;
        }
        int id = is->ReadChunkHeader();
        if (id == 0x45444f4e) {                            // NODE
            int n = AddNode();
            SPAnimNode* node = &Nodes[n];
            node->Name.Load(is);                           // 0x56e7d0
            is->ReadInt();                                 // anim parent index (ignored by HD)
            is->Read(node->Channel.Pos, 0xc);
            is->Read(node->Channel.Quat, 0x10);
            is->Read(node->Channel.Matrix, 0x30);
            while (!is->ReadChunkIsEnd()) {
                int c = is->ReadChunkHeader();
                node = &Nodes[n];
                if (c == 0x534f5043 || c == 0x5a595843 || c == 0x50535043)
                    node->Channel.PosTrack = LoadPositionTrack(c, is);
                else if (c == 0x544f5243 || c == 0x4c554543 || c == 0x50554543)
                    node->Channel.RotTrack = LoadRotationTrack(c, is);
                else if (c == 0x45484e49)                  // INHE
                    node->Channel.Inherit = is->ReadInt();
                else if (c == 0x4b4e4c43)                  // CLNK
                    throw "Link track is not supported in stand alone anim files";
                else
                    throw "Unsupported track component";
                is->ReadChunkValidate(0);
            }
            is->ReadChunkValidate(0);
        } else if (id == 0x47484343) {                     // CCHG
            throw "Camera changes aren't supported in stand alone anim files";
        } else if (id == 0x5a594c46) {                     // FLYZ
            FlyZ = true;
            is->ReadChunkValidate(0);
        } else {
            throw "Unsupported node type";
        }
    }
}

} // namespace pz
