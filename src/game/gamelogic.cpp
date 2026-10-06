// src/game/gamelogic.cpp
// pz::SGameLogic: construction, the 20 Hz Refresh (single-player path), the
// frame stream with the world CRC, animated one-shot models and the per-frame
// unit visuals. OWNER: agent L. Lifted from the HD exe.
//
// The default (M1) Refresh path is unchanged; -m2 / PZ_M2=1 runs RefreshM2,
// the single-player tick of 0x576d80 (see gamelogic.h for the order). The
// per-unit loops call SIUnit only when the unit heap holds agent U's units
// (triggersunits.h); with the M1 stand-ins SWorld::RefreshModels keeps the
// M1 visuals running instead.

#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "trigger.h"
#include "triggersunits.h"
#include "worldapi.h"
#include "world.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "core_common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

void DumpTriggers(SWorld* w);   // trigger.cpp (recompile only)
void StandInMove(SGameLogic* gl);   // movementgroup.cpp (recompile only)

namespace m2u {

// Side table of the stand-in units (see triggersunits.h). Keyed by heap slot;
// a slot whose unit pointer changed is reset to the HD defaults of a new unit
// (+0x254 = -1, +0x258 = 0).
struct StandInSlot {
    void* Unit;
    StandInState State;
};
static StandInSlot* s_StandIn;
static int s_StandInMax;

StandInState* StandIn(int i)
{
    if (i >= s_StandInMax) {
        int n = s_StandInMax ? s_StandInMax : 64;
        while (n <= i)
            n *= 2;
        s_StandIn = (StandInSlot*)realloc(s_StandIn, n * sizeof(StandInSlot));
        memset(s_StandIn + s_StandInMax, 0, (n - s_StandInMax) * sizeof(StandInSlot));
        s_StandInMax = n;
    }
    void* u = (void*)W()->Units.Array[i].Unit;
    if (s_StandIn[i].Unit != u) {
        s_StandIn[i].Unit = u;
        memset(&s_StandIn[i].State, 0, sizeof(s_StandIn[i].State));
        s_StandIn[i].State.MovementGroup = -1;
        s_StandIn[i].State.Follow = -1;
    }
    return &s_StandIn[i].State;
}

} // namespace m2u

using m2u::UV;

// ---------------------------------------------------------------------------
// The world CRC (determinism oracle, docs/M2_INTERFACES.md 8)

static inline unsigned Rotl1(unsigned c) { return (c << 1) | (c >> 31); }

// HD 0x56aa10 body ( the unit heap and the seed are parameters so the
// self-test below can feed it a captured state)
// Starts from the frame and, for each live unit slot i in heap order, folds
// rotl1(c) ^ x over: i, +0xfc player, +0x8c/+0x90/+0x94 position, +0xb0 dir,
// +0x114, +0x108, +0x1dc. Ends with rotl1(c) ^ World+0x7518 (random seed).
static unsigned WorldCrc(unsigned frame, const SUnitHeap::Elem* units, int size, unsigned seed)
{
    unsigned c = frame;
    for (int i = 0; i < size; ++i) {
        if (units[i].Next != kHeapLive)
            continue;
        const unsigned char* u = (const unsigned char*)(const void*)units[i].Unit;
        static const int kFields[8] = { 0xfc, 0x8c, 0x90, 0x94, 0xb0, 0x114, 0x108, 0x1dc };
        c = Rotl1(c) ^ (unsigned)i;
        for (int k = 0; k < 8; ++k) {
            unsigned v;
            memcpy(&v, u + kFields[k], 4);
            c = Rotl1(c) ^ v;
        }
    }
    return Rotl1(c) ^ seed;
}

// PANZERS 0x56aa10
unsigned SGameLogic::ComputeWorldCRC()
{
    PZ_M2_TRACE("SGameLogic::ComputeWorldCRC (0x56aa10)");
    SWorld* w = g_World;
    if (!w)
        return 0;
    if (!UV::kReal) {
        // M1 stand-ins have no HD unit layout: hash the frame and the seed
        // only (the unit count still goes to the CRC log line).
        return Rotl1((unsigned)Frame) ^ w->RandomSeed;
    }
    return WorldCrc((unsigned)Frame, w->Units.Array, w->Units.Size, w->RandomSeed);
}

