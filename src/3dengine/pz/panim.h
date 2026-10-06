// src/3dengine/pz/panim.h
// SPAnim: a stand-alone CANM v100 animation (.anim), referenced by the SREF
// entries of a .4d SSQS chunk. HD 0x2c bytes, ctor 0x68e210, dtor 0x68e440,
// loader SPAnim::LoadAnimFile 0x692d10. Owned by the Gepard facade heap
// (HD SGepard +0x564, loaded by 0x67d920, looked up by 0x67a760).
// OWNER: agent A.

#ifndef PZ_PANIM_H
#define PZ_PANIM_H

#include "pmodel.h"

namespace pz {

// CANM NODE (HD 0x6c bytes, "SPAnimNode"): the channel + the node name.
struct SPAnimNode {
    SPChannel Channel;   // +0x00
    SString   Name;      // +0x64
};
static_assert(sizeof(SPAnimNode) == 0x6c, "HD SPAnimNode stride 0x6c");

struct SPAnim {
    SString     Name;       // +0x00 file name
    int         Unknown08;  // +0x08 never written by HD (copied into SPSequence+8)
    float       Speed;      // +0x0c third header float
    float       BlendTime;  // +0x10 first header float
    float       Length;     // +0x14 second header float
    bool        FlyZ;       // +0x18 FLYZ chunk
    unsigned char _19[3];
    SPAnimNode* Nodes;      // +0x1c
    int         NodeCount;  // +0x20
    int         NodeMax;    // +0x24
    int         RefCount;   // +0x28

    SPAnim();                              // 0x68e210
    ~SPAnim();                             // 0x68e440
    bool LoadAnimFile(const char* file);   // 0x692d10
    int  AddNode();                        // 0x68ea60
};
static_assert(sizeof(SPAnim) == 0x2c, "HD operator new(0x2c)");

void FreeChannelTracks(SPChannel* c);      // 0x68e760

} // namespace pz

#endif // PZ_PANIM_H
