// src/3dengine/pz/pzpixie.cpp
// pz::SPixie skeleton. OWNER: agent C. See pzscene.cpp for the stub rules.

#include <string.h>
#include "pzpixie.h"
#include "stub_log.h"

namespace pz {

SPixie::SPixie()
{
    STUB_LOG("SPixie::SPixie (0x6941e0)");
    PZ_TRACE("SPixie::SPixie (0x6941e0)");
    memset(_04, 0, sizeof(_04));
    RefCount = 1;   // assumed like SScene/SModel; check 0x6941e0
}

SPixie::~SPixie()
{
    STUB_LOG("SPixie::~SPixie (0x69cb90)");
    PZ_TRACE("SPixie::~SPixie (0x69cb90)");
}

// PANZERS 0x69d1d0
void SPixie::AddRef()
{
    PZ_TRACE("SPixie::AddRef (0x69d1d0)");
    ++RefCount;
}

// PANZERS 0x69e8a0
void SPixie::Release()
{
    PZ_TRACE("SPixie::Release (0x69e8a0)");
    if (--RefCount == 0)
        delete this;   // ~SPixie 0x69cb90 + operator delete(0x7c)
}

// ---- generated slot stubs (HD vtable order) ----

// HD SPixie vtbl +0x08 -> 0x69edb0 (1 arg dword)
void SPixie::Slot_08()
{
    STUB_LOG("SPixie::Slot_08 (0x69edb0)");
    PZ_TRACE("SPixie::Slot_08 (0x69edb0)");
}

// HD SPixie vtbl +0x0c -> 0x69e5a0 (1 arg dword)
int SPixie::RefreshEffectPrototype(int proto)
{
    STUB_LOG("SPixie::RefreshEffectPrototype (0x69e5a0)");
    PZ_TRACE("SPixie::RefreshEffectPrototype (0x69e5a0)");
    (void)proto;
    return 0;
}

// HD SPixie vtbl +0x10 -> 0x69dc80 (5 arg dwords)
int SPixie::LoadEffectPrototype(const char* file, bool p2, bool p3, int p4, int p5)
{
    STUB_LOG("SPixie::LoadEffectPrototype (0x69dc80)");
    PZ_TRACE("SPixie::LoadEffectPrototype (0x69dc80)");
    (void)file; (void)p2; (void)p3; (void)p4; (void)p5;
    return -1;
}

// HD SPixie vtbl +0x14 -> 0x69ee50 (2 arg dwords)
void SPixie::Slot_14()
{
    STUB_LOG("SPixie::Slot_14 (0x69ee50)");
    PZ_TRACE("SPixie::Slot_14 (0x69ee50)");
}

// HD SPixie vtbl +0x18 -> 0x69d2a0 (0 arg dwords)
void SPixie::Slot_18()
{
    STUB_LOG("SPixie::Slot_18 (0x69d2a0)");
    PZ_TRACE("SPixie::Slot_18 (0x69d2a0)");
}

// HD SPixie vtbl +0x1c -> 0x69d5b0 (1 arg dword)
void SPixie::Slot_1C()
{
    STUB_LOG("SPixie::Slot_1C (0x69d5b0)");
    PZ_TRACE("SPixie::Slot_1C (0x69d5b0)");
}

// HD SPixie vtbl +0x20 -> 0x69e8c0 (1 arg dword)
void SPixie::Slot_20()
{
    STUB_LOG("SPixie::Slot_20 (0x69e8c0)");
    PZ_TRACE("SPixie::Slot_20 (0x69e8c0)");
}

// HD SPixie vtbl +0x24 -> 0x69e400 (5 arg dwords)
void SPixie::PlayEffect(SIScene* scene, int proto, const float* pos, const float* dir, int p5)
{
    STUB_LOG("SPixie::PlayEffect (0x69e400)");
    PZ_TRACE("SPixie::PlayEffect (0x69e400)");
    (void)scene; (void)proto; (void)pos; (void)dir; (void)p5;
}

// HD SPixie vtbl +0x28 -> 0x69e510 (5 arg dwords)
void SPixie::Slot_28()
{
    STUB_LOG("SPixie::Slot_28 (0x69e510)");
    PZ_TRACE("SPixie::Slot_28 (0x69e510)");
}

// HD SPixie vtbl +0x2c -> 0x69f540 (4 arg dwords)
void SPixie::Slot_2C()
{
    STUB_LOG("SPixie::Slot_2C (0x69f540)");
    PZ_TRACE("SPixie::Slot_2C (0x69f540)");
}

// HD SPixie vtbl +0x30 -> 0x69f610 (4 arg dwords)
void SPixie::Slot_30()
{
    STUB_LOG("SPixie::Slot_30 (0x69f610)");
    PZ_TRACE("SPixie::Slot_30 (0x69f610)");
}

// HD SPixie vtbl +0x34 -> 0x69f6b0 (1 arg dword)
void SPixie::StopEffect(int effect)
{
    STUB_LOG("SPixie::StopEffect (0x69f6b0)");
    PZ_TRACE("SPixie::StopEffect (0x69f6b0)");
    (void)effect;
}

// HD SPixie vtbl +0x38 -> 0x69d1e0 (1 arg dword)
void SPixie::Slot_38()
{
    STUB_LOG("SPixie::Slot_38 (0x69d1e0)");
    PZ_TRACE("SPixie::Slot_38 (0x69d1e0)");
}

// HD SPixie vtbl +0x3c -> 0x69d540 (1 arg dword)
void SPixie::Slot_3C()
{
    STUB_LOG("SPixie::Slot_3C (0x69d540)");
    PZ_TRACE("SPixie::Slot_3C (0x69d540)");
}

// HD SPixie vtbl +0x40 -> 0x69f710 (1 arg dword)
void SPixie::UnregisterEffect(int effect)
{
    STUB_LOG("SPixie::UnregisterEffect (0x69f710)");
    PZ_TRACE("SPixie::UnregisterEffect (0x69f710)");
    (void)effect;
}

// HD SPixie vtbl +0x44 -> 0x69d410 (1 arg dword)
void SPixie::Slot_44()
{
    STUB_LOG("SPixie::Slot_44 (0x69d410)");
    PZ_TRACE("SPixie::Slot_44 (0x69d410)");
}

// HD SPixie vtbl +0x48 -> 0x69f7b0 (1 arg dword)
void SPixie::UpdateFrame(int frame)
{
    STUB_LOG("SPixie::UpdateFrame (0x69f7b0)");
    PZ_TRACE("SPixie::UpdateFrame (0x69f7b0)");
    (void)frame;
}

// HD SPixie vtbl +0x4c -> 0x69f370 (2 arg dwords)
void SPixie::Slot_4C()
{
    STUB_LOG("SPixie::Slot_4C (0x69f370)");
    PZ_TRACE("SPixie::Slot_4C (0x69f370)");
}

// HD SPixie vtbl +0x50 -> 0x69f2d0 (2 arg dwords)
void SPixie::Slot_50()
{
    STUB_LOG("SPixie::Slot_50 (0x69f2d0)");
    PZ_TRACE("SPixie::Slot_50 (0x69f2d0)");
}

// HD SPixie vtbl +0x54 -> 0x69f460 (2 arg dwords)
void SPixie::Slot_54()
{
    STUB_LOG("SPixie::Slot_54 (0x69f460)");
    PZ_TRACE("SPixie::Slot_54 (0x69f460)");
}

// HD SPixie vtbl +0x58 -> 0x69f320 (2 arg dwords)
void SPixie::Slot_58()
{
    STUB_LOG("SPixie::Slot_58 (0x69f320)");
    PZ_TRACE("SPixie::Slot_58 (0x69f320)");
}

// HD SPixie vtbl +0x5c -> 0x69dc20 (1 arg dword)
void SPixie::Slot_5C()
{
    STUB_LOG("SPixie::Slot_5C (0x69dc20)");
    PZ_TRACE("SPixie::Slot_5C (0x69dc20)");
}

// HD SPixie vtbl +0x60 -> 0x69f410 (2 arg dwords)
void SPixie::Slot_60()
{
    STUB_LOG("SPixie::Slot_60 (0x69f410)");
    PZ_TRACE("SPixie::Slot_60 (0x69f410)");
}

// HD SPixie vtbl +0x64 -> 0x69f4e0 (3 arg dwords)
void SPixie::Slot_64()
{
    STUB_LOG("SPixie::Slot_64 (0x69f4e0)");
    PZ_TRACE("SPixie::Slot_64 (0x69f4e0)");
}

// HD SPixie vtbl +0x68 -> 0x69d6a0 (3 arg dwords)
int SPixie::InitEffectPrototype(int p1, int p2, int p3)
{
    STUB_LOG("SPixie::InitEffectPrototype (0x69d6a0)");
    PZ_TRACE("SPixie::InitEffectPrototype (0x69d6a0)");
    (void)p1; (void)p2; (void)p3;
    return 0;
}

} // namespace pz