// Recompile-only self-test (PZ_M2_CRCTEST=1): tick 0 and tick 1 of the
// original menu, from the DynamoRIO trace (docs/re/M2_COVERAGE.md 5;
// m2p0/crc1/m2crc.19148.txt): slot index, player, pos x/y/z and dir as raw
// bits. The three fields the trace does not record are the HD values of an
// idle, unhurt unit: +0x114 = 1.0f, +0x108 = 0, +0x1dc = 0 (the only common
// solution of ticks 0..20, where nothing moves).
struct SCrcTestUnit { int Idx, Player; unsigned X, Y, Z, Dir; };
static const SCrcTestUnit kCrcTick0[] = {
    { 0, 0, 0x42aec6bd, 0x3c9818d1, 0x42c2a83f, 0xbfc42a14 }, { 1, 0, 0x42b67c2b, 0, 0x42b12724, 0xbe99999c },
    { 2, 0, 0x42b67c2b, 0, 0x42b12724, 0xbe99999c }, { 3, 0, 0x42b61282, 0, 0x42bb90c0, 0x402a3d70 },
    { 4, 0, 0x42b61282, 0, 0x42bb90c0, 0x402a3d70 }, { 5, 0, 0x42cdd4dd, 0, 0x42b1770f, 0xbf9eb853 },
    { 6, 0, 0, 0, 0, 0 }, { 7, 0, 0x3ed8347c, 0, 0x3f1c5cb3, 0 },
    { 8, 0, 0x3f3abd66, 0, 0xbf042930, 0 }, { 9, 0, 0xbf0298c6, 0, 0xbeb0c058, 0 },
    { 10, 0, 0xbed84698, 0, 0x3e92bbb9, 0 }, { 11, 0, 0x42c5ef10, 0, 0x429ae6ef, 0 },
    { 12, 0, 0x42c6c947, 0, 0x429aa95f, 0 }, { 13, 0, 0x42c534c2, 0, 0x429b0ce7, 0 },
    { 14, 0, 0x42c5feb6, 0, 0x42a6e494, 0 }, { 15, 0, 0x42c6af17, 0, 0x42a6add9, 0 },
    { 16, 0, 0x42c5478c, 0, 0x42a6c9d6, 0 }, { 17, 0, 0x42c5f6c4, 0, 0x429ef3b7, 0 },
    { 18, 0, 0x42c6bf17, 0, 0x429ecd21, 0 }, { 19, 0, 0x42c528dc, 0, 0x429f1380, 0 },
    { 20, 0, 0x42c6083a, 0, 0x42a2d783, 0 }, { 21, 0, 0x42c6defa, 0, 0x42a2825c, 0 },
    { 22, 0, 0x42c526eb, 0, 0x42a27ced, 0 }, { 23, 0, 0x42b69922, 0, 0x42ccd458, 0x3f666666 },
    { 24, 0, 0x42b69922, 0, 0x42ccd458, 0x3f666666 }, { 25, 0, 0x42b44cd9, 0, 0x42c0a36a, 0x3fbd70a4 },
    { 26, 0, 0x42b44cd9, 0, 0x42c0a36a, 0x3fbd70a4 }, { 27, 0, 0x42b3d776, 0, 0x42b7725a, 0x40028f5c },
    { 28, 0, 0x42b3d776, 0, 0x42b7725a, 0x40028f5c },
};

static bool CrcSelfTest()
{
    const int n = (int)(sizeof(kCrcTick0) / sizeof(kCrcTick0[0]));
    unsigned char* blocks = (unsigned char*)calloc(n, 0x200);
    SUnitHeap::Elem heap[n];
    for (int k = 0; k < n; ++k) {
        unsigned char* u = blocks + k * 0x200;
        const SCrcTestUnit& t = kCrcTick0[k];
        unsigned one = 0x3f800000;
        memcpy(u + 0xfc, &t.Player, 4);
        memcpy(u + 0x8c, &t.X, 4);
        memcpy(u + 0x90, &t.Y, 4);
        memcpy(u + 0x94, &t.Z, 4);
        memcpy(u + 0xb0, &t.Dir, 4);
        memcpy(u + 0x114, &one, 4);
        heap[t.Idx].Next = kHeapLive;
        heap[t.Idx].Unit = (decltype(heap[0].Unit))(void*)u;
    }
    unsigned crc = WorldCrc(0, heap, n, 0xf628b05du);
    free(blocks);
    bool ok = crc == 0xcc5c92d8u;    // docs/re/m2_crc_original.txt "T 0 cc5c92d8 f628b05d 29"
    if (Logger.g)
        Logger.g->Log(0, "PZM2 CRCTEST tick 0: %08x (original cc5c92d8) %s", crc, ok ? "PASS" : "FAIL");
    return ok;
}

