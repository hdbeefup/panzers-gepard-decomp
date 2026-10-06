// src/3dengine/pz/pzterrain.cpp
// pz::STerrain skeleton. OWNER: agent B. See pzscene.cpp for the stub rules.

#include <string.h>
#include "pzterrain.h"
#include "stub_log.h"

namespace pz {

STerrain::STerrain(SScene* scene, int width, int height, int p4)
{
    STUB_LOG("STerrain::STerrain (0x6efde0)");
    PZ_TRACE("STerrain::STerrain (0x6efde0)");
    (void)scene; (void)width; (void)height; (void)p4;
    memset(_04, 0, sizeof(_04));
}

STerrain::~STerrain()
{
    STUB_LOG("STerrain::~STerrain (0x6f06c0)");
    PZ_TRACE("STerrain::~STerrain (0x6f06c0)");
}

// ---- generated slot stubs (HD vtable order) ----

// HD STerrain vtbl +0x00 -> 0x6f0d00 (8 arg dwords)
void STerrain::Acquire(STerrainBuffers* out)
{
    STUB_LOG("STerrain::Acquire (0x6f0d00)");
    PZ_TRACE("STerrain::Acquire (0x6f0d00)");
    (void)out;
}

// HD STerrain vtbl +0x04 -> 0x6f5510 (2 arg dwords)
void STerrain::Slot_04_LoadLayerTexture(int layer, int p2)
{
    STUB_LOG("STerrain::Slot_04_LoadLayerTexture (0x6f5510)");
    PZ_TRACE("STerrain::Slot_04_LoadLayerTexture (0x6f5510)");
    (void)layer; (void)p2;
}

// HD STerrain vtbl +0x08 -> 0x6f52b0 (3 arg dwords)
void STerrain::Slot_08()
{
    STUB_LOG("STerrain::Slot_08 (0x6f52b0)");
    PZ_TRACE("STerrain::Slot_08 (0x6f52b0)");
}

// HD STerrain vtbl +0x0c -> 0x6f5740 (1 arg dword)
void STerrain::LoadSketchTexture(const char* file)
{
    STUB_LOG("STerrain::LoadSketchTexture (0x6f5740)");
    PZ_TRACE("STerrain::LoadSketchTexture (0x6f5740)");
    (void)file;
}

// HD STerrain vtbl +0x10 -> 0x6f8b80 (1 arg dword)
void STerrain::Slot_10()
{
    STUB_LOG("STerrain::Slot_10 (0x6f8b80)");
    PZ_TRACE("STerrain::Slot_10 (0x6f8b80)");
}

// HD STerrain vtbl +0x14 -> 0x6f50b0 (0 arg dwords)
void STerrain::Slot_14()
{
    STUB_LOG("STerrain::Slot_14 (0x6f50b0)");
    PZ_TRACE("STerrain::Slot_14 (0x6f50b0)");
}

// HD STerrain vtbl +0x18 -> 0x6f1f60 (0 arg dwords)
void STerrain::Slot_18()
{
    STUB_LOG("STerrain::Slot_18 (0x6f1f60)");
    PZ_TRACE("STerrain::Slot_18 (0x6f1f60)");
}

// HD STerrain vtbl +0x1c -> 0x6ff510 (2 arg dwords)
void STerrain::Slot_1C()
{
    STUB_LOG("STerrain::Slot_1C (0x6ff510)");
    PZ_TRACE("STerrain::Slot_1C (0x6ff510)");
}

// HD STerrain vtbl +0x20 -> 0x6f5230 (4 arg dwords)
void STerrain::Slot_20()
{
    STUB_LOG("STerrain::Slot_20 (0x6f5230)");
    PZ_TRACE("STerrain::Slot_20 (0x6f5230)");
}

// HD STerrain vtbl +0x24 -> 0x6f8c90 (0 arg dwords)
void STerrain::Slot_24()
{
    STUB_LOG("STerrain::Slot_24 (0x6f8c90)");
    PZ_TRACE("STerrain::Slot_24 (0x6f8c90)");
}

// HD STerrain vtbl +0x28 -> 0x6f4b50 (1 arg dword)
void STerrain::Slot_28()
{
    STUB_LOG("STerrain::Slot_28 (0x6f4b50)");
    PZ_TRACE("STerrain::Slot_28 (0x6f4b50)");
}

// HD STerrain vtbl +0x2c -> 0x6f8680 (1 arg dword)
void STerrain::Slot_2C()
{
    STUB_LOG("STerrain::Slot_2C (0x6f8680)");
    PZ_TRACE("STerrain::Slot_2C (0x6f8680)");
}

// HD STerrain vtbl +0x30 -> 0x6f4b70 (0 arg dwords)
void STerrain::Slot_30()
{
    STUB_LOG("STerrain::Slot_30 (0x6f4b70)");
    PZ_TRACE("STerrain::Slot_30 (0x6f4b70)");
}

// HD STerrain vtbl +0x34 -> 0x6f8b60 (2 arg dwords)
void STerrain::Slot_34()
{
    STUB_LOG("STerrain::Slot_34 (0x6f8b60)");
    PZ_TRACE("STerrain::Slot_34 (0x6f8b60)");
}

// HD STerrain vtbl +0x38 -> 0x6f8640 (1 arg dword)
void STerrain::Slot_38()
{
    STUB_LOG("STerrain::Slot_38 (0x6f8640)");
    PZ_TRACE("STerrain::Slot_38 (0x6f8640)");
}

// HD STerrain vtbl +0x3c -> 0x6f4b60 (0 arg dwords)
void STerrain::Slot_3C()
{
    STUB_LOG("STerrain::Slot_3C (0x6f4b60)");
    PZ_TRACE("STerrain::Slot_3C (0x6f4b60)");
}

// HD STerrain vtbl +0x40 -> 0x6f85f0 (0 arg dwords)
void STerrain::Slot_40()
{
    STUB_LOG("STerrain::Slot_40 (0x6f85f0)");
    PZ_TRACE("STerrain::Slot_40 (0x6f85f0)");
}

// HD STerrain vtbl +0x44 -> 0x6f10e0 (3 arg dwords)
void STerrain::Slot_44()
{
    STUB_LOG("STerrain::Slot_44 (0x6f10e0)");
    PZ_TRACE("STerrain::Slot_44 (0x6f10e0)");
}

// HD STerrain vtbl +0x48 -> 0x6f2420 (5 arg dwords)
void STerrain::Slot_48()
{
    STUB_LOG("STerrain::Slot_48 (0x6f2420)");
    PZ_TRACE("STerrain::Slot_48 (0x6f2420)");
}

// HD STerrain vtbl +0x4c -> 0x6f2910 (1 arg dword)
void STerrain::Slot_4C()
{
    STUB_LOG("STerrain::Slot_4C (0x6f2910)");
    PZ_TRACE("STerrain::Slot_4C (0x6f2910)");
}

// HD STerrain vtbl +0x50 -> 0x6f8bd0 (3 arg dwords)
void STerrain::Slot_50()
{
    STUB_LOG("STerrain::Slot_50 (0x6f8bd0)");
    PZ_TRACE("STerrain::Slot_50 (0x6f8bd0)");
}

// HD STerrain vtbl +0x54 -> 0x6f88d0 (2 arg dwords)
void STerrain::Slot_54()
{
    STUB_LOG("STerrain::Slot_54 (0x6f88d0)");
    PZ_TRACE("STerrain::Slot_54 (0x6f88d0)");
}

// HD STerrain vtbl +0x58 -> 0x6f8840 (4 arg dwords)
void STerrain::Slot_58()
{
    STUB_LOG("STerrain::Slot_58 (0x6f8840)");
    PZ_TRACE("STerrain::Slot_58 (0x6f8840)");
}

// HD STerrain vtbl +0x5c -> 0x6f9580 (0 arg dwords)
void STerrain::Slot_5C()
{
    STUB_LOG("STerrain::Slot_5C (0x6f9580)");
    PZ_TRACE("STerrain::Slot_5C (0x6f9580)");
}

// HD STerrain vtbl +0x60 -> 0x6f24f0 (9 arg dwords)
void STerrain::Slot_60()
{
    STUB_LOG("STerrain::Slot_60 (0x6f24f0)");
    PZ_TRACE("STerrain::Slot_60 (0x6f24f0)");
}

// HD STerrain vtbl +0x64 -> 0x6f29b0 (1 arg dword)
void STerrain::Slot_64()
{
    STUB_LOG("STerrain::Slot_64 (0x6f29b0)");
    PZ_TRACE("STerrain::Slot_64 (0x6f29b0)");
}

// HD STerrain vtbl +0x68 -> 0x6f8900 (3 arg dwords)
void STerrain::Slot_68()
{
    STUB_LOG("STerrain::Slot_68 (0x6f8900)");
    PZ_TRACE("STerrain::Slot_68 (0x6f8900)");
}

// HD STerrain vtbl +0x6c -> 0x6f8970 (2 arg dwords)
void STerrain::Slot_6C()
{
    STUB_LOG("STerrain::Slot_6C (0x6f8970)");
    PZ_TRACE("STerrain::Slot_6C (0x6f8970)");
}

// HD STerrain vtbl +0x70 -> 0x6f8ac0 (2 arg dwords)
void STerrain::Slot_70()
{
    STUB_LOG("STerrain::Slot_70 (0x6f8ac0)");
    PZ_TRACE("STerrain::Slot_70 (0x6f8ac0)");
}

// HD STerrain vtbl +0x74 -> 0x6f8a50 (2 arg dwords)
void STerrain::Slot_74()
{
    STUB_LOG("STerrain::Slot_74 (0x6f8a50)");
    PZ_TRACE("STerrain::Slot_74 (0x6f8a50)");
}

// HD STerrain vtbl +0x78 -> 0x6f2650 (6 arg dwords)
void STerrain::Slot_78()
{
    STUB_LOG("STerrain::Slot_78 (0x6f2650)");
    PZ_TRACE("STerrain::Slot_78 (0x6f2650)");
}

// HD STerrain vtbl +0x7c -> 0x6f5840 (7 arg dwords)
void STerrain::Slot_7C()
{
    STUB_LOG("STerrain::Slot_7C (0x6f5840)");
    PZ_TRACE("STerrain::Slot_7C (0x6f5840)");
}

// HD STerrain vtbl +0x80 -> 0x6fcee0 (1 arg dword)
void STerrain::Slot_80()
{
    STUB_LOG("STerrain::Slot_80 (0x6fcee0)");
    PZ_TRACE("STerrain::Slot_80 (0x6fcee0)");
}

// HD STerrain vtbl +0x84 -> 0x6f2a30 (1 arg dword)
void STerrain::Slot_84()
{
    STUB_LOG("STerrain::Slot_84 (0x6f2a30)");
    PZ_TRACE("STerrain::Slot_84 (0x6f2a30)");
}

// HD STerrain vtbl +0x88 -> 0x6f95b0 (1 arg dword)
void STerrain::UpdateRoad(int road)
{
    STUB_LOG("STerrain::UpdateRoad (0x6f95b0)");
    PZ_TRACE("STerrain::UpdateRoad (0x6f95b0)");
    (void)road;
}

// HD STerrain vtbl +0x8c -> 0x6f2710 (5 arg dwords)
void STerrain::Slot_8C()
{
    STUB_LOG("STerrain::Slot_8C (0x6f2710)");
    PZ_TRACE("STerrain::Slot_8C (0x6f2710)");
}

// HD STerrain vtbl +0x90 -> 0x6f5b80 (6 arg dwords)
void STerrain::Slot_90()
{
    STUB_LOG("STerrain::Slot_90 (0x6f5b80)");
    PZ_TRACE("STerrain::Slot_90 (0x6f5b80)");
}

// HD STerrain vtbl +0x94 -> 0x6f2a70 (1 arg dword)
void STerrain::Slot_94()
{
    STUB_LOG("STerrain::Slot_94 (0x6f2a70)");
    PZ_TRACE("STerrain::Slot_94 (0x6f2a70)");
}

// HD STerrain vtbl +0x98 -> 0x6fd380 (1 arg dword)
void STerrain::UpdateRoadJunction(int junction)
{
    STUB_LOG("STerrain::UpdateRoadJunction (0x6fd380)");
    PZ_TRACE("STerrain::UpdateRoadJunction (0x6fd380)");
    (void)junction;
}

} // namespace pz
