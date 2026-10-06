// src/world/world.h
// pz::SWorld: the Panzers world (HD 0x7538 bytes, vftable 0x7ffef8, 5 slots).
// OWNER: agent D. Skeleton from P0: logged stubs; the constructor already
// creates the scene through the Gepard facade so the 3D path can be tested.

#ifndef PZ_WORLD_H
#define PZ_WORLD_H

#include "pz/pzcommon.h"

struct SStream;

namespace pz {

struct SIViewport;

struct SWorld {
    explicit SWorld(int p1);                                   // 0x5d2f90
    virtual ~SWorld();                                         // +0x00 HD 0x5d68b0 (scalar deleting dtor)
    virtual void Slot_04();                                    // +0x04 HD 0x5ec2a0
    virtual void Slot_08();                                    // +0x08 HD 0x5fec80
    virtual void Slot_0C();                                    // +0x0c HD 0x5ec0c0
    virtual void Slot_10();                                    // +0x10 HD 0x5ebee0

    void ShowLoadingIcon(int parentFrame);                     // 0x5edca0 "menu/panzers_loading_icons_hq.tga"
    void HideLoadingIcon();                                    // 0x5dc7d0
    bool LoadMap(SStream* stream, bool p2, int p3, int p4);    // 0x5f1990 (MAPF v201 chunk loop)
    void Initialize();                                         // 0x5eec90
    void ComputeCamera(SIViewport* vp);                        // 0x5ddc30

    unsigned char _04[0x7538 - 0x04];
};
PZ_HD_SIZE(SWorld, kHdSizeSWorld);

} // namespace pz

#endif // PZ_WORLD_H