// ---------------------------------------------------------------------------
// Construction

// PANZERS 0x55e440 (single-player path; see the notes on the parts left out)
SGameLogic::SGameLogic(int p1, int p2, int p3)
{
    PZ_TRACE("SGameLogic::SGameLogic (0x55e440)");
    // HD writes the fields one by one and leaves the rest as heap garbage;
    // the recompile clears the object first.
    memset(this, 0, sizeof(*this));
    CrcTop = -1;
    MovementGroupFree = -1;
    _308 = -1;
    if (g_GameLogic) {
        // HD: 0x55fe00 + operator delete(0x318) of the previous instance.
        delete g_GameLogic;
    }
    g_GameLogic = this;                                           // DAT_008f2078
    Frame = 0;
    _014 = -0x12f;
    FrameObject = nullptr;
    InFrameSync = false;
    FramesSent = 0;
    if (g_M2.Enabled && Logger.g)
        Logger.g->Log(0, "PZM2: -m2 on, SGameLogic::Refresh runs the M2 path (trace %s, crc %s, %s units)",
                      g_M2.Trace ? "on" : "off", g_M2.Crc ? "on" : "off", UV::kReal ? "HD" : "M1 stand-in");
    if (getenv("PZ_M2_CRCTEST"))
        CrcSelfTest();
    BeginFrame();                                                 // 0x571840 (logs frame 0 once more)
    int* f = (int*)this;
    f[0x20] = 5;                                                  // +0x80
    f[0x21] = 0;
    f[0x24] = 0;                                                  // +0x90 message line count
    f[0x22] = -1;
    f[0x23] = -1;
    for (int i = 0; i < 0x17; ++i) {
        f[0x25 + i] = -1;                                         // +0x94 message frames
        f[0x25 + 0x17 + i] = -1;                                  // +0xf0 message timers
    }
    *(short*)(f + 0x53) = 0;                                      // +0x14c
    f[0x5e] = -1;
    f[0x5d] = -1;
    f[0x55] = -1;                                                 // +0x154 TimeCounter variable
    f[0x54] = -1;                                                 // +0x150 TimeCounter frame
    f[0x58] = -1;
    f[0x57] = -1;                                                 // +0x15c UnitCounter variable
    f[0x56] = -1;                                                 // +0x158 UnitCounter frame
    // HD 0x56da00: board text frames for trigger variables named
    // "TimeCounter" / "UnitCounter" (none in maps/menu.map). Not lifted.
    for (int i = 0; g_World && i < g_World->TriggerVariables.Size; ++i) {
        const SHeapElem<STriggerVariable>& v = g_World->TriggerVariables.Array[i];
        if (v.Next == kHeapLive && (_stricmp(SStr(v.Data.Name), "TimeCounter") == 0 ||
                                    _stricmp(SStr(v.Data.Name), "UnitCounter") == 0) && Logger.g)
            Logger.g->Warning("SGameLogic: counter variable '%s' not shown (0x56da00 not lifted)", SStr(v.Data.Name));
    }
    MessagePlayer = p1;                                           // +0x5c = HD param_2
    MessageCount = 0;                                             // +0x78
    MessageTimer = 400;                                           // +0x7c
    // HD: +0x60 = board +0x08(2, World+0x74e0, 8, 0x52, 0, 1) message frame.
    f[0x18] = -1;
    f[0x19] = f[0x1a] = f[0x1b] = f[0x1c] = f[0x1d] = -1;         // +0x64..+0x74
    Mode = p3;                                                    // +0x00 = HD param_4 (menu 0)
    MinimapFrame = p2;                                            // +0x17c = HD param_3 (menu -1)
    if (MinimapFrame >= 0 && Logger.g)
        Logger.g->Warning("SGameLogic: minimap frame %d (minimap bitmaps 0x669be0 not lifted)", MinimapFrame);
    // HD param_3 < 0: +0x180 = -1, +0x18c = +0x190 = 0 (no minimap bitmaps).
    f[0x60] = -1;
    f[0x63] = 0;
    f[0x64] = 0;
    f[0x6c] = 0;
    f[0x6d] = 0;
    if (g_World)
        for (int i = 0; i < 12; ++i)
            *(int*)(g_World->Players[i] + 0x1c) = 0;              // World+0x18c + i*0x48
    // HD: the multiplayer player setup (DAT_008f1a74) is skipped in the menu.
    // HD 0x564fb0: per-player visibility maps (+0x1c8..+0x264). Not lifted;
    // CanSeeGroundUnit falls back to "visible" while they are missing.
    for (int i = 0; i < 12; ++i) {
        PlayerTable[i] = -1;
        Tick_565e10(i);                                           // 0x565e10
    }
    // HD: air start positions of the 12 players from the "start %d"
    // locations (World players +0x194/+0x198, log "Air start position for
    // player %d is X:%f Z:%f"). Not used by the menu; not lifted.
    CollectActiveLocations();                                     // 0x5640b0
    PZ_FOR_EACH_UNIT(i) {
        int ct = UV::ClassType(i);
        if ((ct == 0 || ct == 5 || ct == 0xc || ct == 0xb) && !UV::Unplaced(i))
            UpdateActiveLocationsAt(i, false);                    // 0x582080(unit, 0)
    }
    // HD 0x564c20: per-8x8-tile tables (+0x1d0 ...). Not lifted.
    Flag288 = false;
    Flag2b8 = false;
    if (g_World)
        g_World->Units.ReuseDelay = 100;                          // World+0x4f0 = 100
    if (g_Pixie) {
        TargetRingFx = g_Pixie->LoadEffectPrototype("effects/target_ring.fx", false, false, 0, 0);
        TankDustFx = g_Pixie->LoadEffectPrototype("effects/smoke/tank_goz.fx", false, false, 0, 0);
    }
    *(short*)(f + 0xb6) = 0;                                      // +0x2d8
    ((unsigned char*)this)[0x2da] = 0;
    if (g_Pixie)
        ShellFallFx = g_Pixie->LoadEffectPrototype("effects/sound/s_shell_fall.fx", false, false, 0, 0);
    // PANZERS 0x5735a0 (inline): support unit prototypes of every active player.
    for (int i = 0; g_World && g_UnitRegistry && i < 12; ++i) {
        int type = *(int*)(g_World->Players[i] + 0x08);           // World+0x178
        if (type != 0 && type != 1)
            continue;
        int race = *(int*)(g_World->Players[i] + 0x04);           // World+0x174
        const char* names[5];
        if (race == 0) {
            names[0] = "Ge Recon plane"; names[1] = "Ge TacBomber"; names[2] = "Ge Heavy Bomber";
            names[3] = "Ge Transport Plane"; names[4] = "Ge Parachute Squad";
        } else if (race == 2) {
            names[0] = "SU Recon plane"; names[1] = "SU TacBomber"; names[2] = "SU Heavy Bomber";
            names[3] = "SU Heavy Bomber"; names[4] = "SU Parachute Squad";
        } else {
            names[0] = "US Recon plane"; names[1] = "US TacBomber"; names[2] = "US Heavy Bomber";
            names[3] = "US Transport Plane"; names[4] = "US Parachute Squad";
        }
        for (int k = 0; k < 5; ++k)
            g_UnitRegistry->GetPUnit(names[k], true);             // 0x5d0e70(name, 1)
    }
    g_UnitRegistry ? (void)g_UnitRegistry->GetPUnit("Projectile cannonade", true) : (void)0;
    // HD: every unit +0x34 (RefreshTargeting), buildings also 0x5497a0.
    if (UV::kReal) {
        PZ_FOR_EACH_UNIT(i) {
            UV::Iface(i)->RefreshTargeting();
            // TODO(U): SBuildingUnit 0x5497a0 for class 9 (not in SIUnit).
        }
    }
    if (g_M2.Enabled)
        DumpTriggers(g_World);
}

