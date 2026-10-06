// src/3dengine/pz/pzmodel.cpp
// pz::SModel skeleton. OWNER: agent A. See pzscene.cpp for the stub rules.

#include <string.h>
#include "pzmodel.h"
#include "stub_log.h"

namespace pz {

SModel::SModel(SScene* scene, SPModel* proto, SPModel* proto2, int flag, int index)
{
    STUB_LOG("SModel::SModel (0x6d4ae0)");
    PZ_TRACE("SModel::SModel (0x6d4ae0)");
    (void)proto; (void)proto2; (void)flag; (void)index;
    memset(_08, 0, sizeof(SModel) - offsetof(SModel, _08));
    RefCount = 1;
    Scene = scene;
}

SModel::~SModel()
{
    STUB_LOG("SModel::~SModel (0x6d562d)");
    PZ_TRACE("SModel::~SModel (0x6d562d)");
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
        delete static_cast<SIAttachable*>(this);   // HD: (+0x04 vtbl)[0](1)
}

// ---- generated slot stubs: SIModel (HD vtable order) ----

// HD SModel vtbl +0x08 -> 0x6d7e90 (0 arg dwords)
void SModel::Slot_08()
{
    STUB_LOG("SModel::Slot_08 (0x6d7e90)");
    PZ_TRACE("SModel::Slot_08 (0x6d7e90)");
}

// HD SModel vtbl +0x0c -> 0x6d7ea0 (0 arg dwords)
void SModel::Slot_0C()
{
    STUB_LOG("SModel::Slot_0C (0x6d7ea0)");
    PZ_TRACE("SModel::Slot_0C (0x6d7ea0)");
}

// HD SModel vtbl +0x10 -> 0x6d7eb0 (1 arg dword)
void SModel::GetPosition(float* xyz)
{
    STUB_LOG("SModel::GetPosition (0x6d7eb0)");
    PZ_TRACE("SModel::GetPosition (0x6d7eb0)");
    (void)xyz;
}

// HD SModel vtbl +0x14 -> 0x6d7950 (1 arg dword)
void SModel::GetRenderPosition(float* xyz)
{
    STUB_LOG("SModel::GetRenderPosition (0x6d7950)");
    PZ_TRACE("SModel::GetRenderPosition (0x6d7950)");
    (void)xyz;
}

// HD SModel vtbl +0x18 -> 0x6da8d0 (3 arg dwords)
void SModel::SetPosition(float x, float y, float z)
{
    STUB_LOG("SModel::SetPosition (0x6da8d0)");
    PZ_TRACE("SModel::SetPosition (0x6da8d0)");
    (void)x; (void)y; (void)z;
}

// HD SModel vtbl +0x1c -> 0x6daab0 (3 arg dwords)
void SModel::SetRotation(float angle, float tiltX, float tiltZ)
{
    STUB_LOG("SModel::SetRotation (0x6daab0)");
    PZ_TRACE("SModel::SetRotation (0x6daab0)");
    (void)angle; (void)tiltX; (void)tiltZ;
}

// HD SModel vtbl +0x20 -> 0x6daa60 (3 arg dwords)
void SModel::SetRotationYawPitchRoll(float pitch, float yaw, float roll)
{
    STUB_LOG("SModel::SetRotationYawPitchRoll (0x6daa60)");
    PZ_TRACE("SModel::SetRotationYawPitchRoll (0x6daa60)");
    (void)pitch; (void)yaw; (void)roll;
}

// HD SModel vtbl +0x24 -> 0x6da9e0 (4 arg dwords)
void SModel::RotateAxis(float x, float y, float z, float angle)
{
    STUB_LOG("SModel::RotateAxis (0x6da9e0)");
    PZ_TRACE("SModel::RotateAxis (0x6da9e0)");
    (void)x; (void)y; (void)z; (void)angle;
}

// HD SModel vtbl +0x28 -> 0x6dad20 (1 arg dword)
void SModel::SetScale(float scale)
{
    STUB_LOG("SModel::SetScale (0x6dad20)");
    PZ_TRACE("SModel::SetScale (0x6dad20)");
    (void)scale;
}

// HD SModel vtbl +0x2c -> 0x6d7ee0 (0 arg dwords)
float SModel::GetScale()
{
    STUB_LOG("SModel::GetScale (0x6d7ee0)");
    PZ_TRACE("SModel::GetScale (0x6d7ee0)");
    return 0.0f;
}

// HD SModel vtbl +0x30 -> 0x6dafd0 (2 arg dwords)
void SModel::SetSequence(unsigned sequence, bool p2)
{
    STUB_LOG("SModel::SetSequence (0x6dafd0)");
    PZ_TRACE("SModel::SetSequence (0x6dafd0)");
    (void)sequence; (void)p2;
}

// HD SModel vtbl +0x34 -> 0x6d86d0 (0 arg dwords)
void SModel::Slot_34()
{
    STUB_LOG("SModel::Slot_34 (0x6d86d0)");
    PZ_TRACE("SModel::Slot_34 (0x6d86d0)");
}

// HD SModel vtbl +0x38 -> 0x6db2a0 (3 arg dwords)
void SModel::Slot_38()
{
    STUB_LOG("SModel::Slot_38 (0x6db2a0)");
    PZ_TRACE("SModel::Slot_38 (0x6db2a0)");
}

// HD SModel vtbl +0x3c -> 0x6da0f0 (0 arg dwords)
void SModel::StoreInterpolationState()
{
    STUB_LOG("SModel::StoreInterpolationState (0x6da0f0)");
    PZ_TRACE("SModel::StoreInterpolationState (0x6da0f0)");
}

// HD SModel vtbl +0x40 -> 0x6d7b90 (1 arg dword)
int SModel::FindNode(const char* name)
{
    STUB_LOG("SModel::FindNode (0x6d7b90)");
    PZ_TRACE("SModel::FindNode (0x6d7b90)");
    (void)name;
    return -1;
}

// HD SModel vtbl +0x44 -> 0x6d7c20 (1 arg dword)
void SModel::Slot_44()
{
    STUB_LOG("SModel::Slot_44 (0x6d7c20)");
    PZ_TRACE("SModel::Slot_44 (0x6d7c20)");
}

// HD SModel vtbl +0x48 -> 0x6da4d0 (7 arg dwords)
void SModel::Slot_48()
{
    STUB_LOG("SModel::Slot_48 (0x6da4d0)");
    PZ_TRACE("SModel::Slot_48 (0x6da4d0)");
}

// HD SModel vtbl +0x4c -> 0x6da330 (6 arg dwords)
void SModel::Slot_4C()
{
    STUB_LOG("SModel::Slot_4C (0x6da330)");
    PZ_TRACE("SModel::Slot_4C (0x6da330)");
}

// HD SModel vtbl +0x50 -> 0x6d7d70 (3 arg dwords)
void SModel::Slot_50()
{
    STUB_LOG("SModel::Slot_50 (0x6d7d70)");
    PZ_TRACE("SModel::Slot_50 (0x6d7d70)");
}

// HD SModel vtbl +0x54 -> 0x6d7d30 (2 arg dwords)
void SModel::Slot_54()
{
    STUB_LOG("SModel::Slot_54 (0x6d7d30)");
    PZ_TRACE("SModel::Slot_54 (0x6d7d30)");
}

// HD SModel vtbl +0x58 -> 0x6d7c60 (2 arg dwords)
void SModel::Slot_58()
{
    STUB_LOG("SModel::Slot_58 (0x6d7c60)");
    PZ_TRACE("SModel::Slot_58 (0x6d7c60)");
}

// HD SModel vtbl +0x5c -> 0x6d7dd0 (3 arg dwords)
void SModel::Slot_5C()
{
    STUB_LOG("SModel::Slot_5C (0x6d7dd0)");
    PZ_TRACE("SModel::Slot_5C (0x6d7dd0)");
}

// HD SModel vtbl +0x60 -> 0x6da8a0 (2 arg dwords)
void SModel::SetNodeVisible(int node, bool visible)
{
    STUB_LOG("SModel::SetNodeVisible (0x6da8a0)");
    PZ_TRACE("SModel::SetNodeVisible (0x6da8a0)");
    (void)node; (void)visible;
}

// HD SModel vtbl +0x64 -> 0x6da810 (4 arg dwords)
void SModel::Slot_64()
{
    STUB_LOG("SModel::Slot_64 (0x6da810)");
    PZ_TRACE("SModel::Slot_64 (0x6da810)");
}

// HD SModel vtbl +0x68 -> 0x6da7a0 (3 arg dwords)
void SModel::Slot_68()
{
    STUB_LOG("SModel::Slot_68 (0x6da7a0)");
    PZ_TRACE("SModel::Slot_68 (0x6da7a0)");
}

// HD SModel vtbl +0x6c -> 0x6da200 (2 arg dwords)
void SModel::Slot_6C()
{
    STUB_LOG("SModel::Slot_6C (0x6da200)");
    PZ_TRACE("SModel::Slot_6C (0x6da200)");
}

// HD SModel vtbl +0x70 -> 0x6da960 (1 arg dword)
void SModel::Slot_70()
{
    STUB_LOG("SModel::Slot_70 (0x6da960)");
    PZ_TRACE("SModel::Slot_70 (0x6da960)");
}

// HD SModel vtbl +0x74 -> 0x6da920 (1 arg dword)
void SModel::Slot_74()
{
    STUB_LOG("SModel::Slot_74 (0x6da920)");
    PZ_TRACE("SModel::Slot_74 (0x6da920)");
}

// HD SModel vtbl +0x78 -> 0x6d78c0 (0 arg dwords)
void SModel::Slot_78()
{
    STUB_LOG("SModel::Slot_78 (0x6d78c0)");
    PZ_TRACE("SModel::Slot_78 (0x6d78c0)");
}

// HD SModel vtbl +0x7c -> 0x6d7850 (1 arg dword)
void SModel::Slot_7C()
{
    STUB_LOG("SModel::Slot_7C (0x6d7850)");
    PZ_TRACE("SModel::Slot_7C (0x6d7850)");
}

// HD SModel vtbl +0x80 -> 0x6d7f00 (1 arg dword)
void SModel::Slot_80()
{
    STUB_LOG("SModel::Slot_80 (0x6d7f00)");
    PZ_TRACE("SModel::Slot_80 (0x6d7f00)");
}

// HD SModel vtbl +0x84 -> 0x6d7f30 (1 arg dword)
void SModel::Slot_84()
{
    STUB_LOG("SModel::Slot_84 (0x6d7f30)");
    PZ_TRACE("SModel::Slot_84 (0x6d7f30)");
}

// HD SModel vtbl +0x88 -> 0x6d8010 (1 arg dword)
void SModel::Slot_88()
{
    STUB_LOG("SModel::Slot_88 (0x6d8010)");
    PZ_TRACE("SModel::Slot_88 (0x6d8010)");
}

// HD SModel vtbl +0x8c -> 0x6d7e30 (0 arg dwords)
void SModel::Slot_8C()
{
    STUB_LOG("SModel::Slot_8C (0x6d7e30)");
    PZ_TRACE("SModel::Slot_8C (0x6d7e30)");
}

// HD SModel vtbl +0x90 -> 0x6d8050 (1 arg dword)
void SModel::Slot_90()
{
    STUB_LOG("SModel::Slot_90 (0x6d8050)");
    PZ_TRACE("SModel::Slot_90 (0x6d8050)");
}

// HD SModel vtbl +0x94 -> 0x6da2e0 (1 arg dword)
void SModel::SetFlags(unsigned flags)
{
    STUB_LOG("SModel::SetFlags (0x6da2e0)");
    PZ_TRACE("SModel::SetFlags (0x6da2e0)");
    (void)flags;
}

// HD SModel vtbl +0x98 -> 0x6d7910 (0 arg dwords)
void SModel::Slot_98()
{
    STUB_LOG("SModel::Slot_98 (0x6d7910)");
    PZ_TRACE("SModel::Slot_98 (0x6d7910)");
}

// HD SModel vtbl +0x9c -> 0x6dad50 (2 arg dwords)
void SModel::Slot_9C()
{
    STUB_LOG("SModel::Slot_9C (0x6dad50)");
    PZ_TRACE("SModel::Slot_9C (0x6dad50)");
}

// HD SModel vtbl +0xa0 -> 0x6d6ce0 (8 arg dwords)
void SModel::Slot_A0()
{
    STUB_LOG("SModel::Slot_A0 (0x6d6ce0)");
    PZ_TRACE("SModel::Slot_A0 (0x6d6ce0)");
}

// HD SModel vtbl +0xa4 -> 0x6d61c0 (6 arg dwords)
void SModel::Slot_A4()
{
    STUB_LOG("SModel::Slot_A4 (0x6d61c0)");
    PZ_TRACE("SModel::Slot_A4 (0x6d61c0)");
}

// HD SModel vtbl +0xa8 -> 0x6d5ca0 (2 arg dwords)
void SModel::Slot_A8()
{
    STUB_LOG("SModel::Slot_A8 (0x6d5ca0)");
    PZ_TRACE("SModel::Slot_A8 (0x6d5ca0)");
}

// HD SModel vtbl +0xac -> 0x6d8090 (2 arg dwords)
void SModel::Slot_AC()
{
    STUB_LOG("SModel::Slot_AC (0x6d8090)");
    PZ_TRACE("SModel::Slot_AC (0x6d8090)");
}

// HD SModel vtbl +0xb0 -> 0x6d6700 (0 arg dwords)
void SModel::Slot_B0()
{
    STUB_LOG("SModel::Slot_B0 (0x6d6700)");
    PZ_TRACE("SModel::Slot_B0 (0x6d6700)");
}

// HD SModel vtbl +0xb4 -> 0x6d59e0 (1 arg dword)
void SModel::Slot_B4()
{
    STUB_LOG("SModel::Slot_B4 (0x6d59e0)");
    PZ_TRACE("SModel::Slot_B4 (0x6d59e0)");
}

// HD SModel vtbl +0xb8 -> 0x6d5ae0 (1 arg dword)
void SModel::Slot_B8()
{
    STUB_LOG("SModel::Slot_B8 (0x6d5ae0)");
    PZ_TRACE("SModel::Slot_B8 (0x6d5ae0)");
}

// HD SModel vtbl +0xbc -> 0x6dae10 (1 arg dword)
void SModel::Slot_BC()
{
    STUB_LOG("SModel::Slot_BC (0x6dae10)");
    PZ_TRACE("SModel::Slot_BC (0x6dae10)");
}

// HD SModel vtbl +0xc0 -> 0x6dad40 (1 arg dword)
void SModel::Slot_C0()
{
    STUB_LOG("SModel::Slot_C0 (0x6dad40)");
    PZ_TRACE("SModel::Slot_C0 (0x6dad40)");
}

// HD SModel vtbl +0xc4 -> 0x6d7ef0 (0 arg dwords)
void SModel::Slot_C4()
{
    STUB_LOG("SModel::Slot_C4 (0x6d7ef0)");
    PZ_TRACE("SModel::Slot_C4 (0x6d7ef0)");
}

// HD SModel vtbl +0xc8 -> 0x6d7920 (2 arg dwords)
void SModel::Slot_C8()
{
    STUB_LOG("SModel::Slot_C8 (0x6d7920)");
    PZ_TRACE("SModel::Slot_C8 (0x6d7920)");
}

// HD SModel vtbl +0xcc -> 0x6dad80 (1 arg dword)
void SModel::Slot_CC()
{
    STUB_LOG("SModel::Slot_CC (0x6dad80)");
    PZ_TRACE("SModel::Slot_CC (0x6dad80)");
}

// HD SModel vtbl +0xd0 -> 0x6db7c0 (1 arg dword)
void SModel::Slot_D0()
{
    STUB_LOG("SModel::Slot_D0 (0x6db7c0)");
    PZ_TRACE("SModel::Slot_D0 (0x6db7c0)");
}

// HD SModel vtbl +0xd4 -> 0x6db960 (1 arg dword)
void SModel::Slot_D4()
{
    STUB_LOG("SModel::Slot_D4 (0x6db960)");
    PZ_TRACE("SModel::Slot_D4 (0x6db960)");
}

// HD SModel vtbl +0xd8 -> 0x6db550 (2 arg dwords)
void SModel::Slot_D8()
{
    STUB_LOG("SModel::Slot_D8 (0x6db550)");
    PZ_TRACE("SModel::Slot_D8 (0x6db550)");
}

// HD SModel vtbl +0xdc -> 0x6d5910 (2 arg dwords)
void SModel::Slot_DC()
{
    STUB_LOG("SModel::Slot_DC (0x6d5910)");
    PZ_TRACE("SModel::Slot_DC (0x6d5910)");
}

// HD SModel vtbl +0xe0 -> 0x6d7310 (0 arg dwords)
void SModel::Slot_E0()
{
    STUB_LOG("SModel::Slot_E0 (0x6d7310)");
    PZ_TRACE("SModel::Slot_E0 (0x6d7310)");
}

// HD SModel vtbl +0xe4 -> 0x6d62e0 (2 arg dwords)
void SModel::Slot_E4()
{
    STUB_LOG("SModel::Slot_E4 (0x6d62e0)");
    PZ_TRACE("SModel::Slot_E4 (0x6d62e0)");
}

// HD SModel vtbl +0xe8 -> 0x6dade0 (3 arg dwords)
void SModel::SetSway(float phase, float p2, float p3)
{
    STUB_LOG("SModel::SetSway (0x6dade0)");
    PZ_TRACE("SModel::SetSway (0x6dade0)");
    (void)phase; (void)p2; (void)p3;
}

// HD SModel vtbl +0xec -> 0x6da310 (2 arg dwords)
void SModel::Slot_EC()
{
    STUB_LOG("SModel::Slot_EC (0x6da310)");
    PZ_TRACE("SModel::Slot_EC (0x6da310)");
}

// HD SModel vtbl +0xf0 -> 0x6da2c0 (2 arg dwords)
void SModel::Slot_F0()
{
    STUB_LOG("SModel::Slot_F0 (0x6da2c0)");
    PZ_TRACE("SModel::Slot_F0 (0x6da2c0)");
}

// HD SModel vtbl +0xf4 -> 0x6d7900 (0 arg dwords)
void SModel::Slot_F4()
{
    STUB_LOG("SModel::Slot_F4 (0x6d7900)");
    PZ_TRACE("SModel::Slot_F4 (0x6d7900)");
}

// HD SModel vtbl +0xf8 -> 0x6d85d0 (0 arg dwords)
void SModel::Slot_F8()
{
    STUB_LOG("SModel::Slot_F8 (0x6d85d0)");
    PZ_TRACE("SModel::Slot_F8 (0x6d85d0)");
}

// HD SModel vtbl +0xfc -> 0x6d6f40 (6 arg dwords)
void SModel::Slot_FC()
{
    STUB_LOG("SModel::Slot_FC (0x6d6f40)");
    PZ_TRACE("SModel::Slot_FC (0x6d6f40)");
}

// HD SModel vtbl +0x100 -> 0x6d7150 (4 arg dwords)
void SModel::Slot_100()
{
    STUB_LOG("SModel::Slot_100 (0x6d7150)");
    PZ_TRACE("SModel::Slot_100 (0x6d7150)");
}

// HD SModel vtbl +0x104 -> 0x6d7400 (2 arg dwords)
void SModel::Slot_104()
{
    STUB_LOG("SModel::Slot_104 (0x6d7400)");
    PZ_TRACE("SModel::Slot_104 (0x6d7400)");
}

// ---- generated slot stubs: SIAttachable ----

// HD SModel vtbl +0x04 -> 0x6dba80 (2 arg dwords)
int SModel::Update(int p1, int p2)
{
    STUB_LOG("SModel::Update (0x6dba80)");
    PZ_TRACE("SModel::Update (0x6dba80)");
    (void)p1; (void)p2;
    return 0;
}

// HD SModel vtbl +0x08 -> 0x6d86e0 (0 arg dwords)
void SModel::Attach_08()
{
    STUB_LOG("SModel::Attach_08 (0x6d86e0)");
    PZ_TRACE("SModel::Attach_08 (0x6d86e0)");
}

// HD SModel vtbl +0x0c -> 0x6d86f0 (1 arg dword)
void SModel::Attach_0C()
{
    STUB_LOG("SModel::Attach_0C (0x6d86f0)");
    PZ_TRACE("SModel::Attach_0C (0x6d86f0)");
}

} // namespace pz
