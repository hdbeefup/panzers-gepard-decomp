// src/3dengine/pz/pmodel.cpp
// SPModel: the HD .4d loader (SCEN v100 and v101). OWNER: agent A.
//
// v100 vs v101 (both handled by 0x6912b0, "local_2d" = old file):
//   - v101 nodes carry two more dwords after the parent index (+0x0c, +0x10:
//     a rotation-link node and a second link; -1 when unused). v100 nodes get
//     -1 for both.
//   - CAM_ nodes are rejected in v100 ("File is too old please re-export").
//   - Everything else (MESH/ANIM/SKIN/DUMY/POLY/BSP_/LITE, MTLS with
//     MATE/STRP + DIFF/SPEC/SILL/BUMP/REFL, VERT formats 0/1/2, FACE/INDI,
//     BONS, SEQS/FRM2, SSQS, FLYZ, AMBI) is version independent.
// HD quirk kept: the VERT scale loop multiplies the vertex NORMALS by the
// prototype scale (accessor 0x691200 reads the normal offset +0x24), not
// the positions; positions are scaled by the model matrix (SModel+0x94).
// Normals are renormalised by D3DRS_NORMALIZENORMALS (set by the scene).

#include <string.h>
#include <stdlib.h>
#include "pmodel.h"
#include "panim.h"
#include "mesh.h"
#include "tracks.h"
#include "pzgepard.h"
#include "ipixie.h"
#include "stream.h"
#include "logger.h"