// PANZERS 0x55fe00 (the parts the recompile owns)
SGameLogic::~SGameLogic()
{
    PZ_TRACE("SGameLogic::~SGameLogic (0x55fe00)");
    for (int i = 0; i < RunningTriggerCount; ++i)
        free(RunningTriggers[i].Found.Units);
    free(RunningTriggers);
    for (int i = 0; i < MovementGroupSize; ++i)
        free(MovementGroups[i].Members);
    free(MovementGroups);
    free(ActiveLocations);
    for (int i = 0; i < AnimatedModelCount; ++i) {
        if (AnimatedModels[i].Model)
            AnimatedModels[i].Model->Release();
        FreeSString(&AnimatedModels[i].Name);
    }
    free(AnimatedModels);
    free(CrcHistory);
    if (FrameObject)
        delete (SStreamBuffer*)FrameObject;
    if (g_Pixie) {
        if (TargetRingFx >= 0 && TargetRingFx) g_Pixie->ReleaseEffectPrototype(TargetRingFx);
        if (TankDustFx >= 0 && TankDustFx) g_Pixie->ReleaseEffectPrototype(TankDustFx);
        if (ShellFallFx >= 0 && ShellFallFx) g_Pixie->ReleaseEffectPrototype(ShellFallFx);
    }
    if (g_GameLogic == this)
        g_GameLogic = nullptr;                                    // DAT_008f2078 = 0
}

// PANZERS 0x5802f0
void SGameLogic::SetRunning(int running)
{
    PZ_TRACE("SGameLogic::SetRunning (0x5802f0)");
    Running = running;
    // HD: Concert +0x60 (pause) when 0, +0x64 (resume) otherwise. The
    // recompile's menu keeps the concert running.
}

// ---------------------------------------------------------------------------
// The tick

int SGameLogic::Refresh()
{
    PZ_TRACE("SGameLogic::Refresh (0x576d80)");
    if (g_M2.Enabled) {
        RefreshM2();
        return 1;
    }
    // M1: only the model part of the tick (unit animations, doodad
    // interpolation state). Triggers, units, AI and the rest are M2.
    if (g_World)
        g_World->RefreshModels();
    return 0;
}

// HD 0x576d80, single-player path (DAT_008f1a74 == 0); the M1 subset in
// world.cpp carries the marker.
// HD checks the FPU control word first (0x7f, logs "SGameLogic::Refresh:
// Invalid FPU control word (0x%04X)."). The multiplayer frame sync, the
// "has left/lost the game" checks, AI groups 0x5f5c70 (every 20th frame),
// 0x605930 and 0x6090e0 are not on the menu path.
void SGameLogic::RefreshM2()
{
    PZ_M2_TRACE("SGameLogic::RefreshM2 (0x576d80 single-player path)");
    SWorld* w = g_World;
    if (w)
        w->UpdateSpeech();                                        // 0x607f50
    Tick_578b00();
    Tick_578a70();
    if ((((const int*)this)[0x6d] == 0 || Running != 0) && FrameObject) {   // +0x1b4, +0x04, +0x20
        // HD 0x65daf0(0): FrameObject->Seek(0, 0) to read it back.
        ProcessPacket(w ? w->LocalPlayer : 0, 0);                 // 0x5737c0(World+0x16c, &FrameObject)
        BeginFrame();                                             // 0x571840
        ++FramesSent;
        Tick_579390();
    }
    if (Running < 1) {
        if (w)
            w->RefreshBlockMapDirtyRect();                        // 0x604620
        M2NextTick();
        return;
    }
    for (int n = 0; n < Running; ++n) {
        Tick_57dfe0();
        // HD 0x6088f0: weather (M1-lifted inside SWorld; driven by the scene).
        if (w)
            w->Units.Frame = (unsigned)Frame;                     // World+0x4ec = frame
        if (Frame % 20 == 0)
            DispatchEverySecond();                                // 0x570cc0
        RunTriggers();                                            // 0x579ab0
        Tick_568af0();
        static const int kPlayerCycle[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };   // HD table 0x7f6220
        int pl = kPlayerCycle[Frame % 12];
        if (w && *(int*)(w->Players[pl] + 0x08) != 2)             // World+0x178 + pl*0x48
            Tick_565e10(pl);
        if (UV::kReal) {
            PZ_FOR_EACH_UNIT(i)
                UV::Iface(i)->StoreInterpolationState();          // +0x16c
            // HD: scene +0xf0(2) between the two loops.
            PZ_FOR_EACH_UNIT(i) {
                if (UV::Container(i) != -1 || UV::Unplaced(i))
                    continue;
                // HD panics "World->Units[i]->StoredMembers.GetSize() == 0" for
                // squads (class 5) without members.
                UV::Iface(i)->ServerRefresh(Frame);               // +0x2c
            }
            // HD: SWorld::RefreshFlyingFox 0x5f6bf0 (no flying fox in the menu).
            PZ_FOR_EACH_UNIT(i)
                UV::Iface(i)->RefreshModel();                     // +0x3c
        } else if (w) {
            StandInMove(this);                                    // recompile test mover (movementgroup.cpp)
            w->RefreshModels();                                   // M1 visuals (stand-in units, doodads)
        }
        Tick_5822a0();
        // HD: the World+0x158 doodad animations (falling trees, M1 visuals
        // in SWorld::RefreshModels) and 0x605930 per World+0x7454 entry.
        ++Frame;
    }
    if (w)
        w->RefreshBlockMapDirtyRect();                            // 0x604620
    M2NextTick();
}