namespace pz {

SITrackPosition* LoadPositionTrack(int id, SStream* is);   // panim.cpp
SITrackRotation* LoadRotationTrack(int id, SStream* is);

static const char* Str(const SString& s) { return s.buf ? s.buf : ""; }

// SString helpers (HD 0x5335c0 concat, 0x5336b0 substring, 0x597390 set
// extension, 0x5e4e00 directory part).
static void StrSet(SString* s, const char* v) { *s = v; }
static void StrFree(SString* s) { delete[] s->buf; s->buf = nullptr; s->size = 0; }

// PANZERS 0x597390
// Replaces the extension after the last '.' of the last path component, or
// appends ".ext" when there is none.
void SetExtension(SString* s, const char* ext)
{
    int dot = -1;
    for (int i = 0; i < s->size; ++i) {
        char c = s->buf[i];
        if (c == '/' || c == '\\')
            dot = -1;
        else if (c == '.')
            dot = i;
    }
    char tmp[512];
    int keep = dot >= 0 ? dot + 1 : s->size;
    if (keep > 500)
        keep = 500;
    memcpy(tmp, s->buf ? s->buf : "", keep);
    if (dot < 0)
        tmp[keep++] = '.';
    tmp[keep] = 0;
    strncat(tmp, ext, sizeof(tmp) - strlen(tmp) - 1);
    StrSet(s, tmp);
}

// PANZERS 0x68e250
SPModel::SPModel(int param)
{
    memset(this, 0, sizeof(*this));
    Param = param;
    RefCount = 1;
}

void FreeSequence(SPSequence* s, int nodeCount)
{
    if (s->Channels) {
        // SREF channels share their tracks with the SPAnim (copied by value).
        if (s->Anim < 0)
            for (int i = 0; i < nodeCount; ++i)
                FreeChannelTracks(&s->Channels[i]);
        operator delete(s->Channels);
    }
    operator delete(s->FrameTimes);
    delete s->CameraChanges;
    StrFree(&s->Name);
    if (s->Anim >= 0)
        GepardReleaseAnim(s->Anim);
}

// PANZERS 0x68e4c0
SPModel::~SPModel()
{
    if (RefCount != 0)
        Logger.g->Log(0, "SPModel::~SPModel: Deleted model prototype with nonzero reference-count: %s",
                      Str(FileName));
    for (int i = 0; i < NodeCount; ++i) {
        SPModelNode* n = &Nodes[i];
        // HD: pixie +0x20 releases the ':' node's effect prototype.
        if (n->Effect >= 0) {
            if (SIPixie* pixie = GepardPixie())
                pixie->ReleaseEffectPrototype(n->Effect);
            n->Effect = -1;
        }
        delete n->Mesh;
        delete n->Collision;
        operator delete(n->Bones);
        operator delete(n->Camera);
        operator delete(n->Light);
        StrFree(&n->Name);
    }
    free(Nodes);
    free(PolyNodes);
    if (Sequences) {
        for (int i = 0; i < SequenceCount; ++i)
            FreeSequence(&Sequences[i], NodeCount);
        operator delete(Sequences);
    }
    StrFree(&FileName);
    StrFree(&SequenceFile);
}

// PANZERS 0x68ead0
int SPModel::AddNode()
{
    if (NodeCount == NodeMax) {
        int n = NodeMax < 0x10 ? 0x10 : (NodeMax * 6) / 5;
        Nodes = (SPModelNode*)realloc(Nodes, (size_t)n * sizeof(SPModelNode));
        memset(Nodes + NodeMax, 0, (size_t)(n - NodeMax) * sizeof(SPModelNode));
        NodeMax = n;
    }
    return NodeCount++;
}

// PANZERS 0x6d7f70 (SModel helper; case-insensitive name lookup)
int SPModel::FindSequence(const char* name) const
{
    for (int i = 0; i < SequenceCount; ++i)
        if (!_stricmp(Str(Sequences[i].Name), name ? name : ""))
            return i;
    return -1;
}

static SPSequence* NewSequences(int count)
{
    SPSequence* s = (SPSequence*)operator new(sizeof(SPSequence) * (count ? count : 1));
    memset(s, 0, sizeof(SPSequence) * (count ? count : 1));
    for (int i = 0; i < count; ++i)
        s[i].Anim = -1;
    return s;
}

static SPChannel* NewChannels(int count)
{
    SPChannel* c = (SPChannel*)operator new(sizeof(SPChannel) * (count ? count : 1));
    memset(c, 0, sizeof(SPChannel) * (count ? count : 1));
    for (int i = 0; i < count; ++i) {   // 0x68e2e0 -> 0x671000
        c[i].Matrix[0] = c[i].Matrix[4] = c[i].Matrix[8] = 1.0f;
        c[i].Quat[3] = 1.0f;
    }
    return c;
}

// Reads one SEQ2 descriptor (HD inline in the SEQS case).
static void ReadFrameSequence(SStream* is, SPSequence* s)
{
    s->Name.Load(is);
    s->Unknown08 = is->ReadInt();
    s->Speed = is->ReadFloat();
    s->Next = is->ReadInt();
    s->Type = is->ReadInt();
    s->FirstFrame = is->ReadInt();
    s->FrameCount = is->ReadInt();
    s->FrameTimes = (float*)operator new((size_t)s->FrameCount * 4);
    is->Read(s->FrameTimes, s->FrameCount * 4);
    s->BlendTime = s->FrameTimes[0];
    s->Length = s->FrameTimes[s->FrameCount - 1];
    s->Anim = -1;
}

// PANZERS 0x6912b0
// dir: the model's directory with a trailing '/', prefixed to texture names.
bool SPModel::Load4DFile(const char* dir, const char* file, float scale)
{
    Logger.g->Log(0, "SPModel::Load4DFile: Loading 4D Scene file: %s", file);
    SStream* is = FileSystem.OpenRead(file, nullptr);
    if (!is) {
        Logger.g->Log(0, "SPModel::Load4DFile: Couldn't open %s", file);
        return false;
    }
    Scale = scale;
    FlyZ = false;
    is->ReadSignature();
    if (is->ReadChunkHeader() != 0x4e454353)   // SCEN
        throw "Not a scene file";
    int ver = is->ReadInt();
    bool old;
    if (ver == 0x31303176)        // v101
        old = false;
    else if (ver == 0x30303176)   // v100
        old = true;
    else
        throw "Unsupported 4D Scene file version (%s)";

    while (!is->ReadChunkIsEnd()) {
        SMesh* mesh = nullptr;
        char kind = 0;
        int n = -1;
        int id = is->ReadChunkHeader();
        switch (id) {
        case 0x4853454d: n = AddNode(); mesh = new SMesh(); kind = 'M'; break;        // MESH
        case 0x4d494e41: n = AddNode(); mesh = new SAnimesh(); kind = 'A'; break;     // ANIM
        case 0x4e494b53: n = AddNode(); mesh = new SSkinnedMesh(); kind = 'S'; break; // SKIN
        case 0x594d5544: n = AddNode(); kind = 'D'; break;                           // DUMY
        case 0x594c4f50: n = AddNode(); kind = 'P'; break;                           // POLY
        case 0x5f505342: n = AddNode(); kind = 'B'; break;                           // BSP_
        case 0x5f4d4143:                                                             // CAM_
            if (old)
                throw "File is too old please re-export";
            n = AddNode();
            Nodes[n].Camera = (SPCamera*)operator new(sizeof(SPCamera));
            memset(Nodes[n].Camera, 0, sizeof(SPCamera));
            kind = 'C';
            break;
        case 0x4554494c:                                                             // LITE
            n = AddNode();
            Nodes[n].Light = (SPLight*)operator new(sizeof(SPLight));
            memset(Nodes[n].Light, 0, sizeof(SPLight));
            kind = 'L';
            break;
        case 0x53515353: {                                                           // SSQS
            SString path;
            StrSet(&path, file);
            // 0x5e4e00 directory + 0x52c580 "/"
            int slash = -1;
            for (int i = 0; i < path.size - 1; ++i)
                if (path.buf[i] == '/' || path.buf[i] == '\\')
                    slash = i;
            char d[512];
            if (slash >= 0) {
                memcpy(d, path.buf, slash);
                d[slash] = 0;
            } else {
                strcpy(d, ".");
            }
            strcat(d, "/");
            LoadSequences(is, d);
            StrFree(&path);
            is->ReadChunkValidate(0);
            continue;
        }
        case 0x5a594c46:                                                             // FLYZ
            FlyZ = true;
            is->ReadChunkValidate(0);
            continue;
        case 0x49424d41:                                                             // AMBI
            Ambient[0] = is->ReadFloat();
            Ambient[1] = is->ReadFloat();
            Ambient[2] = is->ReadFloat();
            is->ReadChunkValidate(0);
            continue;
        default:
            Logger.g->Log(1, "Unsupported scene entry");   // 0x65cac0 (warning)
            is->ReadChunkSkip();
            is->ReadChunkValidate(0);
            continue;
        }

        SPModelNode* node = &Nodes[n];
        node->Mesh = mesh;
        node->Collision = nullptr;
        node->Name.Load(is);
        node->Parent = is->ReadInt();
        if (!old) {
            node->Link = is->ReadInt();
            node->Link2 = is->ReadInt();
        } else {
            node->Link = -1;
            node->Link2 = -1;
        }
        if (node->Parent < 0)
            Logger.g->Log(1, "    %c[%2d   ] %s", kind, n, Str(node->Name));
        else
            Logger.g->Log(1, "    %c[%2d:%2d] %s", kind, n, node->Parent, Str(node->Name));
        is->Read(node->Transform, 0x30);

        // Effect nodes: ":name" (looped) and "::name" (one shot) attach the
        // pixie effect "effects/<name>.fx".
        if (node->Name.size > 0 && node->Name.buf[0] == ':') {
            bool once = node->Name.size > 1 && node->Name.buf[1] == ':';
            char fx[512];
            strcpy(fx, "effects/");
            strncat(fx, node->Name.buf + (once ? 2 : 1), sizeof(fx) - 16);
            SString p;
            StrSet(&p, fx);
            SetExtension(&p, "fx");
            SIPixie* pixie = GepardPixie();   // HD g_Pixie 0x92f104 = SGepard+0x7f4
            node->Effect = pixie ? pixie->LoadEffectPrototype(Str(p), false, false, 0, 0) : -1;
            node->EffectOnce = once;
            StrFree(&p);
        } else {
            node->Effect = -1;
            node->EffectOnce = false;
        }

        if (kind == 'P' || kind == 'B') {
            SCollisionBase* c;
            if (kind == 'P')
                c = new SCollisionConvexPoly();
            else
                c = new SCollisionBSPTree();
            Nodes[n].Collision = c;
            c->Load(is);
        } else if (kind == 'C') {
            SPCamera* cam = Nodes[n].Camera;
            cam->Data[0] = is->ReadFloat();
            cam->Data[1] = is->ReadFloat() * scale;
            cam->Data[2] = is->ReadFloat() * scale;
            cam->HasCmfg = false;
            while (!is->ReadChunkIsEnd()) {
                if (is->ReadChunkHeader() != 0x47464d43)   // CMFG
                    throw "Unsupported camera node component";
                cam->HasCmfg = true;
                cam->Cmfg[0] = is->ReadFloat();
                cam->Cmfg[1] = is->ReadFloat();
                cam->Cmfg[2] = is->ReadFloat();
                cam->Cmfg[3] = is->ReadFloat() * scale;
                cam->Cmfg[4] = is->ReadFloat() * scale;
                is->ReadChunkValidate(0);
            }
        } else if (kind == 'L') {
            SPLight* l = Nodes[n].Light;
            if (is->ReadInt() != 0x30303176)
                throw "Unsupported light node version";
            l->Color[0] = is->ReadFloat();
            l->Color[1] = is->ReadFloat();
            l->Color[2] = is->ReadFloat();
            l->Intensity = is->ReadFloat();
            while (!is->ReadChunkIsEnd()) {
                int c = is->ReadChunkHeader();
                if (l->Type != 0 && (c == 0x45524944 || c == 0x544e4f50 || c == 0x544f5053))
                    throw "Multiple light node types";
                if (c == 0x45524944) {                     // DIRE
                    l->Type = 3;
                } else if (c == 0x544e4f50 || c == 0x544f5053) {   // PONT / SPOT
                    l->Type = c == 0x544e4f50 ? 1 : 2;
                    l->Range = is->ReadFloat() * Scale;
                    l->Falloff = is->ReadFloat();
                    l->Atten1 = is->ReadFloat() / Scale;
                    l->Atten2 = is->ReadFloat() / (Scale * Scale);
                    if (c == 0x544f5053) {
                        l->Theta = is->ReadFloat();
                        l->Phi = is->ReadFloat();
                    }
                } else {
                    throw "Unsupported light node component";
                }
                is->ReadChunkValidate(0);
            }
        } else if (!mesh) {
            // DUMY: only controller placeholders are accepted.
            while (!is->ReadChunkIsEnd()) {
                int c = is->ReadChunkHeader();
                if (c == 0x4e4f4352)                       // RCON
                    throw "File is too old please re-export";
                if (c != 0x534f5043 && c != 0x4b4e4c43 && c != 0x544f5243)   // CPOS CLNK CROT
                    throw "Unsupported node component";
                is->ReadChunkSkip();
                is->ReadChunkValidate(0);
            }
        } else {
            bool hasMaterial = false;
            while (!is->ReadChunkIsEnd()) {
                int c = is->ReadChunkHeader();
                node = &Nodes[n];
                switch (c) {
                case 0x534e4f42: {                         // BONS
                    node->BoneCount = is->ReadInt();
                    node->Bones = (SPBone*)operator new(sizeof(SPBone) * (node->BoneCount ? node->BoneCount : 1));
                    for (int b = 0; b < node->BoneCount; ++b) {
                        SPBone* bone = &node->Bones[b];
                        bone->Node = is->ReadInt();
                        memset(bone->Matrix, 0, sizeof(bone->Matrix));   // 0x691230
                        bone->Matrix[0] = bone->Matrix[4] = bone->Matrix[8] = 1.0f;
                        is->Read(bone->Matrix, 0x30);
                    }
                    break;
                }
                case 0x4b4e4c43:                           // CLNK
                case 0x544f5243:                           // CROT
                case 0x534f5043:                           // CPOS
                    is->ReadChunkSkip();
                    break;
                case 0x324d5246: {                         // FRM2: one "default" sequence
                    SequenceCount = 1;
                    FrameSequences = true;
                    Sequences = NewSequences(1);
                    SPSequence* s = &Sequences[0];
                    StrSet(&s->Name, "default");
                    s->Unknown08 = 0;
                    s->Next = 0;
                    s->FirstFrame = 0;
                    s->FrameCount = is->ReadInt();
                    s->FrameTimes = (float*)operator new((size_t)s->FrameCount * 4);
                    is->Read(s->FrameTimes, s->FrameCount * 4);
                    s->Speed = is->ReadFloat();
                    s->BlendTime = s->FrameTimes[0];
                    s->Length = s->FrameTimes[s->FrameCount - 1];
                    s->Anim = -1;
                    break;
                }
                case 0x45434146:                           // FACE
                case 0x49444e49: {                         // INDI
                    int count = is->ReadInt();
                    if (c == 0x45434146)
                        count *= 3;
                    unsigned short* idx = mesh->CreateIndexBuffer(count);
                    if ((unsigned)count > 0xffff) {
                        Logger.g->Panic("SPModel::Load4DFile: Too many indices");
                        return false;
                    }
                    is->Read(idx, count * 2);
                    mesh->UnlockIndexBuffer();
                    break;
                }
                case 0x4e4f4352:                           // RCON
                    throw "File is too old please re-export";
                case 0x534c544d: {                         // MTLS
                    unsigned count = (unsigned)is->ReadInt();
                    SMaterial* mats = mesh->CreateMaterials(count);
                    hasMaterial = true;
                    for (unsigned m = 0; m < count; ++m) {
                        int e = is->ReadChunkHeader();
                        if (e != 0x4554414d && e != 0x50525453)   // MATE / STRP
                            throw "Unsupported material entry";
                        SMaterial* mat = &mats[m];
                        mat->PrimType = (e == 0x50525453) + 4;
                        mat->PrimCount = is->ReadInt();
                        mat->MinIndex = is->ReadInt();
                        mat->NumVertices = is->ReadInt();
                        while (!is->ReadChunkIsEnd()) {
                            int t = is->ReadChunkHeader();
                            int slot;
                            switch (t) {
                            case 0x46464944: slot = 0; break;   // DIFF
                            case 0x43455053: slot = 1; break;   // SPEC
                            case 0x4c4c4953: slot = 2; break;   // SILL
                            case 0x504d5542: slot = 3; break;   // BUMP
                            case 0x4c464552: slot = 4; break;   // REFL
                            default: throw "Unsupported material component";
                            }
                            SString name;
                            name.Load(is);                      // 0x5910e0
                            char path[512];
                            strncpy(path, dir ? dir : "", sizeof(path) - 1);
                            path[sizeof(path) - 1] = 0;
                            strncat(path, Str(name), sizeof(path) - strlen(path) - 1);   // 0x5335c0
                            mat->SetTexture(slot, PzGepard()->LoadTexture(path, 1, true));   // Gepard +0x44
                            StrFree(&name);
                            is->ReadChunkValidate(0);
                        }
                        is->ReadChunkValidate(0);
                    }
                    break;
                }
                case 0x53514553: {                         // SEQS
                    int count = is->ReadInt();
                    FrameSequences = true;
                    SequenceCount = count;
                    Sequences = NewSequences(count);
                    for (int s = 0; s < SequenceCount; ++s) {
                        if (is->ReadChunkHeader() != 0x32514553) {   // SEQ2
                            Logger.g->Panic("SPModel::Load4DFile: Expected sequence descriptor");
                            return false;
                        }
                        ReadFrameSequence(is, &Sequences[s]);
                        is->ReadChunkValidate(0);
                    }
                    SortSequences();
                    break;
                }
                case 0x54524556: {                         // VERT
                    unsigned count = (unsigned)is->ReadInt();
                    if (count > 0xffff) {
                        Logger.g->Panic("SPModel::Load4DFile: Too many vertices");
                        return false;
                    }
                    int format = is->ReadInt();
                    if (format == 0) {
                        mesh->CreateVertexBuffer(0x112, count);
                        is->Read(mesh->Vertices + mesh->OffPosition, count << 5);
                        for (unsigned v = 0; v < count; ++v) {
                            float* nrm = mesh->VertexNormal(v);
                            nrm[0] = nrm[0] * scale;
                            nrm[1] = scale * nrm[1];
                            nrm[2] = scale * nrm[2];
                        }
                    } else if (format == 1) {
                        mesh->CreateVertexBuffer(0x2102, count);
                        is->Read(mesh->Vertices + mesh->OffPosition, count * 0x38);
                        // HD then assembles a vs.1.0 / ps.1.1 bump shader
                        // source into two SStrings and frees them unused.
                    } else if (format == 2) {
                        is->ReadInt();   // discarded
                        mesh->CreateVertexBuffer(0x4112, count);
                        is->Read(mesh->Vertices + mesh->OffPosition, count * 0x34);
                        for (unsigned v = 0; v < count; ++v) {
                            float* nrm = mesh->VertexNormal(v);
                            nrm[0] = nrm[0] * scale;
                            nrm[1] = nrm[1] * scale;
                            nrm[2] = nrm[2] * scale;
                        }
                    } else {
                        Logger.g->Panic("SPModel::Load4DFile: Unknown vertex format");
                        return false;
                    }
                    mesh->Unlock();
                    break;
                }
                case 0x584f4242:                           // BBOX
                    is->Read(node->BBox, 0x60);
                    break;
                default:
                    throw "Unsupported mesh component";
                }
                is->ReadChunkValidate(0);
            }
            if (!hasMaterial)
                throw "Mesh has no material";
        }
        is->ReadChunkValidate(0);
    }
    is->ReadChunkValidate(0);
    is->Release();

    // Collect the nodes with a collision object.
    PolyNodeCount = 0;
    for (int i = 0; i < NodeCount; ++i) {
        if (!Nodes[i].Collision)
            continue;
        if (PolyNodeCount == PolyNodeMax) {
            int m = PolyNodeMax < 0x10 ? 0x10 : (PolyNodeMax * 6) / 5;
            PolyNodes = (int*)realloc(PolyNodes, m * sizeof(int));
            memset(PolyNodes + PolyNodeMax, 0, (m - PolyNodeMax) * sizeof(int));
            PolyNodeMax = m;
        }
        PolyNodes[PolyNodeCount++] = i;
    }
    return true;
}

// PANZERS 0x693550
// SSQS: count, then per sequence either SSQE (inline: header, one SCON per
// node with controller tracks, CCHG camera changes) or SREF (name, .anim file
// relative to the model directory, next, type).
void SPModel::LoadSequences(SStream* is, const char* dir)
{
    FrameSequences = false;
    SequenceCount = is->ReadInt();
    Sequences = NewSequences(SequenceCount);
    for (int s = 0; s < SequenceCount; ++s) {
        SPSequence* seq = &Sequences[s];
        int id = is->ReadChunkHeader();
        if (id == 0x45515353) {                                    // SSQE
            seq->Name.Load(is);
            seq->Unknown08 = is->ReadInt();
            seq->Speed = is->ReadFloat();
            seq->Next = is->ReadInt();
            seq->Type = is->ReadInt();
            seq->BlendTime = is->ReadFloat();
            seq->Length = is->ReadFloat();
            seq->Channels = NewChannels(NodeCount);
            seq->Anim = -1;
            for (int n = 0; n < NodeCount; ++n) {
                if (is->ReadChunkHeader() != 0x4e4f4353) {          // SCON
                    Logger.g->Panic("SPModel::LoadSequences: Expected sequence controller descriptor");
                    return;
                }
                SPChannel* ch = &seq->Channels[n];
                if (is->ReadChunkRemain() == 0) {                   // 0x65d3f0
                    memcpy(ch->Matrix, Nodes[n].Transform, 0x30);
                } else {
                    is->Read(ch->Pos, 0xc);
                    is->Read(ch->Quat, 0x10);
                    is->Read(ch->Matrix, 0x30);
                    while (!is->ReadChunkIsEnd()) {
                        int c = is->ReadChunkHeader();
                        switch (c) {
                        case 0x534f5043: case 0x5a595843: case 0x50535043:
                            ch->PosTrack = LoadPositionTrack(c, is);
                            break;
                        case 0x544f5243: case 0x4c554543: case 0x50554543:
                            ch->RotTrack = LoadRotationTrack(c, is);
                            break;
                        case 0x4b4e4c43:                            // CLNK
                            ch->Link = new STrackLink();
                            ch->Link->Load(is);
                            break;
                        case 0x45484e49:                            // INHE
                            ch->Inherit = is->ReadInt();
                            break;
                        case 0x49534956:                            // VISI
                            ch->Visibility = new STrackBool();
                            ch->Visibility->Load(is);
                            break;
                        case 0x544e494c:                            // LINT
                        case 0x564f4643:                            // CFOV
                            ch->Fov = new SKeyTrackFloat();
                            ch->Fov->Load(is);
                            break;
                        default:
                            throw "Unsupported sequence per-node component";
                        }
                        is->ReadChunkValidate(0);
                    }
                }
                is->ReadChunkValidate(0);
            }
            while (!is->ReadChunkIsEnd()) {
                if (is->ReadChunkHeader() != 0x47484343)            // CCHG
                    throw "Unsupported sequence global component";
                seq->CameraChanges = new STrackCameraChange();
                seq->CameraChanges->Load(is);
                is->ReadChunkValidate(0);
            }
        } else {
            if (id != 0x46455253) {                                 // SREF
                Logger.g->Panic("SPModel::LoadSequences: Expected sequence descriptor");
                return;
            }
            seq->Name.Load(is);
            SString rel;
            rel.Load(is);
            char path[512];
            strncpy(path, dir, sizeof(path) - 1);
            path[sizeof(path) - 1] = 0;
            strncat(path, Str(rel), sizeof(path) - strlen(path) - 1);
            int anim = GepardLoadAnim(path);                       // 0x67d920
            SPAnim* a = GepardAnim(anim);                           // 0x67a760
            if (!a) {
                Logger.g->Log(0, "SPModel::LoadSequences: %s: animation not loaded", path);
                seq->Next = is->ReadInt();
                seq->Type = is->ReadInt();
                seq->Channels = NewChannels(NodeCount);
                for (int n = 0; n < NodeCount; ++n)
                    memcpy(seq->Channels[n].Matrix, Nodes[n].Transform, 0x30);
                seq->Anim = -1;
                StrFree(&rel);
                is->ReadChunkValidate(0);
                continue;
            }
            seq->Unknown08 = a->Unknown08;
            seq->Speed = a->Speed;
            seq->Next = is->ReadInt();
            seq->Type = is->ReadInt();
            seq->BlendTime = a->BlendTime;
            seq->Length = a->Length;
            seq->Channels = NewChannels(NodeCount);   // 0x78c879 calloc
            seq->Anim = anim;
            for (int n = 0; n < NodeCount; ++n)
                memcpy(seq->Channels[n].Matrix, Nodes[n].Transform, 0x30);
            // Each CANM node replaces the channel of the model node with the
            // same name (length then case-insensitive compare). The channel
            // is copied by value: its tracks stay owned by the SPAnim.
            for (int k = 0; k < a->NodeCount; ++k) {
                SPAnimNode* an = &a->Nodes[k];
                int n = 0;
                for (; n < NodeCount; ++n) {
                    if (Nodes[n].Name.size == an->Name.size &&
                        (Nodes[n].Name.size == 0 || !_stricmp(Str(Nodes[n].Name), Str(an->Name))))
                        break;
                }
                if (n >= NodeCount) {
                    Logger.g->Panic("SPModel::LoadSequences: Node '%s' not found in animation", Str(an->Name));
                    break;
                }
                memcpy(&seq->Channels[n], &an->Channel, sizeof(SPChannel));
            }
            StrFree(&rel);
        }
        is->ReadChunkValidate(0);
    }
    SortSequences();
}

// PANZERS 0x693f60
// Selection sort by name (case-insensitive), fixing the Next indices.
void SPModel::SortSequences()
{
    for (int i = 0; i + 1 < SequenceCount; ++i) {
        int best = i;
        for (int j = i + 1; j < SequenceCount; ++j)
            if (_stricmp(Str(Sequences[j].Name), Str(Sequences[best].Name)) < 0)
                best = j;
        if (best == i)
            continue;
        for (int k = 0; k < SequenceCount; ++k) {
            if (Sequences[k].Next == i)
                Sequences[k].Next = best;
            else if (Sequences[k].Next == best)
                Sequences[k].Next = i;
        }
        // HD swaps the 0x38-byte records raw. A member-wise C++ swap would
        // run SString::operator= (a deep copy that frees the old buffer,
        // which the temporary still points at) and corrupt the names.
        unsigned char t[sizeof(SPSequence)];
        memcpy(t, &Sequences[i], sizeof(SPSequence));
        memcpy((void*)&Sequences[i], &Sequences[best], sizeof(SPSequence));
        memcpy((void*)&Sequences[best], t, sizeof(SPSequence));
    }
}

// ---- collision objects ----

SCollisionBSPTree::SCollisionBSPTree()   // 0x6d3490
{
    AabbVtbl = nullptr;
    memset(Aabb, 0, sizeof(Aabb));
    SphereVtbl = nullptr;
    memset(Sphere, 0, sizeof(Sphere));
    Nodes = nullptr;
}

SCollisionBSPTree::~SCollisionBSPTree()  // 0x6d3690
{
    operator delete(Nodes);
}

// PANZERS 0x6d37b0
void SCollisionBSPTree::Load(SStream* is)
{
    while (!is->ReadChunkIsEnd()) {
        int c = is->ReadChunkHeader();
        if (c == 0x444e4f42) {                      // BOND
            for (int i = 0; i < 6; ++i)
                Aabb[i] = is->ReadFloat();
            for (int i = 0; i < 4; ++i)
                Sphere[i] = is->ReadFloat();
        } else if (c == 0x45525442) {               // BTRE
            int n = is->ReadInt();
            if (n > 0xffff)
                Logger.g->Panic("SCollisionBSPTree::Load: Too many BSP nodes");
            if (is->ReadInt() != 0)
                Logger.g->Panic("SCollisionBSPTree::Load: Unsupported BSP tree format");
            Nodes = operator new((size_t)n * 0x14 + 1);
            is->Read(Nodes, n * 0x14);
        } else {
            throw "Unsupported poly component";
        }
        is->ReadChunkValidate(0);
    }
}

SCollisionConvexPoly::SCollisionConvexPoly()   // 0x6d3500
{
    AabbVtbl = nullptr;
    memset(Aabb, 0, sizeof(Aabb));
    SphereVtbl = nullptr;
    memset(Sphere, 0, sizeof(Sphere));
    Verts = nullptr;
    VertCount = 0;
    Indices = nullptr;
    IndexCount = 0;
    Faces = nullptr;
    FaceCount = 0;
}

SCollisionConvexPoly::~SCollisionConvexPoly()  // 0x6d35f0
{
    operator delete(Verts);
    operator delete(Indices);
    operator delete(Faces);
}

// PANZERS 0x6d3950
void SCollisionConvexPoly::Load(SStream* is)
{
    while (!is->ReadChunkIsEnd()) {
        int c = is->ReadChunkHeader();
        if (c == 0x49444e49) {                      // INDI
            IndexCount = is->ReadInt();
            Indices = (unsigned short*)operator new((size_t)IndexCount * 2 + 2);
            if (IndexCount > 0xffff)
                Logger.g->Panic("SCollisionConvexPoly::Load: Too many indices");
            is->Read(Indices, IndexCount * 2);
        } else if (c == 0x4341464e) {               // NFAC
            FaceCount = is->ReadInt();
            Faces = operator new((size_t)FaceCount * 0x14 + 1);
            if (is->ReadInt() != 0)
                Logger.g->Panic("SCollisionConvexPoly::Load: Unsupported polygon type");
            is->Read(Faces, FaceCount * 0x14);
        } else if (c == 0x444e4f42) {               // BOND
            for (int i = 0; i < 6; ++i)
                Aabb[i] = is->ReadFloat();
            for (int i = 0; i < 4; ++i)
                Sphere[i] = is->ReadFloat();
        } else if (c == 0x54524556) {               // VERT
            VertCount = is->ReadInt();
            if (VertCount > 0xffff)
                Logger.g->Panic("SCollisionConvexPoly::Load: Too many vertices");
            if (is->ReadInt() != 0)
                Logger.g->Panic("SCollisionConvexPoly::Load: Unsupported vertex type");
            Verts = (float*)operator new((size_t)VertCount * 12 + 1);
            is->Read(Verts, VertCount * 12);
        } else {
            throw "Unsupported poly component";
        }
        is->ReadChunkValidate(0);
    }
}

// ---- collision tests (M3-I) ----

// PANZERS 0x6d40e0
// SCollisionAABB +0x08: min <= p < max on each axis.
static bool AabbTestPoint(const float* b, const float* p)
{
    return p[0] >= b[0] && b[1] > p[0] && p[1] >= b[2] && b[3] > p[1] && p[2] >= b[4] && b[5] > p[2];
}

// PANZERS 0x6d3c90
// SCollisionSphere +0x10: whether the segment a-b passes within the radius
// (sphere {cx, cy, cz, r^2}).
static bool SphereTestLineSection(const float* s, const float* a, const float* b)
{
    float fy = s[1] - a[1], fx = s[0] - a[0], fz = s[2] - a[2];
    float ex = b[0] - s[0], ey = b[1] - s[1], ez = b[2] - s[2];
    float dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
    float dot = (dy * fy + dx * fx) + dz * fz;
    if (0.0f > dot)
        return s[3] > (fy * fy + fx * fx) + fz * fz;
    float dot2 = (dy * ey + dx * ex) + dz * ez;
    if (0.0f > dot2)
        return s[3] > (ey * ey + ex * ex) + ez * ez;
    float ly = a[1] - b[1], lx = a[0] - b[0], lz = a[2] - b[2];
    float len2 = (lx * lx + ly * ly) + lz * lz;
    float d2 = (fy * fy + fx * fx) + fz * fz;
    return s[3] * len2 > d2 * len2 - dot * dot;
}

// BSP node (0x14 bytes): u16 front (+0), u16 back (+2), plane a, b, c, d.
struct SBspNode {
    unsigned short Front;
    unsigned short Back;
    float A, B, C, D;
};
static_assert(sizeof(SBspNode) == 0x14, "HD BSP node");

static inline float BspSide(const SBspNode& n, const float* p)
{
    return ((n.B * p[1] + n.A * p[0]) + n.C * p[2]) + n.D;
}

// PANZERS 0x6d4130
// Inside when the walk ends on a front (solid) leaf: behind a plane (<= 0)
// go to the front child, else to the back child; child 0 ends the walk.
bool SCollisionBSPTree::TestPoint(const float* p)
{
    if (!AabbTestPoint(Aabb, p))                                  // +0x04 -> +0x08 (0x6d40e0)
        return false;
    const SBspNode* nodes = (const SBspNode*)Nodes;               // +0x34
    unsigned n = 0;
    for (;;) {
        while (BspSide(nodes[n], p) <= 0.0f) {
            n = nodes[n].Front;
            if (n == 0)
                return true;
        }
        n = nodes[n].Back;
        if (n == 0)
            return false;
    }
}

// PANZERS 0x6d3e40
// The first solid node the segment a-b reaches from `node` (-1: none);
// `hit` is the node the segment came through.
int SCollisionBSPTree::FirstHit(const float* a, const float* b, int node, int hit)
{
    const SBspNode* nodes = (const SBspNode*)Nodes;
    unsigned n = (unsigned)node;
    float da, db;
    for (;;) {
        da = BspSide(nodes[n], a);
        db = BspSide(nodes[n], b);
        if (da > 0.0f && db > 0.0f) {
            n = nodes[n].Back;
            if (n == 0)
                return -1;
            continue;
        }
        if (0.0f > da && 0.0f > db) {
            unsigned f = nodes[n].Front;
            if (f == 0)
                return hit < 0 ? (int)n : hit;
            n = f;
            continue;
        }
        break;
    }
    float t = 0.0f;
    if (!(da == db)) {                                            // ucomiss: unordered computes too
        float r = da / (da - db);
        if (r > 0.0f) {
            if (!(1.0f > r))
                t = 1.0f;
            else
                t = r;
        }
    }
    float mid[3];
    mid[0] = (b[0] - a[0]) * t + a[0];
    mid[1] = (b[1] - a[1]) * t + a[1];
    mid[2] = (b[2] - a[2]) * t + a[2];
    if (da > db) {
        unsigned k = nodes[n].Back;
        if (k != 0) {
            int r = FirstHit(a, mid, (int)k, hit);
            if (r >= 0)
                return r;
        }
        k = nodes[n].Front;
        if (k == 0)
            return (int)n;
        return FirstHit(mid, b, (int)k, (int)n);
    }
    unsigned k = nodes[n].Front;
    if (k != 0) {
        int r = FirstHit(a, mid, (int)k, hit);
        if (r >= 0)
            return r;
    } else if (hit >= 0) {
        return hit;
    }
    k = nodes[n].Back;
    if (k == 0)
        return -1;
    return FirstHit(mid, b, (int)k, (int)n);
}

// PANZERS 0x6d3c00
bool SCollisionBSPTree::TestLineSection(const float* a, const float* b)
{
    if (!SphereTestLineSection(Sphere, a, b))                     // +0x20 -> +0x10 (0x6d3c90)
        return false;
    return FirstHit(a, b, 0, -1) >= 0;
}

// PANZERS 0x6d41d0
// Inside the AABB and behind (<= 0) every face plane (faces 0x14 bytes, the
// plane at +0x04).
bool SCollisionConvexPoly::TestPoint(const float* p)
{
    if (!AabbTestPoint(Aabb, p))
        return false;
    const unsigned char* f = (const unsigned char*)Faces;         // +0x44
    for (int i = 0; i < FaceCount; ++i) {
        const float* pl = (const float*)(f + i * 0x14 + 4);
        if (0.0f < ((pl[1] * p[1] + pl[0] * p[0]) + pl[2] * p[2]) + pl[3])
            return false;
    }
    return true;
}

// PANZERS 0x6d3c70
bool SCollisionConvexPoly::TestLineSection(const float*, const float*)
{
    Logger.g->Panic("SCollisionConvexPoly::TestLineSection: not implemented");
    return false;
}

} // namespace pz