// PANZERS 0x571840
void SGameLogic::BeginFrame()
{
    PZ_M2_TRACE("SGameLogic::BeginFrame (0x571840)");
    if (FrameObject) {
        delete (SStreamBuffer*)FrameObject;                       // vtbl +0 (1)
        FrameObject = nullptr;
    }
    FrameObject = new SStreamBuffer();                            // new 0x30, 0x65cd20
    unsigned crc = ComputeWorldCRC();
    // HD logs "Frame %d started with server CRC: 0x%08X, RandomSeed: 0x%08X"
    // only with SMulti (DAT_008f1a74). The recompile writes the M2 CRC line.
    if (g_M2.Crc && Logger.g) {
        int units = 0;
        PZ_FOR_EACH_UNIT(i) { (void)i; ++units; }
        Logger.g->Log(0, "PZM2 CRC %d %08x %08x %d", Frame, crc, g_World ? g_World->RandomSeed : 0u, units);
    }
    // PANZERS 0x5610a0 (SDEQueue<unsigned>::AddTop, inline)
    if (CrcCount == CrcMax) {
        // PANZERS 0x56d7d0 (grow)
        int nmax = CrcMax < 0x10 ? 0x10 : (CrcMax * 6) / 5;
        CrcHistory = (unsigned*)realloc(CrcHistory, nmax * sizeof(unsigned));
        if (CrcMax <= CrcTop - CrcBase) {
            int wrapped = (CrcBase - CrcBottom) + CrcMax;
            memmove(CrcHistory + (nmax - wrapped), CrcHistory + (CrcMax - wrapped), wrapped * sizeof(unsigned));
            CrcBase += CrcMax - nmax;
        }
        CrcMax = nmax;
    }
    ++CrcTop;
    ++CrcCount;
    int idx = CrcTop;
    if (idx < CrcBottom)
        Logger.g->Panic("SDEQueue<%s>::operator[]: invalid index (%d)", "unsigned int", idx);
    idx -= CrcBase;
    if (CrcMax <= idx)
        idx -= CrcMax;
    CrcHistory[idx] = 0;
    // back in 0x571840: store the CRC at the new top
    idx = CrcTop - CrcBase;
    if (CrcMax <= idx)
        idx -= CrcMax;
    CrcHistory[idx] = crc;
    ((SStreamBuffer*)FrameObject)->Write(&crc, 4);                // 0x65dc40: stream vtbl +0x0c
}

// PANZERS 0x579390 (SDEQueue<unsigned>::RemoveBottom)
void SGameLogic::Tick_579390()
{
    PZ_M2_TRACE("SGameLogic::Tick_579390 (0x579390)");
    if (CrcCount == 0)
        Logger.g->Panic("SDEQueue<%s>::RemoveBottom: queue is already empty!", "unsigned int");
    int b = CrcBottom;
    --CrcCount;
    if (b - CrcBase == CrcMax - 1)
        CrcBase += CrcMax;
    if (b > CrcTop)
        Logger.g->Panic("SDEQueue<%s>::operator[]: invalid index (%d)", "unsigned int", b);
    CrcBottom = b + 1;
}

// PANZERS 0x578b00 (menu path)
// Message line fade-out (+0x90 lines, +0xf0 timers) and the TimeCounter /
// UnitCounter board texts. In the menu there are no lines and no counter
// variables, so only the guards run.
void SGameLogic::Tick_578b00()
{
    PZ_M2_TRACE("SGameLogic::Tick_578b00 (0x578b00)");
    const int* f = (const int*)this;
    if (f[0x20] < 0)                                              // +0x80
        return;
    bool lines = f[0x24] > 0;                                     // +0x90
    bool counters = (f[0x54] >= 0 && f[0x55] >= 0) || (f[0x56] >= 0 && f[0x57] >= 0);
    if ((lines || counters) && Logger.g) {
        static bool once;
        if (!once) {
            once = true;
            Logger.g->Warning("SGameLogic 0x578b00: board message lines / counters not lifted");
        }
    }
}

// PANZERS 0x578a70 (menu path: MessagePlayer = -1 returns at once)
void SGameLogic::Tick_578a70()
{
    PZ_M2_TRACE("SGameLogic::Tick_578a70 (0x578a70)");
    if (MessagePlayer < 0 || MessageCount == 0)
        return;
    if (--MessageTimer != 0)
        return;
    // HD: board +0x0c deletes the oldest line frame (+0x64), the others move
    // up 14 pixels (board +0x10), the last slot becomes -1.
    int* lines = (int*)((unsigned char*)this + 0x64);
    for (int k = 0; k < MessageCount - 1; ++k)
        lines[k] = lines[k + 1];
    lines[MessageCount] = -1;
    --MessageCount;
    MessageTimer = 400;
}

// PANZERS 0x5737c0 (single-player: no SMulti, the local frame stream only)
// HD decodes the packets of the frame stream (player orders). The menu has
// none; the stream holds only the CRC that BeginFrame wrote.
void SGameLogic::ProcessPacket(int frame, int p2)
{
    PZ_M2_TRACE("SGameLogic::ProcessPacket (0x5737c0)");
    (void)frame; (void)p2;
}

// PANZERS 0x57dfe0 (menu path)
// Autosave every 5/15/30 minutes (DAT_00929dd8) when a campaign exists
// (DAT_00929a0c); the menu has no campaign.
void SGameLogic::Tick_57dfe0()
{
    PZ_M2_TRACE("SGameLogic::Tick_57dfe0 (0x57dfe0)");
}

// PANZERS 0x568af0 (menu path: +0x2b8 is 0)
void SGameLogic::Tick_568af0()
{
    PZ_M2_TRACE("SGameLogic::Tick_568af0 (0x568af0)");
    if (!Flag2b8)
        return;
    STUB_LOG("SGameLogic::Tick_568af0 scripted sequence (0x568bc0 / 0x5826c0 / 0x565390)");
}

void SGameLogic::Tick_565e10(int player)
{
    STUB_LOG("SGameLogic::Tick_565e10 (0x565e10)");
    PZ_M2_TRACE("SGameLogic::Tick_565e10 (0x565e10)");
    (void)player;
}

// PANZERS 0x5649e0
void SGameLogic::CreateAnimatedModel(const char* file, float x, float y, float z, float dir)
{
    int proto = PzGepard()->LoadModelPrototype(file, 0.005f, 0, 0);   // Gepard +0x20 (0x3ba3d70a)
    if (proto < 0) {
        Logger.g->Log(0, "SGameLogic::CreateAnimatedModel: '%s' not found.", file);
        return;
    }
    // PANZERS 0x560a70 (SDArray<SAnimatedModel>::Add)
    if (AnimatedModelCount == AnimatedModelMax) {
        int nmax = AnimatedModelMax < 0x10 ? 0x10 : (AnimatedModelMax * 6) / 5;
        AnimatedModels = (SAnimatedModel*)realloc(AnimatedModels, nmax * sizeof(SAnimatedModel));
        memset(AnimatedModels + AnimatedModelMax, 0, (nmax - AnimatedModelMax) * sizeof(SAnimatedModel));
        AnimatedModelMax = nmax;
    }
    SAnimatedModel& a = AnimatedModels[AnimatedModelCount++];
    a.Name = file;
    a.Pos[0] = x;
    a.Pos[1] = y;
    a.Pos[2] = z;
    a.Dir = dir;
    a.Model = g_Scene->CreateModelFromPrototype(proto, 1);        // scene +0x58
    if (!a.Model)
        Logger.g->Panic("SGameLogic::CreateAnimatedModel: Cannot load model: %s", SStr(a.Name));
    a.Model->SetFlags(0x54);                                      // +0x94
    a.Model->SetPosition(a.Pos[0], a.Pos[1], a.Pos[2]);           // +0x18
    a.Model->SetRotation(a.Dir, 0.0f, 0.0f);                      // +0x1c
    a.Model->StoreInterpolationState();                           // +0x3c
    {
        // model +0x80(0): length of sequence 0 (imodel.h Slot_80, unnamed there).
        typedef float(__thiscall * SeqLenFn)(SIModel*, int);
        a.Duration = (*(SeqLenFn*)(*(void***)a.Model + 0x80 / 4))(a.Model, 0);
    }
    a.Time = 0.0f;
    PzGepard()->ReleaseModelPrototype(proto);                     // Gepard +0x24
}

// PANZERS 0x5822a0
void SGameLogic::Tick_5822a0()
{
    PZ_M2_TRACE("SGameLogic::Tick_5822a0 (0x5822a0)");
    int i = 0;
    while (i < AnimatedModelCount) {
        SAnimatedModel& a = AnimatedModels[i];
        if (a.Time <= a.Duration) {
            a.Model->StoreInterpolationState();                   // +0x3c
            a.Model->AdvanceAnimation(0.05f);                     // +0x70(0x3d4ccccd)
            a.Time += 0.05f;                                      // _DAT_007f4534
            ++i;
        } else {
            // PANZERS 0x578f70 (SDArray<SAnimatedModel>::Remove)
            if (a.Model)
                a.Model->Release();
            a.Model = nullptr;
            FreeSString(&a.Name);
            --AnimatedModelCount;
            if (AnimatedModelCount - i != 0)
                memmove(&AnimatedModels[i], &AnimatedModels[i + 1], (AnimatedModelCount - i) * sizeof(SAnimatedModel));
            memset(&AnimatedModels[AnimatedModelCount], 0, sizeof(SAnimatedModel));
            if (g_World)
                g_World->CamProjectionDirty = true;               // World+0x34 = 1
        }
    }
}

// PANZERS 0x5638f0 (menu path: MinimapFrame +0x17c is -1)
void SGameLogic::UpdateUnitVisuals(SIViewport* vp, double interpolation)
{
    PZ_TRACE("SGameLogic::UpdateUnitVisuals (0x5638f0)");
    (void)interpolation;
    // HD: SMulti 0x520ff0 first in multiplayer.
    if (UV::kReal) {
        PZ_FOR_EACH_UNIT(i)
            UV::Iface(i)->UpdateVisuals(vp);                      // +0x40
    }
    if (MinimapFrame < 0)
        return;
    STUB_LOG("SGameLogic::UpdateUnitVisuals minimap dots (0x5638f0)");
}

// PANZERS 0x56d1a0
int SGameLogic::GetFrame()
{
    return Frame;
}

// PANZERS 0x56e150
bool SGameLogic::IsPaused()
{
    return Flag288 || Flag2b8;
}

// HD 0x549ab0 (as in triggers.cpp; player alliance: same team id, or the same player)
static bool SameSide(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);               // World+0x17c + a*0x48
    if (team != 0)
        return team == *(int*)(g_World->Players[b] + 0x0c);
    return a == b;
}

// PANZERS 0x562760
// Per-player visibility maps (+0x1d4, built by 0x564fb0 / 0x565e10) are not
// lifted: where HD reads them, the recompile answers "visible" (HD would
// panic "SGameLogic::CanSeeGroundUnit: Player[%d] (editor: %d) has no
// VisMap" if the map were missing). Real units only.
bool SGameLogic::CanSeeGroundUnit(int player, SIUnit* unit)
{
    PZ_M2_TRACE("SGameLogic::CanSeeGroundUnit (0x562760)");
    const unsigned char* u = (const unsigned char*)(void*)unit;
    if (!u || !UV::kReal)
        return true;
    const unsigned char* pu = *(unsigned char* const*)(u + 0x04);
    if (pu[0x8a])
        return true;
    if (u[0x168])
        return false;
    // HD 0x5bb5c0: unit +0x1a8 TestBlockMap(+0x8c, +0x94, +0xb0, max(+0x58, 1), 1)
    // unless +0xd8 bit 0 is set: hidden units are not seen.
    if (!(*(const unsigned char*)(u + 0xd8) & 1)) {
        int size = *(const int*)(u + 0x58);
        if (size < 1)
            size = 1;
        if (unit->TestBlockMap(*(const int*)(u + 0x8c), *(const int*)(u + 0x94), *(const int*)(u + 0xb0), size, 1))
            return false;
    }
    if (SameSide(player, *(const int*)(u + 0xfc)) && !u[0x110])
        return true;
    float x = *(const float*)(u + 0x8c), z = *(const float*)(u + 0x94);
    if (!(x >= 0.0f && x <= (float)g_World->TerrainW && z >= 0.0f && z <= (float)g_World->TerrainH))
        return false;
    static bool once;
    if (!once && Logger.g) {
        once = true;
        Logger.g->Warning("SGameLogic::CanSeeGroundUnit: no VisMap (0x564fb0 not lifted), units count as visible");
    }
    return true;
}

} // namespace pz
