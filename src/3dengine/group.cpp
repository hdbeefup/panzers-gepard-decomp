// 3dengine/group.cpp
// Scene graph groups and transformations
// Decompiled from: gameSplit/sgroup.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "group.h"

static inline unsigned int float_bits(float f) { unsigned int u; memcpy(&u, &f, sizeof(u)); return u; }
#include "gepard.h"
#include "mesh.h"
#include "terrain.h"
#include "texture.h"
#include "logger.h"

// Verify critical struct offsets match the original binary (x86 only —
// on x64/ARM64 pointer-size changes legitimately shift these offsets)
#if defined(_M_IX86)
static_assert(offsetof(SGroup, Flags) == 0x88, "Flags offset mismatch — SGroup layout is wrong");
static_assert(offsetof(SGroup, Transformation) == 0x1C, "Transformation offset mismatch — SGroup layout is wrong");
static_assert(offsetof(SGroup, LastTransformation) == 0x3C, "LastTransformation offset mismatch — SGroup layout is wrong");
static_assert(offsetof(SGroup, Gepard) == 0x0C, "Gepard offset mismatch — SGroup layout is wrong");
static_assert(offsetof(SGroup, VisClass) == 0x12C, "VisClass offset mismatch — SGroup layout is wrong");
#endif
#include "stream.h"
#include "core_common.h"

#include <new>
#include "hdbeefup.h"


// SString default empty buffer — IDA uses &fullpath for empty string default
static char _fullpath_buf[4] = {0};
static const char *fullpath = _fullpath_buf;

// Lighting type constants
#define LT_AMBIENT 1
#define LT_NORMAL 0

// Classes: SGroup, STransformation, SMatrix4
// Function count: 74

//----- (00446C50) --------------------------------------------------------

SGroup::SGroup(const SGroup *src, bool dynamic, int gepard_index)

{
  const SGroup *v5; // edx
  int v6;
  int maxsize;
  int v8;
  SMeshProp *v9; // eax
  int v10;
  int size;
  char *v12; // eax
  int v13;
  void *v14; // eax
  SMeshProp *array; // ecx
  SMeshProp *v16; // eax
  SMeshProp *v17; // eax
  SMeshProp *v18; // eax
  SMeshProp *v19; // eax
  int v20;
  SMesh *v21; // ecx
  SMeshProp *v22; // ecx
  SMeshProp *v23; // eax
  D3DXQUATERNION Rotation; // xmm0
  int v25;
  char *v26;
  SMeshProp *v27;
  int v28;
  int v29;
  char *v30;
  SMeshProp *v31;
  this->RefCount = 1;
  this->MeshArray.array = 0;
  this->MeshArray.size = 0;
  this->MeshArray.maxsize = 0;
  this->Transformation.Translation.x = 0.0;
  this->Transformation.Translation.y = 0.0;
  this->Transformation.Translation.z = 0.0;
  this->Transformation.Scale = 1.0;
  this->Transformation.Rotation.z = 0.0;
  this->Transformation.Rotation.y = 0.0;
  this->Transformation.Rotation.x = 0.0;
  this->Transformation.Rotation.w = 1.0;
  this->LastTransformation.Translation.x = 0.0;
  this->LastTransformation.Translation.y = 0.0;
  this->LastTransformation.Translation.z = 0.0;
  this->LastTransformation.Scale = 1.0;
  this->LastTransformation.Rotation.z = 0.0;
  this->LastTransformation.Rotation.y = 0.0;
  this->LastTransformation.Rotation.x = 0.0;
  this->LastTransformation.Rotation.w = 1.0;
  memset(&this->ColorizeConstants, 0, sizeof(this->ColorizeConstants));
  this->Effects.first = 0;
  this->Effects.last = 0;
  this->Effects.current = 0;
  this->Effects.Closed = 0;
  this->Effects.NumItems = 0;
  v5 = src;
  this->Activated = 0;
  v28 = 0;
  this->lpD3DDev = src->lpD3DDev;
  this->Gepard = src->Gepard;
  this->DrawType = src->DrawType;
  this->ambientMode = 0;
  if ( src->MeshArray.size > 0 )
  {
    v6 = 0;
    v25 = 0;
    do
    {
      maxsize = this->MeshArray.maxsize;
      if ( this->MeshArray.size == maxsize )
      {
        if ( maxsize >= 16 )
          v8 = 6 * maxsize / 5;
        else
          v8 = 16;
        v29 = v8;
        v9 = (SMeshProp *)realloc(this->MeshArray.array, (int)sizeof(SMeshProp) * v8);
        v10 = this->MeshArray.maxsize;
        this->MeshArray.array = v9;
        memset(&v9[v10], 0, sizeof(SMeshProp) * (v29 - v10));
        v6 = v25;
        v5 = src;
        this->MeshArray.maxsize = v29;
      }
      size = this->MeshArray.size;
      this->MeshArray.size = size + 1;
      if ( v28 != size )
        Logger.g->Panic("SGroup::SGroup: Damaged MeshArray");
      // x64 fix: original IDA decomp accessed SMeshProp.MeshName via _DWORD
      // pointers (`*(_DWORD*)v12` for buf, `*((_DWORD*)v12+1)` for size).
      // That layout is x86-only (SString = 4-byte buf + 4-byte size = 8 bytes).
      // On x64 SString = 8-byte buf + 4-byte size + 4-byte pad = 16 bytes, so
      // `*((_DWORD*)v12+1)` was reading the upper 32 bits of buf (not size),
      // and `*(_DWORD*)v12 = (_DWORD)ptr` truncated a 64-bit pointer. Result
      // was a junk pointer that crashed in ~SGroup or the next assignment.
      {
        SMeshProp *src_mp = (SMeshProp *)((char *)v5->MeshArray.array + v6);
        SMeshProp *dst_mp = (SMeshProp *)((char *)this->MeshArray.array + v6);
        if (dst_mp->MeshName.buf)
        {
          delete[] dst_mp->MeshName.buf;
          dst_mp->MeshName.buf = nullptr;
        }
        if (src_mp->MeshName.size)
        {
          dst_mp->MeshName.size = src_mp->MeshName.size;
          dst_mp->MeshName.buf = new char[src_mp->MeshName.size + 1];
          memcpy(dst_mp->MeshName.buf, src_mp->MeshName.buf, src_mp->MeshName.size + 1);
        }
        else
        {
          dst_mp->MeshName.size = 0;
          dst_mp->MeshName.buf = nullptr;
        }
      }
      (void)v12; (void)v26; (void)v30; (void)v13; (void)v14;
      *(int *)((char *)&this->MeshArray.array->Parent + v6) = *(int *)((char *)&src->MeshArray.array->Parent + v6);
      array = src->MeshArray.array;
      v16 = this->MeshArray.array;
      memcpy((char *)&v16->OriginMatrix + v6, (char *)&array->OriginMatrix + v6, sizeof(D3DXMATRIX));
      v17 = this->MeshArray.array;
      *(float *)((char *)&v17->Transformation.Translation.x + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Translation.y + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Translation.z + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Scale + v6) = 1.0;
      *(float *)((char *)&v17->Transformation.Rotation.z + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Rotation.y + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Rotation.x + v6) = 0.0;
      *(float *)((char *)&v17->Transformation.Rotation.w + v6) = 1.0;
      v18 = this->MeshArray.array;
      *(float *)((char *)&v18->WorldMatrix._43 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._42 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._41 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._34 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._32 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._31 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._24 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._23 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._21 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._14 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._13 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._12 + v6) = 0.0;
      *(float *)((char *)&v18->WorldMatrix._44 + v6) = 1.0;
      *(float *)((char *)&v18->WorldMatrix._33 + v6) = 1.0;
      *(float *)((char *)&v18->WorldMatrix._22 + v6) = 1.0;
      *(float *)((char *)&v18->WorldMatrix._11 + v6) = 1.0;
      // x64 fix: original read SPtr<SMesh>::ptr as `*(int *)`, truncating the
      // pointer to 32 bits. On x64 if the pointer has non-zero upper bits
      // the truncated value indexes random memory. Use typed pointer access.
      {
        SMeshProp *src_mp = (SMeshProp *)((char *)src->MeshArray.array + v6);
        SMeshProp *dst_mp = (SMeshProp *)((char *)this->MeshArray.array + v6);
        if (dst_mp->Mesh.ptr)
        {
          dst_mp->Mesh.ptr->Release();
          dst_mp->Mesh.ptr = nullptr;
        }
        SMesh *src_mesh = src_mp->Mesh.ptr;
        dst_mp->Mesh.ptr = src_mesh;
        if (src_mesh)
          src_mesh->AddRef();
      }
      (void)v19; (void)v20; (void)v21; (void)v27; (void)v31;
      v5 = src;
      *(&this->MeshArray.array->Visible + v6) = *(&src->MeshArray.array->Visible + v6);
      *(&this->MeshArray.array->CanCastShadows + v6) = *(&src->MeshArray.array->CanCastShadows + v6);
      v22 = src->MeshArray.array;
      v23 = this->MeshArray.array;
      memcpy((char *)&v23->Bound[0] + v6, (char *)&v22->Bound[0] + v6, sizeof(D3DXVECTOR3) * 8);
      // x64 fix: original literal `v6 += 320` assumed x86 sizeof(SMeshProp)==320.
      // On x64 the embedded SString and SPtr<SMesh> grow, so use sizeof to keep
      // the byte stride correct on both archs.
      v6 += (int)sizeof(SMeshProp);
      ++v28;
      v25 = v6;
    }
    while ( v28 < src->MeshArray.size );
  }
  memcpy(&this->Transformation.Translation, &v5->Transformation.Translation, sizeof(D3DXVECTOR3) + sizeof(float));
  Rotation = v5->Transformation.Rotation;
  this->Type = !dynamic + 1;
  this->GepardIndex = gepard_index;
  this->Transformation.Rotation = Rotation;
  this->Recalculate = 1;
  this->LastRecalculated = -1;
  this->NeedNewShadow = 1;
  this->Flags = 0;
  this->ShadowTexture = -1;
  this->ShadowMask2 = 0;
  this->ShadowMesh = 0;
  this->Visible = 1;
  this->VisClass = dynamic;
  this->Selection = 0;
  this->IsColorized = 0;
  Rotation.x = (float)((float)((float)rand() / 32767.0f) * 0.1f) + 0.2f;
  this->treeBendFreqX = Rotation.x;
  this->treeBendFreqZ = Rotation.x * 1.3f;
  this->treeBendPhaseX = (float)((float)rand() / 32767.0) * 6.2831855f;
  this->treeBendPhaseZ = (float)((float)rand() / 32767.0) * 6.2831855f;
}

//----- (004471C0) --------------------------------------------------------

SGroup::SGroup(SGepard *gepard, SDrawType drawtype)

{
  this->RefCount = 1;
  this->MeshArray.array = 0;
  this->MeshArray.size = 0;
  this->MeshArray.maxsize = 0;
  this->Transformation.Translation.x = 0.0;
  this->Transformation.Translation.y = 0.0;
  this->Transformation.Translation.z = 0.0;
  this->Transformation.Scale = 1.0;
  this->Transformation.Rotation.z = 0.0;
  this->Transformation.Rotation.y = 0.0;
  this->Transformation.Rotation.x = 0.0;
  this->Transformation.Rotation.w = 1.0;
  this->LastTransformation.Translation.x = 0.0;
  this->LastTransformation.Translation.y = 0.0;
  this->LastTransformation.Translation.z = 0.0;
  this->LastTransformation.Scale = 1.0;
  this->LastTransformation.Rotation.z = 0.0;
  this->LastTransformation.Rotation.y = 0.0;
  this->LastTransformation.Rotation.x = 0.0;
  this->LastTransformation.Rotation.w = 1.0;
  memset(&this->ColorizeConstants, 0, sizeof(this->ColorizeConstants));
  this->Effects.first = 0;
  this->Effects.last = 0;
  this->Effects.current = 0;
  this->Effects.Closed = 0;
  this->Effects.NumItems = 0;
  this->Gepard = gepard;
  this->Activated = 0;
  this->lpD3DDev = gepard->lpD3DDev;
  this->DrawType = drawtype;
  this->ambientMode = 0;
  this->Recalculate = 1;
  this->LastRecalculated = -1;
  this->NeedNewShadow = 1;
  this->Flags = 0;
  this->ShadowTexture = -1;
  this->ShadowMask2 = 0;
  this->ShadowMesh = 0;
  this->Type = 0;
  this->Visible = 1;
  this->VisClass = 0;
  this->Selection = 0;
  this->IsColorized = 0;
}

//----- (004473B0) --------------------------------------------------------

STransformation::STransformation(STransformation *trans0, STransformation *trans1, float interpolation)

{
  this->Translation.x = (float)((float)(trans1->Translation.x - trans0->Translation.x) * interpolation)
                      + trans0->Translation.x;
  this->Translation.y = (float)((float)(trans1->Translation.y - trans0->Translation.y) * interpolation)
                      + trans0->Translation.y;
  this->Translation.z = (float)((float)(trans1->Translation.z - trans0->Translation.z) * interpolation)
                      + trans0->Translation.z;
  this->Scale = (float)((float)(trans1->Scale - trans0->Scale) * interpolation) + trans0->Scale;
  D3DXQuaternionSlerp(&this->Rotation, &trans0->Rotation, &trans1->Rotation, interpolation);
}

//----- (00447440) --------------------------------------------------------

STransformation::STransformation()

{
  this->Translation.x = 0.0;
  this->Translation.y = 0.0;
  this->Translation.z = 0.0;
  this->Scale = 1.0;
  this->Rotation.z = 0.0;
  this->Rotation.y = 0.0;
  this->Rotation.x = 0.0;
  this->Rotation.w = 1.0;
}

// IDA-generated SChain<SGroup::SEffects> destructor — handled by template
#if 0
void SChain<SGroup::SEffects>::~SChain<SGroup::SEffects>(SChain<SGroup::SEffects> *this)
{
  SGroup::SEffects *first = this->first;
  if ( first )
  {
    do
    {
      SGroup::SEffects *next = first->next;
      ::operator delete(first);
      first = next;
    }
    while ( first );
  }
  this->first = 0;
  this->last = 0;
  this->current = 0;
  this->NumItems = 0;
}
#endif

//----- (004475D0) --------------------------------------------------------

SGroup::~SGroup()

{
  bool v2;
  SShadowMask2 *ShadowMask2; // esi
  SMesh *ShadowMesh; // ecx
  int Type;
  SGroup::SEffects *i; // edx
  SGroup::SEffects *current; // eax
  SGroup::SEffects *next; // edi
  SGroup::SEffects *prev; // eax
  SGroup::SEffects *v10;
  v2 = this->RefCount == 0;
  if ( !v2 )
    Logger.g->Panic("SGroup::~SGroup: Object deleted instead of release");
  this->Gepard->ReleaseTexture(this->ShadowTexture, 0);
  ShadowMask2 = this->ShadowMask2;
  this->ShadowTexture = -1;
  if ( ShadowMask2 )
  {
    delete ShadowMask2;
    this->ShadowMask2 = 0;
  }
  ShadowMesh = this->ShadowMesh;
  if ( ShadowMesh )
  {
    ShadowMesh->Release();
    this->ShadowMesh = 0;
  }
  Type = this->Type;
  if ( Type == 1 )
  {
    this->Gepard->RemoveDynamicObject(this->GepardIndex);
  }
  else if ( Type == 2 )
  {
    this->Gepard->RemoveHashedObject(this->GepardIndex);
  }
  this->Effects.current = this->Effects.first;
  for ( i = this->Effects.current; i; i = this->Effects.current )
  {
    this->Gepard->StopEffect(i->effect);
    current = this->Effects.current;
    if ( current )
    {
      next = current->next;
      prev = current->prev;
      v10 = prev;
      if ( next )
        next->prev = prev;
      if ( prev )
        prev->next = next;
      operator delete(this->Effects.current);
      if ( next )
      {
        this->Effects.current = next;
      }
      else
      {
        this->Effects.last = v10;
        this->Effects.current = 0;
      }
      if ( !v10 )
        this->Effects.first = next;
      --this->Effects.NumItems;
    }
  }
  this->Effects.~SChain();
  this->MeshArray.~SDArray();
}

//----- (00447740) --------------------------------------------------------

SMatrix4 SMatrix4::operator*(const SMatrix4 &mat) const
{
  SMatrix4 v6;
  v6 = *this;
  v6 *= mat;
  return v6;
}

//----- (00447800) --------------------------------------------------------

SMatrix4 &SMatrix4::operator*=(const SMatrix4 &mat)

{
  float m01; // xmm5_4
  float m00; // xmm4_4
  float m02; // xmm2_4
  float m03; // xmm3_4
  float v7; // xmm0_4
  float m12; // xmm2_4
  float v9; // xmm5_4
  float m10; // xmm4_4
  float v11; // xmm5_4
  float v12; // xmm0_4
  float m13; // xmm3_4
  float m11; // xmm5_4
  float v15; // xmm0_4
  float v16; // xmm5_4
  float m22; // xmm2_4
  float m20; // xmm4_4
  float v19; // xmm5_4
  float v20; // xmm0_4
  float m23; // xmm3_4
  float m21; // xmm5_4
  float v23; // xmm0_4
  float v24; // xmm5_4
  float m32; // xmm2_4
  float m30; // xmm4_4
  float v27; // xmm5_4
  float v28; // xmm0_4
  float m33; // xmm3_4
  float m31; // xmm5_4
  float v31; // xmm5_4
  m01 = this->m01;
  m00 = this->m00;
  m02 = this->m02;
  m03 = this->m03;
  this->m00 = (float)((float)((float)(mat.m00 * this->m00) + (float)(m01 * mat.m10)) + (float)(m02 * mat.m20))
            + (float)(mat.m30 * m03);
  this->m01 = (float)((float)((float)(mat.m11 * m01) + (float)(mat.m01 * m00)) + (float)(m02 * mat.m21))
            + (float)(mat.m31 * m03);
  this->m02 = (float)((float)((float)(m01 * mat.m12) + (float)(m00 * mat.m02)) + (float)(mat.m22 * m02))
            + (float)(mat.m32 * m03);
  v7 = mat.m23 * m02;
  m12 = this->m12;
  v9 = (float)(m01 * mat.m13) + (float)(m00 * mat.m03);
  m10 = this->m10;
  v11 = v9 + v7;
  v12 = mat.m33 * m03;
  m13 = this->m13;
  this->m03 = v11 + v12;
  m11 = this->m11;
  this->m10 = (float)((float)((float)(mat.m00 * m10) + (float)(m11 * mat.m10)) + (float)(m12 * mat.m20))
            + (float)(mat.m30 * m13);
  this->m11 = (float)((float)((float)(mat.m11 * m11) + (float)(mat.m01 * m10)) + (float)(m12 * mat.m21))
            + (float)(mat.m31 * m13);
  this->m12 = (float)((float)((float)(m11 * mat.m12) + (float)(m10 * mat.m02)) + (float)(mat.m22 * m12))
            + (float)(mat.m32 * m13);
  v15 = mat.m23 * m12;
  v16 = (float)(m11 * mat.m13) + (float)(m10 * mat.m03);
  m22 = this->m22;
  m20 = this->m20;
  v19 = v16 + v15;
  v20 = mat.m33 * m13;
  m23 = this->m23;
  this->m13 = v19 + v20;
  m21 = this->m21;
  this->m20 = (float)((float)((float)(mat.m00 * m20) + (float)(m21 * mat.m10)) + (float)(m22 * mat.m20))
            + (float)(mat.m30 * m23);
  this->m21 = (float)((float)((float)(mat.m11 * m21) + (float)(mat.m01 * m20)) + (float)(m22 * mat.m21))
            + (float)(mat.m31 * m23);
  this->m22 = (float)((float)((float)(m21 * mat.m12) + (float)(m20 * mat.m02)) + (float)(mat.m22 * m22))
            + (float)(mat.m32 * m23);
  v23 = mat.m23 * m22;
  v24 = (float)(m21 * mat.m13) + (float)(m20 * mat.m03);
  m32 = this->m32;
  m30 = this->m30;
  v27 = v24 + v23;
  v28 = mat.m33 * m23;
  m33 = this->m33;
  this->m23 = v27 + v28;
  m31 = this->m31;
  this->m30 = (float)((float)((float)(mat.m00 * m30) + (float)(m31 * mat.m10)) + (float)(m32 * mat.m20))
            + (float)(mat.m30 * m33);
  this->m31 = (float)((float)((float)(mat.m11 * m31) + (float)(mat.m01 * m30)) + (float)(m32 * mat.m21))
            + (float)(mat.m31 * m33);
  this->m32 = (float)((float)((float)(m31 * mat.m12) + (float)(m30 * mat.m02)) + (float)(mat.m22 * m32))
            + (float)(mat.m32 * m33);
  v31 = (float)((float)(m31 * mat.m13) + (float)(m30 * mat.m03)) + (float)(mat.m23 * m32);
  this->m33 = v31 + (float)(mat.m33 * m33);
  return *this;
}

//----- (00447B80) --------------------------------------------------------

// Compiler-generated scalar deleting destructor — not needed
#if 0
SGroup *SGroup_scalar_deleting_destructor(SGroup *self, char a2) { ... }
#endif

//----- (00447D90) --------------------------------------------------------

void SGroup::AddRef()

{
  ++this->RefCount;
}

//----- (00447DA0) --------------------------------------------------------

bool SGroup::CheckClickBox(float click_x1, float click_y1, float click_x2, float click_y2)

{
  STransformation *p_Transformation; // edi
  SGepard *Gepard; // edi
  SGepard *v9; // ebx
  float Interpolation;
  STransformation trans;
  p_Transformation = &this->Transformation;
  if ( (this->Flags & 2) != 0 )
  {
    Interpolation = (float)(this->Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_Transformation->Translation.x)
                                * Interpolation)
                        + p_Transformation->Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                * Interpolation)
                        + this->Transformation.Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                * Interpolation)
                        + this->Transformation.Translation.z;
    trans.Scale = (float)((float)(this->LastTransformation.Scale - this->Transformation.Scale) * Interpolation)
                + this->Transformation.Scale;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &this->Transformation.Rotation,
      &this->LastTransformation.Rotation,
      Interpolation);
    Gepard = this->Gepard;
    if ( Gepard->Terrain->IsVisible(trans.Translation.x, trans.Translation.z, this->VisClass) )
      return Gepard->CheckClickBox(&trans.Translation, click_x1, click_y1, click_x2, click_y2);
  }
  else
  {
    v9 = this->Gepard;
 if ( v9->Terrain->IsVisible(p_Transformation->Translation.x, this->Transformation.Translation.z, this->VisClass) )
    {
      return v9->CheckClickBox(&p_Transformation->Translation, click_x1, click_y1, click_x2, click_y2);
    }
  }
  return 0;
}

//----- (00447F60) --------------------------------------------------------

void SGroup::ClearEffect(void *effect)

{
  SGroup::SEffects *v3; // ebx
  SGroup::SEffects *first; // esi
  SGroup::SEffects **eff = (SGroup::SEffects **)effect;
  if ( effect )
  {
    v3 = eff[0];
    first = eff[1];
    if ( first )
      first->prev = v3;
    if ( v3 )
      v3->next = first;
    ::operator delete(effect);
    if ( !first )
      this->Effects.last = v3;
    if ( v3 )
    {
      first = this->Effects.first;
      --this->Effects.NumItems;
    }
    else
    {
      --this->Effects.NumItems;
      this->Effects.first = first;
    }
    this->Effects.current = first;
  }
}

//----- (00447FD0) --------------------------------------------------------

void SGroup::ClearShadow(bool keepmask)

{
  SShadowMask2 *ShadowMask2; // edi
  SMesh *ShadowMesh; // ecx
  this->Gepard->ReleaseTexture(this->ShadowTexture, 0);
  this->ShadowTexture = -1;
  if ( !keepmask )
  {
    ShadowMask2 = this->ShadowMask2;
    if ( ShadowMask2 )
    {
      delete ShadowMask2;
      this->ShadowMask2 = 0;
    }
    this->NeedNewShadow = 1;
  }
  ShadowMesh = this->ShadowMesh;
  if ( ShadowMesh )
  {
    ShadowMesh->Release();
    this->ShadowMesh = 0;
  }
}

//----- (00448050) --------------------------------------------------------

void SGroup::ComputeProjectedShadowBounding(float *umin, float *umax, float *vmin, float *vmax)

{
  bool v6;
  STransformation *p_Transformation; // ecx
  SMatrix4 *Matrix; // eax
  bool v9; // cc
  int v10;
  SMeshProp *array; // ecx
  float v12; // xmm2_4
  SMatrix4 *v13; // eax
  SMeshProp *v14; // ecx
  int Parent;
  D3DXMATRIX *p_world; // eax
  float *v17; // eax
  int v18;
  SMeshProp *v19; // eax
  int i;
  float x; // xmm1_4
  float y; // xmm1_4
  STransformation *p_trans; // ecx
  int v24;
  int v25;
  SMeshProp *v26; // ecx
  float v27; // xmm2_4
  SMatrix4 *v28; // eax
  SMeshProp *v29; // ecx
  int v30;
  D3DXMATRIX *p_WorldMatrix; // eax
  float Interpolation;
  D3DXMATRIX *p_OriginMatrix;
  D3DXMATRIX *p_SunProjection;
  float v35;
  D3DXMATRIX *v36;
  SMatrix4 result;
  float *v38;
  float *v39;
  float *v40;
  float *v41;
  int v42;
  D3DXMATRIX mat;
  D3DXVECTOR3 vec;
  D3DXMATRIX dynamic;
  D3DXMATRIX world;
  STransformation trans;
  v41 = umin;
  v40 = vmin;
  v6 = (this->Flags & 2) == 0;
  p_Transformation = &this->Transformation;
  v38 = umax;
  v39 = vmax;
  if ( !v6 )
  {
    Interpolation = (float)(this->Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_Transformation->Translation.x)
                                * Interpolation)
                        + p_Transformation->Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - p_Transformation->Translation.y)
                                * Interpolation)
                        + p_Transformation->Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - p_Transformation->Translation.z)
                                * Interpolation)
                        + p_Transformation->Translation.z;
    trans.Scale = (float)((float)(this->LastTransformation.Scale - p_Transformation->Scale) * Interpolation)
                + p_Transformation->Scale;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &p_Transformation->Rotation,
      &this->LastTransformation.Rotation,
      Interpolation);
    p_Transformation = &trans;
  }
  Matrix = p_Transformation->GetMatrix(&result);
  v9 = this->MeshArray.size <= 0;
  v42 = 0;
  memcpy(&world, Matrix, 48);
  world._44 = Matrix->m33;
  memset(world.m[3], 0, 12);
  if ( !v9 )
  {
    v10 = 0;
    do
    {
      if ( (this->Flags & 4) != 0 )
      {
        array = this->MeshArray.array;
        v12 = (float)(this->Gepard->Interpolation);

        trans.Translation.x = (float)((float)(array[v10].LastTransformation.Translation.x
                                            - array[v10].Transformation.Translation.x)
                                    * v12)
                            + array[v10].Transformation.Translation.x;
        trans.Translation.y = (float)((float)(array[v10].LastTransformation.Translation.y
                                            - array[v10].Transformation.Translation.y)
                                    * v12)
                            + array[v10].Transformation.Translation.y;
        trans.Translation.z = (float)((float)(array[v10].LastTransformation.Translation.z
                                            - array[v10].Transformation.Translation.z)
                                    * v12)
                            + array[v10].Transformation.Translation.z;
        trans.Scale = (float)((float)(array[v10].LastTransformation.Scale - array[v10].Transformation.Scale) * v12)
                    + array[v10].Transformation.Scale;
        D3DXQuaternionSlerp(
          &trans.Rotation,
          &array[v10].Transformation.Rotation,
          &array[v10].LastTransformation.Rotation,
          v12);
        v13 = trans.GetMatrix(&result);
      }
      else
      {
        v13 = this->MeshArray.array[v10].Transformation.GetMatrix(&result);
      }
      memcpy(&dynamic, v13, sizeof(D3DXMATRIX));
      p_OriginMatrix = &this->MeshArray.array[v10].OriginMatrix;
      D3DXMatrixMultiply(&mat, &dynamic, p_OriginMatrix);
      v14 = this->MeshArray.array;
      Parent = v14[v10].Parent;
      if ( Parent < 0 )
        p_world = &world;
      else
        p_world = &v14[Parent].WorldMatrix;
      D3DXMatrixMultiply(&v14[v10++].WorldMatrix, &mat, p_world);
      ++v42;
    }
    while ( v42 < this->MeshArray.size );
  }
  v42 = 0;
  *v41 = 10000.0;
  v17 = v40;
  *umax = -10000.0;
  *v17 = 10000.0;
  *v39 = -10000.0;
  if ( this->MeshArray.size > 0 )
  {
    v18 = 0;
    do
    {
      v19 = this->MeshArray.array;
      memcpy(&mat, &v19[v18].WorldMatrix, sizeof(D3DXMATRIX));
      p_SunProjection = &this->Gepard->SunProjection;
      D3DXMatrixMultiply(&mat, &mat, p_SunProjection);
      for ( i = 0; i < 8; ++i )
      {
        D3DXVec3TransformCoord(&vec, &this->MeshArray.array[v18].Bound[i], &mat);
        x = vec.x;
        if ( *v41 > vec.x )
          *v41 = vec.x;
        if ( x > *v38 )
          *v38 = x;
        y = vec.y;
        if ( *v40 > vec.y )
          *v40 = vec.y;
        if ( y > *v39 )
          *v39 = y;
      }
      ++v18;
      ++v42;
    }
    while ( v42 < this->MeshArray.size );
  }
  v6 = (this->Flags & 2) == 0;
  p_trans = &this->Transformation;
  this->Recalculate = 0;
  if ( !v6 )
  {
    v35 = (float)(this->Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_trans->Translation.x) * v35)
                        + p_trans->Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                * v35)
                        + this->Transformation.Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                * v35)
                        + this->Transformation.Translation.z;
    trans.Scale = (float)((float)(this->LastTransformation.Scale - this->Transformation.Scale) * v35)
                + this->Transformation.Scale;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &this->Transformation.Rotation,
      &this->LastTransformation.Rotation,
      v35);
    p_trans = &trans;
  }
  v24 = 0;
  world = *(D3DXMATRIX *)p_trans->GetMatrix(&result);
  if ( this->MeshArray.size > 0 )
  {
    v25 = 0;
    do
    {
      if ( (this->Flags & 4) != 0 )
      {
        v26 = this->MeshArray.array;
        v27 = (float)(this->Gepard->Interpolation);

        trans.Translation.x = (float)((float)(v26[v25].LastTransformation.Translation.x
                                            - v26[v25].Transformation.Translation.x)
                                    * v27)
                            + v26[v25].Transformation.Translation.x;
        trans.Translation.y = (float)((float)(v26[v25].LastTransformation.Translation.y
                                            - v26[v25].Transformation.Translation.y)
                                    * v27)
                            + v26[v25].Transformation.Translation.y;
        trans.Translation.z = (float)((float)(v26[v25].LastTransformation.Translation.z
                                            - v26[v25].Transformation.Translation.z)
                                    * v27)
                            + v26[v25].Transformation.Translation.z;
        trans.Scale = (float)((float)(v26[v25].LastTransformation.Scale - v26[v25].Transformation.Scale) * v27)
                    + v26[v25].Transformation.Scale;
        D3DXQuaternionSlerp(
          &trans.Rotation,
          &v26[v25].Transformation.Rotation,
          &v26[v25].LastTransformation.Rotation,
          v27);
        v28 = trans.GetMatrix(&result);
      }
      else
      {
        v28 = this->MeshArray.array[v25].Transformation.GetMatrix(&result);
      }
      memcpy(&dynamic, v28, sizeof(D3DXMATRIX));
      v36 = &this->MeshArray.array[v25].OriginMatrix;
      D3DXMatrixMultiply(&mat, &dynamic, v36);
      v29 = this->MeshArray.array;
      v30 = v29[v25].Parent;
      if ( v30 < 0 )
        p_WorldMatrix = &world;
      else
        p_WorldMatrix = &v29[v30].WorldMatrix;
      D3DXMatrixMultiply(&v29[v25].WorldMatrix, &mat, p_WorldMatrix);
      ++v24;
      ++v25;
    }
    while ( v24 < this->MeshArray.size );
  }
}

//----- (00448690) --------------------------------------------------------

void SGroup::ComputeShadowBounding(float *xmin, float *xmax, float *zmin, float *zmax)

{
  double Height; // st7
  SGroup *v6; // edx
  SGepard *Gepard; // eax
  float y; // xmm0_4
  float z; // xmm2_4
  float v10; // xmm1_4
  float v11; // xmm2_4
  float v13; // xmm5_4
  float v14; // xmm1_4
  float v17; // xmm2_4
  int i;
  float v19; // xmm1_4
  float v20; // xmm1_4
  float v22;
  float v25;
  float v26;
  int v27;
  float y0;
  D3DXMATRIX mat2;
  D3DXVECTOR3 vec;
  D3DXMATRIX mat;
 Height = this->Gepard->Terrain->GetHeight(this->Transformation.Translation.x, this->Transformation.Translation.z);
  v6 = this;
  y0 = (float)(Height);

  v27 = 0;
  Gepard = this->Gepard;
  y = Gepard->SunDir.y;
  z = Gepard->SunDir.z;
  v10 = Gepard->SunDir.x / y;
  *xmin = 10000.0;
  *xmax = -10000.0;
  *zmin = 10000.0;
  *zmax = -10000.0;
  v11 = z / y;
  if ( this->MeshArray.size > 0 )
  {
    v13 = y0 + 0.02f;
    v14 = v10 * y0;
    v17 = v11 * y0;
    v26 = v14;
    v22 = v13;
    v25 = v17;
    do
    {
      mat._11 = 1.0f; mat._12 = 0.0f;
      mat.m[0][2] = 0.0f; mat.m[0][3] = 0.0f;
      mat.m[1][0] = -v10; mat.m[1][1] = 0.0f;
      mat.m[1][2] = -v11; mat.m[1][3] = 0.0f;
      mat.m[2][0] = 0.0f; mat.m[2][1] = 0.0f;
      mat.m[2][2] = 1.0f; mat.m[2][3] = 0.0f;
      mat.m[3][0] = v14; mat.m[3][1] = v13;
      mat.m[3][2] = v17; mat.m[3][3] = 1.0f;
      // x64 fix: original IDA decomp walked MeshArray via byte-offset
      // arithmetic — `x = (int)&array->WorldMatrix + v15;` with v15 +=
      // sizeof(SMeshProp). On x64 the (int) cast truncated the high 32
      // bits of the heap pointer; D3DXMatrixMultiply read garbage and
      // ComputeShadowBounding returned bogus xmin/xmax/zmin/zmax. Use
      // typed array indexing instead.
      D3DXMatrixMultiply(&mat2, &v6->MeshArray.array[v27].WorldMatrix, &mat);
      for ( i = 0; i < 8; ++i )
      {
        D3DXVec3TransformCoord(&vec, &this->MeshArray.array[v27].Bound[i], &mat2);
        v19 = vec.x;
        if ( *xmin > vec.x )
          *xmin = vec.x;
        if ( v19 > *xmax )
          *xmax = v19;
        v20 = vec.z;
        if ( *zmin > vec.z )
          *zmin = vec.z;
        if ( v20 > *zmax )
          *zmax = v20;
      }
      v6 = this;
      v14 = v26;
      v17 = v25;
      v13 = v22;
      ++v27;
    }
    while ( v27 < this->MeshArray.size );
  }
}

//----- (00448930) --------------------------------------------------------

void SGroup::ComputeTransformedBounding(float *xmin, float *xmax, float *ymin, float *ymax, float *zmin, float *zmax, float ylimit, float ylimit0)

{
  SGroup *v9; // eax
  int v10;
  SMeshProp *array; // ecx
  unsigned int i;
  SMesh *ptr; // ecx
  float y; // xmm1_4
  float x; // xmm2_4
  float z; // xmm1_4
  int v17;
  unsigned int NumVertices;
  int v19;
  D3DXMATRIX mat;
  D3DXVECTOR3 vec;
  *xmin = 10000.0;
  *xmax = -10000.0;
  *ymin = 10000.0;
  *ymax = -10000.0;
  *zmin = 10000.0;
  *zmax = -10000.0;
  this->Precalculate();
  v9 = this;
  v17 = 0;
  if ( this->MeshArray.size > 0 )
  {
    v10 = 0;
    v19 = 0;
    do
    {
      array = v9->MeshArray.array;
      mat = *(D3DXMATRIX *)((char *)&array->WorldMatrix + v10);
      NumVertices = (*(SMesh **)((char *)&array->Mesh.ptr + v10))->GetNumVertices();
      (*(SMesh **)((char *)&this->MeshArray.array->Mesh.ptr + v10))->LockVertexBuffer();
      for ( i = 0; i < NumVertices; ++i )
      {
        ptr = this->MeshArray.array[v19].Mesh.ptr;
        D3DXVec3TransformCoord(&vec, (D3DXVECTOR3 *)((char *)ptr->lpVertices + ptr->OffsetXYZ + i * ptr->VertexSize), &mat);
        y = vec.y;
        if ( ylimit > vec.y && vec.y > ylimit0 )
        {
          x = vec.x;
          if ( *xmin > vec.x )
            *xmin = vec.x;
          if ( x > *xmax )
            *xmax = x;
          if ( *ymin > y )
            *ymin = y;
          if ( y > *ymax )
            *ymax = y;
          z = vec.z;
          if ( *zmin > vec.z )
            *zmin = vec.z;
          if ( z > *zmax )
            *zmax = z;
        }
      }
      this->MeshArray.array[v19].Mesh.ptr->UnlockVertexBuffer();
      v10 = (int)sizeof(SMeshProp) * (v19 + 1);
      v9 = this;
      ++v17;
      ++v19;
    }
    while ( v17 < this->MeshArray.size );
  }
}

//----- (00448AF0) --------------------------------------------------------

void SGroup::ComputeVisBounding(float *xmin, float *xmax, float *zmin, float *zmax)

{
  SGroup *v5; // eax
  int v6;
  int i;
  float x; // xmm1_4
  float z; // xmm1_4
  int v10;
  int v12;
  D3DXVECTOR3 vec;
  *xmin = 10000.0;
  *xmax = -10000.0;
  *zmin = 10000.0;
  *zmax = -10000.0;
  v5 = this;
  v10 = 0;
  if ( this->MeshArray.size > 0 )
  {
    v6 = 0;
    v12 = 0;
    do
    {
      for ( i = 0; i < 96; i += 12 )
      {
        D3DXVec3TransformCoord(
          &vec,
          (D3DXVECTOR3 *)((char *)v5->MeshArray.array->Bound + v6 + i),
          (D3DXMATRIX *)((char *)&v5->MeshArray.array->WorldMatrix + v6));
        x = vec.x;
        if ( *xmin > vec.x )
          *xmin = vec.x;
        if ( x > *xmax )
          *xmax = x;
        z = vec.z;
        if ( *zmin > vec.z )
          *zmin = vec.z;
        if ( z > *zmax )
          *zmax = z;
        v6 = v12;
        v5 = this;
      }
      // x64 fix: original `+= 320` assumed x86 sizeof(SMeshProp).
      v6 = v12 + (int)sizeof(SMeshProp);
      v12 += (int)sizeof(SMeshProp);
      ++v10;
    }
    while ( v10 < this->MeshArray.size );
  }
}

//----- (00448BF0) --------------------------------------------------------

void SGroup::Draw()

{
  SGepard *Gepard; // ecx
  SGepard *v3; // eax
  float b; // xmm0_4
  float r; // xmm2_4
  float g; // xmm1_4
  float FogStart; // xmm5_4
  float FogEnd; // xmm4_4
  float v9; // xmm5_4
  IDirect3DDevice9 *lpD3DDev; // eax
  int Selection;
  unsigned int Flags;
  int v13;
  int v14;
  SMeshProp *array; // eax
  SMeshProp *v16; // ecx
  float Interpolation; // xmm1_4
  int v18;
  unsigned int drawtype;
  SLightCB lcb;
  if ( this->Visible )
  {
    Gepard = this->Gepard;
    if ( this->ambientMode )
    {
      Gepard->SetLightingType(LT_AMBIENT);
      this->Gepard->SetAmbientColor((int)(float)(this->ambientColor.b * 255.0) | (((int)(float)(this->ambientColor.g * 255.0) | ((int)(float)(this->ambientColor.r * 255.0) << 8)) << 8));
      v3 = this->Gepard;
      b = this->ambientColor.b;
      r = this->ambientColor.r;
      g = this->ambientColor.g;
      lcb.Vector4fCount = 5;
      FogStart = v3->FogStart;
      FogEnd = v3->FogEnd;
      lcb.Ambient[3] = 1.0;
      lcb.SunlightDir[3] = 0.0;
      lcb.SunlightColor[3] = 1.0;
      lcb.Ambient[2] = b;
      lcb.BottomAmbient[2] = b;
      lcb.BottomAmbient[3] = 1.0f;   // alpha/padding (standard value)
      lcb.SunlightDir[0] = 0.0f;     // XMM constant {1.0, 0.0, -1.0, 0.0}
      lcb.SunlightDir[1] = -1.0f;    // = straight-down direction
      lcb.SunlightDir[2] = 0.0f;
      lcb.Ambient[0] = r;
      lcb.Ambient[1] = g;
      lcb.BottomAmbient[0] = r;
      lcb.BottomAmbient[1] = g;
      lcb.SunlightColor[0] = 0.0;
      lcb.SunlightColor[1] = 0.0; // _mm_shuffle_ps of zero = zero
      lcb.SunlightColor[2] = 0.0;
      if ( FogEnd <= FogStart )
      {
        lcb.FogParams[0] = 0.0;
        lcb.FogParams[1] = 0.0;
      }
      else
      {
        v9 = FogStart - FogEnd;
        lcb.FogParams[0] = 1.0f / v9;
        lcb.FogParams[1] = -(FogEnd / v9);
      }
      lpD3DDev = this->lpD3DDev;
      lcb.FogParams[2] = 0.0;
      lcb.FogParams[3] = 0.0;
      lpD3DDev->SetVertexShaderConstantF(16u, lcb.Ambient, 5u);
    }
    else
    {
      Gepard->SetLightingType(LT_NORMAL);
    }
    this->Gepard->SetDrawType(this->DrawType);
    if ( this->DrawType == DT_ADD )
      this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
    Selection = this->Selection;
    if ( Selection )
    {
      switch ( Selection )
      {
        case 3:
          this->Gepard->SelectionFog(0x40FFFFC0u);
          break;
        case 2:
          // Hover white tint -- depth-independent so it's visible on near
          // and far doodads alike. Original was SelectionFog(0x20FFFFFFu),
          // which produced a ~12% white wash at typical editor camera
          // distances (oFog = length(viewPos) * -0.000392 + 0.875 ≈ 0.87
          // at distance 17). That gradient made the picker firing across
          // the rect look like nothing was happening at the hovered model.
          this->Gepard->SelectionFogFlat(0xFFFFFFFFu, 0.45f);
          break;
        case 1:
          this->Gepard->SelectionFog(0x40FFFF00u);
          break;
      }
    }
    if ( this->IsColorized )
      this->lpD3DDev->SetPixelShaderConstantF(0, (const float *)&this->ColorizeConstants, 2u);
    Flags = this->Flags;
    v13 = 2;
    drawtype = 2;
    if ( (Flags & 8) != 0 )
    {
      this->lpD3DDev->SetVertexShaderConstantF(22u, this->treeBendParams, 1u);
      Flags = this->Flags;
      v13 = 34;
      drawtype = 34;
    }
    if ( (Flags & 0x10) != 0 )
    {
      this->lpD3DDev->SetVertexShaderConstantF(23u, this->treeWobbleParams, 1u);
      drawtype = v13 | 0x40;
    }
    v18 = 0;
    if ( this->MeshArray.size > 0 )
    {
      v14 = 0;
      do
      {
        this->lpD3DDev->SetTransform(
          (_D3DTRANSFORMSTATETYPE)256,
          &this->MeshArray.array[v14].WorldMatrix);
        array = this->MeshArray.array;
        if ( array[v14].Visible )
        {
          this->Gepard->SetWorldViewProjVertexShaderConstantBuffer(&array[v14].WorldMatrix);
          v16 = this->MeshArray.array;
          if ( v16[v14].HasUVTranslate )
          {
            Interpolation = (float)(this->Gepard->Interpolation);

            v16[v14].Mesh.ptr->Draw(drawtype, 0, (float)((float)(1.0 - Interpolation) * v16[v14].UVTranslate)
            + (float)(v16[v14].LastUVTranslate * Interpolation));
          }
          else
          {
            v16[v14].Mesh.ptr->Draw(drawtype, 0, 0.0);
          }
        }
        ++v14;
        ++v18;
      }
      while ( v18 < this->MeshArray.size );
    }
    if ( this->ambientMode )
      this->Gepard->SetLightCB();
    if ( this->Selection || this->DrawType == DT_ADD )
      this->Gepard->EnableFog();
  }
}

//----- (00448F20) --------------------------------------------------------

void SGroup::DrawShadow2()

{
  unsigned int Flags;
  int v3;
  int v4;
  int v5;
  SMeshProp *array; // ecx
  unsigned int drawtype;
  if ( this->DrawType != DT_ADD )
  {
    Flags = this->Flags;
    v3 = 16;
    drawtype = 16;
    if ( (Flags & 8) != 0 )
    {
      this->lpD3DDev->SetVertexShaderConstantF(22u, this->treeBendParams, 1u);
      Flags = this->Flags;
      v3 = 48;
      drawtype = 48;
    }
    if ( (Flags & 0x10) != 0 )
    {
      this->lpD3DDev->SetVertexShaderConstantF(23u, this->treeWobbleParams, 1u);
      drawtype = v3 | 0x40;
    }
    v4 = 0;
    if ( this->MeshArray.size > 0 )
    {
      v5 = 0;
      do
      {
        this->Gepard->SetWorldViewProjVertexShaderConstantBuffer(&this->MeshArray.array[v5].WorldMatrix);
        array = this->MeshArray.array;
        if ( array[v5].Visible && array[v5].CanCastShadows )
          array[v5].Mesh.ptr->Draw(drawtype, 0, 0.0);
        ++v4;
        ++v5;
      }
      while ( v4 < this->MeshArray.size );
    }
  }
}

//----- (00448FF0) --------------------------------------------------------

void SGroup::DrawShadow()

{
  int v2;
  int v3;
  SMeshProp *array; // ecx
  v2 = 0;
  if ( this->MeshArray.size > 0 )
  {
    v3 = 0;
    do
    {
      this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &this->MeshArray.array[v3].WorldMatrix);
      array = this->MeshArray.array;
      if ( array[v3].Visible && array[v3].CanCastShadows )
        array[v3].Mesh.ptr->Draw(8u, 0, 0.0);
      ++v2;
      ++v3;
    }
    while ( v2 < this->MeshArray.size );
  }
}

//----- (00449060) --------------------------------------------------------

void SGroup::DrawShadowHack2()

{
  double Height; // st7
  SGepard *Gepard; // eax
  int v4;
  float y; // xmm0_4
  float v6; // xmm2_4
  float v7; // xmm3_4
  float v8; // xmm4_4
  float v9; // xmm5_4
  SGroup *v10; // eax
  float v11; // xmm2_4
  int v13;
  float v14; // xmm3_4
  SMeshProp *array; // ecx
  D3DXMATRIX *x;
  float v22;
  float y0;
  float y0a;
  D3DXMATRIX mat2;
  D3DXMATRIX mat;
 Height = this->Gepard->Terrain->GetHeight(this->Transformation.Translation.x, this->Transformation.Translation.z);
  Gepard = this->Gepard;
  v4 = 0;
  y0 = (float)(Height);

  y = Gepard->SunDir.y;
  v6 = Gepard->SunDir.x / y;
  v7 = Gepard->SunDir.z / y;
  if ( this->MeshArray.size > 0 )
  {
    v8 = Gepard->SunDir.x / y;
    v9 = Gepard->SunDir.z / y;
    v10 = this;
    v11 = v6 * y0;
    v13 = 0;
    v14 = v7 * y0;
    y0a = v11;
    v22 = v14;
    do
    {
      x = &v10->MeshArray.array[v13].WorldMatrix;
      mat._11 = 1.0f; mat._12 = 0.0f;
      mat.m[0][2] = 0.0f; mat.m[0][3] = 0.0f;
      mat.m[1][0] = -v8; mat.m[1][1] = 1.0f;
      mat.m[1][2] = -v9; mat.m[1][3] = 0.0f;
      mat.m[2][0] = 0.0f; mat.m[2][1] = 0.0f;
      mat.m[2][2] = 1.0f; mat.m[2][3] = 0.0f;
      mat.m[3][0] = v11; mat.m[3][1] = -y0;
      mat.m[3][2] = v14; mat.m[3][3] = 1.0f;
      D3DXMatrixMultiply(&mat2, x, &mat);
      this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &mat2);
      v10 = this;
      array = this->MeshArray.array;
      if ( array[v13].Visible && array[v13].CanCastShadows )
      {
        array[v13].Mesh.ptr->Draw(8u, 0, 0.0);
        v10 = this;
      }
      v11 = y0a;
      ++v4;
      v14 = v22;
      ++v13;
    }
    while ( v4 < v10->MeshArray.size );
  }
}

//----- (00449260) --------------------------------------------------------

void SGroup::DrawShadowHack()

{
  double Height; // st7
  SGepard *Gepard; // eax
  int v4;
  float y; // xmm0_4
  float v6; // xmm1_4
  float v7; // xmm2_4
  float v8; // xmm3_4
  int v11;
  SGroup *v12; // eax
  float v13; // xmm1_4
  float v14; // xmm2_4
  SMeshProp *array; // ecx
  D3DXMATRIX *x;
  float v19;
  float v20;
  float y0;
  D3DXMATRIX mat2;
  D3DXMATRIX mat;
 Height = this->Gepard->Terrain->GetHeight(this->Transformation.Translation.x, this->Transformation.Translation.z);
  Gepard = this->Gepard;
  v4 = 0;
  y0 = (float)(Height);

  y = Gepard->SunDir.y;
  v6 = Gepard->SunDir.x / y;
  v7 = Gepard->SunDir.z / y;
  if ( this->MeshArray.size > 0 )
  {
    v8 = (float)(Height);

    v11 = 0;
    v12 = this;
    v13 = v6 * y0;
    v14 = v7 * y0;
    v20 = v13;
    v19 = v14;
    do
    {
      x = &v12->MeshArray.array[v11].WorldMatrix;
      mat._11 = 1.0f; mat._12 = 0.0f;
      mat.m[0][2] = 0.0f; mat.m[0][3] = 0.0f;
      mat.m[1][0] = -v6; mat.m[1][1] = 0.0f;
      mat.m[1][2] = -v7; mat.m[1][3] = 0.0f;
      mat.m[2][0] = 0.0f; mat.m[2][1] = 0.0f;
      mat.m[2][2] = 1.0f; mat.m[2][3] = 0.0f;
      mat.m[3][0] = v13; mat.m[3][1] = v8;
      mat.m[3][2] = v14; mat.m[3][3] = 1.0f;
      D3DXMatrixMultiply(&mat2, x, &mat);
      this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &mat2);
      v12 = this;
      array = this->MeshArray.array;
      if ( array[v11].Visible && array[v11].CanCastShadows )
      {
        array[v11].Mesh.ptr->Draw(0, 0, 0.0);
        v12 = this;
      }
      v13 = v20;
      ++v4;
      v14 = v19;
      ++v11;
      v8 = (float)(Height);

    }
    while ( v4 < v12->MeshArray.size );
  }
}

//----- (00449450) --------------------------------------------------------

void SGroup::ForceUpdate()

{
  this->LastRecalculated = -1;
}

//----- (00449460) --------------------------------------------------------

bool SGroup::GetActivationState()

{
  return this->Activated;
}

//----- (00449470) --------------------------------------------------------

float SGroup::GetClickDistanceSquare(float click_x, float click_y)

{
  STransformation *p_Transformation; // edi
  SGepard *Gepard; // edi
  SGepard *v7; // ebx
  float z_4;
  STransformation trans;
  p_Transformation = &this->Transformation;
  if ( (this->Flags & 2) != 0 )
  {
    z_4 = (float)(this->Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_Transformation->Translation.x) * z_4)
                        + p_Transformation->Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                * z_4)
                        + this->Transformation.Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                * z_4)
                        + this->Transformation.Translation.z;
    trans.Scale = (float)((float)(this->LastTransformation.Scale - this->Transformation.Scale) * z_4)
                + this->Transformation.Scale;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &this->Transformation.Rotation,
      &this->LastTransformation.Rotation,
      z_4);
    Gepard = this->Gepard;
    if ( Gepard->Terrain->IsVisible(trans.Translation.x, trans.Translation.z, this->VisClass) )
      return Gepard->GetClickDistanceSquare(&trans.Translation, click_x, click_y);
  }
  else
  {
    v7 = this->Gepard;
 if ( v7->Terrain->IsVisible(p_Transformation->Translation.x, this->Transformation.Translation.z, this->VisClass) )
    {
      return v7->GetClickDistanceSquare(&p_Transformation->Translation, click_x, click_y);
    }
  }
  return 3.4028235e38f;
}

//----- (00449600) --------------------------------------------------------

void SGroup::GetInternalIndex(bool *dynamic, int *gepard_index)

{
  *dynamic = this->Type == 1;
  *gepard_index = this->GepardIndex;
}

//----- (00449620) --------------------------------------------------------

void SGroup::GetInterpolatedPosition(float *x, float *y, float *z)

{
  float Interpolation;
  STransformation trans;
  if ( (this->Flags & 2) != 0 )
  {
    Interpolation = (float)(this->Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - this->Transformation.Translation.x)
                                * Interpolation)
                        + this->Transformation.Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                * Interpolation)
                        + this->Transformation.Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                * Interpolation)
                        + this->Transformation.Translation.z;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &this->Transformation.Rotation,
      &this->LastTransformation.Rotation,
      Interpolation);
    *x = trans.Translation.x;
    *y = trans.Translation.y;
    *z = trans.Translation.z;
  }
  else
  {
    *x = this->Transformation.Translation.x;
    *y = this->Transformation.Translation.y;
    *z = this->Transformation.Translation.z;
  }
}

//----- (00449720) --------------------------------------------------------

SMatrix4 *STransformation::GetMatrix(SMatrix4 *result)

{
  D3DXQUATERNION Rotation; // xmm1
  float v3; // xmm7_4
  float v4; // xmm4_4
  SMatrix4 *v5; // eax
  float v6; // xmm6_4
  float v7; // xmm5_4
  float v8; // xmm6_4
  float v9; // xmm4_4
  float v10; // xmm3_4
  float v11; // xmm0_4
  D3DXQUATERNION v12; // xmm2
  float v13; // xmm6_4
  float v14; // xmm0_4
  float v15; // xmm0_4
  float v16; // xmm5_4
  float z;
  float v19;
  float v20;
  Rotation = this->Rotation;
  float _tx = this->Translation.x, _ty = this->Translation.y;
  v3 = Rotation.z; // _mm_shuffle_ps mask 170 = z component
  v4 = Rotation.x + Rotation.x;
  z = this->Translation.z;
  v5 = result;
  result->m03 = 0.0;
  v6 = Rotation.w; // _mm_shuffle_ps mask 255 = w component
  v19 = Rotation.y; // _mm_shuffle_ps mask 85 = y component
  v7 = (float)(v3 * v3) + (float)(v3 * v3);
  Rotation.x = (float)(Rotation.x + Rotation.x) * v19;
  v8 = v6 + v6;
  v9 = v4 * v3;
  v20 = (float)(v19 * v19) + (float)(v19 * v19);
  v10 = v8 * v19;
  result->m00 = (float)((float)(1.0 - v20) - v7) * this->Scale;
  v11 = Rotation.x + (float)(v8 * v3);
  v12 = this->Rotation;
  Rotation.x = (float)(Rotation.x - (float)(v8 * v3)) * this->Scale;
  result->m01 = v11 * this->Scale;
  v13 = v8 * v12.x;
  result->m10 = Rotation.x;
  result->m02 = (float)(v9 - v10) * this->Scale;
  v14 = v12.x * v12.x;
  v12.x = this->Scale;
  result->m20 = (float)(v10 + v9) * v12.x;
  Rotation.x = 1.0f - (float)(v14 + v14);
  v15 = Rotation.x - v7;
  v16 = (float)(v19 + v19) * v3;
  result->m11 = v15 * this->Scale;
  result->m22 = (float)(Rotation.x - v20) * v12.x;
  result->m12 = (float)(v13 + v16) * v12.x;
  result->m[3][0] = _tx; result->m[3][1] = _ty;
  result->m21 = (float)(v16 - v13) * v12.x;
  result->m32 = z;
  result->m13 = 0.0;
  result->m23 = 0.0;
  result->m33 = 1.0;
  return v5;
}

//----- (004498B0) --------------------------------------------------------

void STransformation::GetMatrix(D3DXMATRIX *matrix)

{
  SMatrix4 result;
  *matrix = *(D3DXMATRIX *)this->GetMatrix(&result);
}

//----- (004498F0) --------------------------------------------------------

int SGroup::GetMeshIndex2(char *name)

{
  int v3;
  int i;
  char *buf; // ecx
  v3 = 0;
  if ( this->MeshArray.size <= 0 )
    return -1;
  for ( i = 0; ; ++i )
  {
    buf = (char *)fullpath;
    if ( this->MeshArray.array[i].MeshName.buf )
      buf = this->MeshArray.array[i].MeshName.buf;
    if ( strstr(buf, name) )
      break;
    if ( ++v3 >= this->MeshArray.size )
      return -1;
  }
  return v3;
}

//----- (00449940) --------------------------------------------------------

int SGroup::GetMeshIndex(char *name)

{
  int v3;
  int i;
  char *buf; // ecx
  v3 = 0;
  if ( this->MeshArray.size <= 0 )
    return -1;
  for ( i = 0; ; ++i )
  {
    buf = (char *)fullpath;
    if ( this->MeshArray.array[i].MeshName.buf )
      buf = this->MeshArray.array[i].MeshName.buf;
    if ( !_stricmp(buf, name) )
      break;
    if ( ++v3 >= this->MeshArray.size )
      return -1;
  }
  return v3;
}

//----- (004499A0) --------------------------------------------------------

void SGroup::GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z, float *head_x, float *head_y, float *head_z)

{
  D3DXMATRIX fmat;
  this->GetMeshInterpolatedProperties(idx, &fmat);
  *x = fmat._41;
  *y = fmat._42;
  *z = fmat._43;
  *head_x = fmat._21;
  *head_y = fmat._22;
  *head_z = fmat._23;
}

//----- (00449A30) --------------------------------------------------------

void SGroup::GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z)

{
  D3DXMATRIX fmat;
  this->GetMeshInterpolatedProperties(idx, &fmat);
  *x = fmat._41;
  *y = fmat._42;
  *z = fmat._43;
}

//----- (00449A90) --------------------------------------------------------

void SGroup::GetMeshInterpolatedProperties(int idx, D3DXMATRIX *matrix)

{
  bool v4;
  SMeshProp *array; // edx
  float Interpolation; // xmm2_4
  SMatrix4 *v7; // eax
  SMeshProp *v8; // ecx
  int Parent;
  STransformation *p_Transformation; // ecx
  float v11;
  SMatrix4 result;
  int v13;
  D3DXMATRIX mat;
  D3DXMATRIX dynamic;
  D3DXMATRIX world;
  STransformation trans;
  if ( idx < 0 || idx >= this->MeshArray.size )
  {
    D3DXMatrixIdentity(matrix);
  }
  else
  {
    v4 = (this->Flags & 4) == 0;
    v13 = (int)sizeof(SMeshProp) * idx;
    if ( v4 )
    {
      v7 = this->MeshArray.array[idx].Transformation.GetMatrix((SMatrix4 *)&world);
    }
    else
    {
      array = this->MeshArray.array;
      Interpolation = (float)(this->Gepard->Interpolation);

      trans.Translation.x = (float)((float)(array[v13 / (int)sizeof(SMeshProp)].LastTransformation.Translation.x
                                          - array[idx].Transformation.Translation.x)
                                  * Interpolation)
                          + array[idx].Transformation.Translation.x;
      trans.Translation.y = (float)((float)(array[v13 / (int)sizeof(SMeshProp)].LastTransformation.Translation.y
                                          - array[idx].Transformation.Translation.y)
                                  * Interpolation)
                          + array[idx].Transformation.Translation.y;
      trans.Translation.z = (float)((float)(array[v13 / (int)sizeof(SMeshProp)].LastTransformation.Translation.z
                                          - array[idx].Transformation.Translation.z)
                                  * Interpolation)
                          + array[idx].Transformation.Translation.z;
      trans.Scale = (float)((float)(array[v13 / (int)sizeof(SMeshProp)].LastTransformation.Scale - array[idx].Transformation.Scale)
                          * Interpolation)
                  + array[idx].Transformation.Scale;
      D3DXQuaternionSlerp(
        &trans.Rotation,
        &array[idx].Transformation.Rotation,
        &array[v13 / (int)sizeof(SMeshProp)].LastTransformation.Rotation,
        Interpolation);
      v7 = trans.GetMatrix((SMatrix4 *)&world);
    }
    v8 = this->MeshArray.array;
    dynamic = *(D3DXMATRIX *)v7;
    D3DXMatrixMultiply(&mat, &dynamic, &v8[v13 / (int)sizeof(SMeshProp)].OriginMatrix);
    Parent = this->MeshArray.array[idx].Parent;
    if ( Parent < 0 )
    {
      p_Transformation = &this->Transformation;
      if ( (this->Flags & 2) != 0 )
      {
        v11 = (float)(this->Gepard->Interpolation);

        trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_Transformation->Translation.x)
                                    * v11)
                            + p_Transformation->Translation.x;
        trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                    * v11)
                            + this->Transformation.Translation.y;
        trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                    * v11)
                            + this->Transformation.Translation.z;
        trans.Scale = (float)((float)(this->LastTransformation.Scale - this->Transformation.Scale) * v11)
                    + this->Transformation.Scale;
        D3DXQuaternionSlerp(
          &trans.Rotation,
          &this->Transformation.Rotation,
          &this->LastTransformation.Rotation,
          v11);
        p_Transformation = &trans;
      }
      world = *(D3DXMATRIX *)p_Transformation->GetMatrix(&result);
      D3DXMatrixMultiply(matrix, &mat, &world);
    }
    else
    {
      this->GetMeshProperties(Parent, &world);
      D3DXMatrixMultiply(matrix, &mat, &world);
    }
  }
}

//----- (00449DC0) --------------------------------------------------------

const char *SGroup::GetMeshName(int index)

{
  char *buf; // ecx
  char *result; // eax
  buf = this->MeshArray.array[index].MeshName.buf;
  result = (char *)fullpath;
  if ( buf )
    return buf;
  return result;
}

//----- (00449DE0) --------------------------------------------------------

SDArray<SString> *SGroup::GetMeshNames(SDArray<SString> *result)

{
  int v2;
  const char **v3; // ebx
  int maxsize;
  int v5;
  SString *v6; // eax
  int v7;
  int v8;
  const char *v9; // eax
  bool v10;
  const char *v11; // ebx
  SString *v12; // edi
  unsigned int v13;
  char *v14; // ecx
  size_t v15;
  SGroup *v17;
  int v18;
  int v19;
  v17 = this;
  result->array = 0;
  result->size = 0;
  result->maxsize = 0;
  v19 = 0;
  if ( this->MeshArray.size > 0 )
  {
    v2 = 0;
    v18 = 0;
    do
    {
      v3 = (const char **)((char *)&this->MeshArray.array->MeshName.buf + v2);
      maxsize = result->maxsize;
      if ( result->size == maxsize )
      {
        if ( maxsize >= 16 )
          v5 = 6 * maxsize / 5;
        else
          v5 = 16;
        // x64: literal `8` is x86 sizeof(SString); on x64 buf:char* + size:int +
        // padding = 16, so the original allocation was half-sized and array[N]
        // walked into 0xFDFDFDFD no-man's-land — operator delete crashed when the
        // garbage was read as v12->buf.
        v6 = (SString *)realloc(result->array, sizeof(SString) * v5);
        v7 = result->maxsize;
        result->array = v6;
        memset(&v6[v7], 0, sizeof(SString) * (v5 - v7));
        result->maxsize = v5;
      }
      v8 = result->size++;
      v9 = *v3;
      v10 = *v3 == 0;
      v11 = fullpath;
      if ( !v10 )
        v11 = v9;
      v12 = &result->array[v8];
      if ( v12->buf )
      {
        operator delete(v12->buf);
        v12->buf = 0;
      }
      if ( v11 )
      {
        v13 = strlen(v11);
        v12->size = v13;
        v14 = new char[v13 + 1];
        v15 = v12->size + 1;
        v12->buf = v14;
        memcpy(v14, v11, v15);
      }
      else
      {
        v12->size = 0;
        v12->buf = 0;
      }
      // x64 fix: original `+= 320` assumed x86 sizeof(SMeshProp).
      v2 = v18 + (int)sizeof(SMeshProp);
      ++v19;
      // this = v17; // IDA artifact
      v18 += (int)sizeof(SMeshProp);
    }
    while ( v19 < v17->MeshArray.size );
  }
  return result;
}

//----- (00449F60) --------------------------------------------------------

SMatrix4 *SGroup::GetMeshProperties(SMatrix4 *result, int idx)

{
  SMeshProp *array; // esi
  SMatrix4 *Matrix; // eax
  const SMatrix4 *MeshProperties; // eax
  SMatrix4 *v7; // eax
  SMatrix4 v8;
  SMatrix4 mat;
  if ( idx < 0 || idx >= this->MeshArray.size )
  {
    v7 = result;
    memset(result, 0, sizeof(SMatrix4));
    result->m00 = 1.0f;
    result->m[1][1] = 1.0f;
    result->m[2][2] = 1.0f;
    result->m33 = 1.0f;
  }
  else
  {
    array = this->MeshArray.array;
    Matrix = array[idx].Transformation.GetMatrix(&v8);
    mat = *Matrix * *(const SMatrix4 *)&array[idx].OriginMatrix;
    if ( array[idx].Parent < 0 )
      MeshProperties = this->Transformation.GetMatrix(&v8);
    else
      MeshProperties = this->GetMeshProperties(&v8, array[idx].Parent);
    *result = mat * *MeshProperties;
    return result;
  }
  return v7;
}

//----- (0044A060) --------------------------------------------------------

void SGroup::GetMeshProperties(int idx, float *x, float *y, float *z, float *head_x, float *head_y, float *head_z)

{
  D3DXMATRIX fmat;
  this->GetMeshProperties(idx, &fmat);
  *x = fmat._41;
  *y = fmat._42;
  *z = fmat._43;
  *head_x = fmat._21;
  *head_y = fmat._22;
  *head_z = fmat._23;
}

//----- (0044A0F0) --------------------------------------------------------

void SGroup::GetMeshProperties(int idx, float *x, float *y, float *z)

{
  D3DXMATRIX fmat;
  this->GetMeshProperties(idx, &fmat);
  *x = fmat._41;
  *y = fmat._42;
  *z = fmat._43;
}

//----- (0044A150) --------------------------------------------------------

void SGroup::GetMeshProperties(int idx, D3DXMATRIX *matrix)

{
  SMatrix4 result;
  *matrix = *(D3DXMATRIX *)this->GetMeshProperties(&result, idx);
}

//----- (0044A190) --------------------------------------------------------

void SGroup::GetPosition(float *x, float *y, float *z)

{
  *x = this->Transformation.Translation.x;
  *y = this->Transformation.Translation.y;
  *z = this->Transformation.Translation.z;
}

//----- (0044A1B0) --------------------------------------------------------

void SGroup::GetScreenPos(float *x, float *y, float *z)

{
  SGepard *Gepard; // eax
  float Interpolation; // xmm2_4
  bool v7;
  float *v8; // eax
  STransformation trans;
  Gepard = this->Gepard;
  if ( (this->Flags & 2) != 0 )
  {
    Interpolation = (float)(Gepard->Interpolation);

    trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - this->Transformation.Translation.x)
                                * Interpolation)
                        + this->Transformation.Translation.x;
    trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y - this->Transformation.Translation.y)
                                * Interpolation)
                        + this->Transformation.Translation.y;
    trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z - this->Transformation.Translation.z)
                                * Interpolation)
                        + this->Transformation.Translation.z;
    D3DXQuaternionSlerp(
      &trans.Rotation,
      &this->Transformation.Rotation,
      &this->LastTransformation.Rotation,
      Interpolation);
    v7 = this->Gepard->Terrain->IsVisible(trans.Translation.x, trans.Translation.z, this->VisClass) == 0;
    v8 = z;
    if ( !v7 )
    {
      this->Gepard->TransformPoint(trans.Translation.x, trans.Translation.y, trans.Translation.z, x, y, z);
      return;
    }
  }
  else
  {
 v7 = Gepard->Terrain->IsVisible(this->Transformation.Translation.x, this->Transformation.Translation.z, this->VisClass) == 0;
    v8 = z;
    if ( !v7 )
    {
      this->Gepard->TransformPoint(this->Transformation.Translation.x, this->Transformation.Translation.y, this->Transformation.Translation.z, x, y, z);
      return;
    }
  }
  *v8 = -1.0;
  *y = -1.0;
  *x = -1.0;
}

//----- (0044A380) --------------------------------------------------------

void STransformation::Identity()

{
  this->Translation.x = 0.0;
  this->Translation.y = 0.0;
  this->Translation.z = 0.0;
  this->Scale = 1.0;
  this->Rotation.z = 0.0;
  this->Rotation.y = 0.0;
  this->Rotation.x = 0.0;
  this->Rotation.w = 1.0;
}

//----- (0044A3C0) --------------------------------------------------------

char SGroup::Load4DFile(const char *textureprefix, const char *filename, float scale)

{
  SGroup *v4; // ebx
  SStream *v5; // eax
  SStream *v6; // esi
  SMeshMaterial *MaterialBuffer; // edi
  int v9;
  bool v10;
  int Word;
  SMeshMaterial *v12; // eax
  void *v13; // eax
  void *v14; // eax
  SMeshMaterial *v15; // ecx
  const char *buf; // ecx
  const char *v17; // ecx
  SMeshProp *v18; // edx
  SGroup *v19; // ecx
  int Int;
  int v21;
  SMeshProp *v22; // eax
  int v23;
  int v24;
  SMeshProp *v26; // eax
  int v27;
  int ChunkHeader;
  unsigned int v29;
  int v30;
  int v31;
  int v32;
  char *v34; // eax
  const char *v35; // edx
  _DWORD *v37; // ecx
  size_t v38;
  char *v39; // edi
  const char *v40; // edx
  char *v41; // esi
  int v42;
  SStream *v43; // ecx
  int v44;
  void *v45; // edi
  size_t v46;
  size_t v47;
  char *v48; // ebx
  const char *v49; // edx
  char *v50; // esi
  int v51;
  int v52;
  unsigned int v53;
  SMesh *v54; // eax
  unsigned int v55;
  unsigned int i;
  int v61;
  char *v62; // ecx
  int OffsetNormal;
  int v65;
  char *v66;
  void *v67;
  SString v68;
  char *v69;
  size_t v70;
  SString v71;
  SString result;
  int v74;
  void *v82;
  int v83;
  unsigned int v84;
  int v85;
  void *block;
  SStream *is;
  int v88;
  void *IndexBuffer;
  int v90;
  int v91;
  SMesh *v92;
  SMeshMaterial *array;
  SGroup *v94;
  int *v95;
  int v96;
  v95 = &v65;
  v4 = this;
  v94 = this;
  v88 = 0;
  v5 = FileSystem.OpenRead(filename, 0);
  v6 = v5;
  is = v5;
  if ( v5 )
  {
    v4->Transformation.Scale = scale;
    v96 = 0;
    v5->ReadSignature();
    if ( v6->ReadChunkHeader() != 1313162067 )
    {
      throw "Not a scene file";
    }
    if ( v6->ReadInt() != 808464758 )
    {
      throw "Unsupported 4D Scene file version (%s)";
    }
    while ( !v6->ReadChunkIsEnd() )
    {
      MaterialBuffer = 0;
      if ( v6->ReadChunkHeader() == 1213416781 )
      {
        v85 = v4->MeshArray.Add();
        v92 = 0;
        v9 = v85;
        v10 = !v6->NewTypeStrings;
        // Load the mesh-name SString (MeshName.buf / MeshName.size). IDA expressed
        // this via a mislabelled (SMeshMaterial*) cast + v9*320 byte math, which
        // breaks on x64 because SString is wider and SMeshProp is larger.
        SMeshProp *mp = &v94->MeshArray.array[v9];
        array = (SMeshMaterial *)v94->MeshArray.array;  // kept only for the v12/v15 aliasing below
        if ( v10 )
          Word = v6->ReadWord() - 1;
        else
          Word = v6->ReadWord();
        v12 = array;
        mp->MeshName.size = Word;
        v13 = mp->MeshName.buf;
        if ( v13 )
        {
          delete[] v13;
          Word = mp->MeshName.size;
          mp->MeshName.buf = nullptr;
        }
        if ( Word >= 0 )
        {
          v14 = operator new[](Word + 1);
          v15 = array;
          mp->MeshName.buf = (char *)v14;
          if ( !v14 )
          {
            throw "Out of memory";
          }
          v6->Read(v14, mp->MeshName.size);
          mp->MeshName.buf[mp->MeshName.size] = 0;
        }
        buf = fullpath;
        if ( v94->MeshArray.array[v9].MeshName.buf )
          buf = v94->MeshArray.array[v9].MeshName.buf;
        Logger.g->Log(2, "                    Mesh name: %s", buf);
        v17 = fullpath;
        v18 = v94->MeshArray.array;
        if ( v18[v9].MeshName.buf )
          v17 = v18[v9].MeshName.buf;
        v10 = *v17 == 95;
        v19 = v94;
        v18[v9].Visible = !v10;
        v19->MeshArray.array[v9].CanCastShadows = 1;
        // x64 fix: original `v91 = (int)&array[v9]; *(_DWORD*)(v91+8) = Int`
        // truncated the SMeshProp pointer to 32 bits and used x86 offset 8 for
        // Parent. On x64 Parent is at +16 (SString grew by 8). Use typed access.
        Int = v6->ReadInt();
        v94->MeshArray.array[v9].Parent = Int;
        Logger.g->Log(2, "                      Parent num: %d", v94->MeshArray.array[v9].Parent);
        Logger.g->Log(2, "                      Local transformation matrix");
        v21 = v85;
        v22 = v94->MeshArray.array;
        v22[v9].OriginMatrix._43 = 0.0;
        v22[v9].OriginMatrix._42 = 0.0;
        v22[v9].OriginMatrix._41 = 0.0;
        v22[v9].OriginMatrix._34 = 0.0;
        v22[v9].OriginMatrix._32 = 0.0;
        v22[v9].OriginMatrix._31 = 0.0;
        v22[v9].OriginMatrix._24 = 0.0;
        v22[v9].OriginMatrix._23 = 0.0;
        v22[v9].OriginMatrix._21 = 0.0;
        v22[v9].OriginMatrix._14 = 0.0;
        v22[v9].OriginMatrix._13 = 0.0;
        v22[v9].OriginMatrix._12 = 0.0;
        v22[v9].OriginMatrix._44 = 1.0;
        v22[v9].OriginMatrix._33 = 1.0;
        v22[v9].OriginMatrix._22 = 1.0;
        v22[v9].OriginMatrix._11 = 1.0;
        // x64 fix: original `v24 = 20 * v21` is x86 sizeof(SMeshProp)/sizeof(int)
        // (= 320 / 16 row stride). On x64 sizeof(SMeshProp) is bigger.
        // Use typed indexing into OriginMatrix rows.
        for (v23 = 0; v23 < 4; ++v23)
        {
          v6->Read(&v94->MeshArray.array[v21].OriginMatrix.m[v23][0], 12);
          Logger.g->Log(
            2,
            "                         %f, %f, %f, %f",
            v94->MeshArray.array[v21].OriginMatrix.m[v23][0],
            v94->MeshArray.array[v21].OriginMatrix.m[v23][1],
            v94->MeshArray.array[v21].OriginMatrix.m[v23][2],
            v94->MeshArray.array[v21].OriginMatrix.m[v23][3]);
        }
        (void)v24; (void)v90; (void)v91;
        v4 = v94;
        v26 = v94->MeshArray.array;
        v27 = v85;
        v74 = v27 * (int)sizeof(SMeshProp);
        v26[v27].Transformation.Translation.x = 0.0;
        v26[v27].Transformation.Translation.y = 0.0;
        v26[v27].Transformation.Translation.z = 0.0;
        v26[v27].Transformation.Scale = 1.0;
        v26[v27].Transformation.Rotation.z = 0.0;
        v26[v27].Transformation.Rotation.y = 0.0;
        v26[v27].Transformation.Rotation.x = 0.0;
        v26[v27].Transformation.Rotation.w = 1.0;
        while ( !v6->ReadChunkIsEnd() )
        {
          ChunkHeader = v6->ReadChunkHeader();
          if ( ChunkHeader > 1414677846 )
          {
            if ( ChunkHeader != 1481589314 )
            {
LABEL_100:
              throw "Unsupported mesh component";
            }
            v6->Read((char *)v4->MeshArray.array->Bound + v74, 96);
            v6->ReadChunkValidate(0);
          }
          else if ( ChunkHeader == 1414677846 )
          {
            v53 = v6->ReadInt();
            v91 = v6->ReadInt();
            // sizeof(SMesh) — IDA had hardcoded 0x6Cu (108 bytes) which
            // matched the shipped layout exactly. We added VertexAABB fields,
            // so use sizeof so future field changes don't desync.
            v54 = (SMesh *)operator new(sizeof(SMesh));
            IndexBuffer = v54;
            // LOBYTE(v96) = 1;
            if ( v54 )
            {
              v55 = 17;
              if ( v91 )
                v55 = 81;
              new (v54) SMesh(v94->Gepard, v55);
              v92 = v54;
            }
            else
            {
              v92 = 0;
            }
            // LOBYTE(v96) = 0;
            {
              SMesh *oldMesh = v94->MeshArray.array[v85].Mesh.ptr;
              if (oldMesh) {
                oldMesh->Release();
              }
              v94->MeshArray.array[v85].Mesh.ptr = v92;
            }
            v92->CreateVertexBuffer(v53);
            Logger.g->Log(2, "                      Vertices: %d", v53);
            if ( v53 > 0xFFFF )
              Logger.g->Panic("SGroup::LoadNew4DFile: Too many vertices");
            v6->Read((char *)v92->lpVertices + v92->OffsetXYZ, v53 * (4 * (v91 != 0) + 32));
            for ( i = 0; i < v53; *(float *)&v62[OffsetNormal + 8] = *(float *)&v62[OffsetNormal + 8] * scale )
            {
              v61 = i * v92->VertexSize;
              ++i;
              v62 = (char *)v92->lpVertices + v61;
              OffsetNormal = v92->OffsetNormal;
              *(float *)&v62[OffsetNormal] = *(float *)&v62[OffsetNormal] * scale;
              *(float *)&v62[OffsetNormal + 4] = *(float *)&v62[OffsetNormal + 4] * scale;
            }
            // Walk the still-locked vertex buffer to compute a true AABB
            // around the position component. Stored on SMesh, used by the
            // editor object picker (world.cpp ProjectObjectScreenAABB) so
            // hover/click rects match the rendered silhouette instead of
            // the often trunk-sized authored Bound[8].
            if ( v53 > 0 )
            {
              D3DXVECTOR3 mn(+1e30f, +1e30f, +1e30f);
              D3DXVECTOR3 mx(-1e30f, -1e30f, -1e30f);
              for ( unsigned int vi = 0; vi < v53; ++vi )
              {
                const float *p = (const float *)((char *)v92->lpVertices
                    + vi * v92->VertexSize + v92->OffsetXYZ);
                if ( p[0] < mn.x ) mn.x = p[0]; if ( p[0] > mx.x ) mx.x = p[0];
                if ( p[1] < mn.y ) mn.y = p[1]; if ( p[1] > mx.y ) mx.y = p[1];
                if ( p[2] < mn.z ) mn.z = p[2]; if ( p[2] > mx.z ) mx.z = p[2];
              }
              v92->VertexAABBMin = mn;
              v92->VertexAABBMax = mx;
              v92->VertexAABBValid = 1;
            }
            v92->UnlockVertexBuffer();
            v4 = v94;
            v6->ReadChunkValidate(0);
          }
          else
          {
            if ( ChunkHeader == 1162035526 )
            {
              v52 = v6->ReadInt();
              IndexBuffer = v92->CreateIndexBuffer(0, 3 * v52);
              Logger.g->Log(2, "                      Faces: %d", v52);
              if ( (unsigned int)(3 * v52) > 0xFFFF )
                Logger.g->Panic("SGroup::LoadNew4DFile: Too many faces");
              v6->Read(IndexBuffer, 6 * v52);
              v92->UnlockIndexBuffer(0);
            }
            else
            {
              if ( ChunkHeader != 1397511245 )
                goto LABEL_100;
              v84 = v6->ReadInt();
              MaterialBuffer = v92->CreateMaterialBuffer(v84);
              v29 = 0;
              array = MaterialBuffer;
              while ( 1 )
              {
                v91 = v29;
                if ( v29 >= v84 )
                  break;
                if ( v6->ReadChunkHeader() != 1163149645 )
                {
                  throw "Unsupported material entry";
                }
                v30 = v6->ReadInt();
                v31 = 3 * v29;
                v90 = v31;
                *(&MaterialBuffer->NumFaces + 2 * v31) = v30;
                *(&MaterialBuffer->StartVertex + 2 * v31) = v6->ReadInt();
                *(&MaterialBuffer->NumVertices + 2 * v31) = v6->ReadInt();
                while ( !v6->ReadChunkIsEnd() )
                {
                  v32 = v6->ReadChunkHeader();
                  switch ( v32 )
                  {
                    case 1179011396:
                      if ( v6->NewTypeStrings )
                        v44 = v6->ReadWord();
                      else
                        v44 = v6->ReadWord() - 1;
                      v83 = v44;
                      if ( v44 < 0 )
                      {
                        v45 = 0;
                        v82 = 0;
                      }
                      else
                      {
                        v45 = operator new[](v44 + 1);
                        v82 = v45;
                        if ( !v45 )
                        {
                          throw "Out of memory";
                        }
                        v6->Read(v45, v44);
                        *((_BYTE *)v45 + v44) = 0;
                      }
                      // LOBYTE(v96) = 2;
                      v46 = strlen(textureprefix);
                      IndexBuffer = (void *)(v44 + v46);
                      block = operator new[](v44 + v46 + 1);
                      memcpy(block, textureprefix, v46);
                      v47 = v44 + 1;
                      v48 = (char *)block;
                      memcpy((char *)block + v46, v45, v47);
                      v88 |= 1u;
                      v66 = v48;
                      v67 = IndexBuffer;
                      // LOBYTE(v96) = 3;
                      if ( v48 )
                      {
                        v49 = v48;
                        v50 = v48;
                      }
                      else
                      {
                        v49 = fullpath;
                        v50 = 0;
                      }
                      v51 = v94->Gepard->LoadTexture(v49, 1, 1);
                      *(&array->TextureIndex + 2 * v90) = v51;
                      if ( v50 )
                      {
                        delete[] v48;
                        v66 = 0;
                      }
                      // LOBYTE(v96) = 0;
                      if ( v45 )
                      {
                        delete[] v45;
                        v82 = 0;
                      }
                      v6 = is;
                      MaterialBuffer = array;
                      v31 = v90;
                      is->ReadChunkValidate(0);
                      break;
                    case 1279673682:
                      if ( *(&MaterialBuffer->TextureIndex2 + 2 * v31) >= 0 )
 Logger.g->Panic(
                          "SGroup::LoadNew4DFile: Multiple effect textures not supported.");
                      { char *_tmp = v6->ReadString(); v68.buf = _tmp; v68.size = _tmp ? (int)strlen(_tmp) : 0; }
                      // x64: was `v37 = (_DWORD*)&v68; v37[1]` for SString::size — only valid
                      // on x86 where buf is 4 bytes. On x64 buf is 8, so v37[1] reads the
                      // high half of buf (typically 0), undersizing the new[] and copying
                      // 1 byte instead of size+1. The case below (1280067923) already does
                      // typed access; mirror that.
                      // LOBYTE(v96) = 6;
                      v38 = strlen(textureprefix);
                      v39 = new char[v38 + v68.size + 1];
                      memcpy(v39, textureprefix, v38);
                      if (v68.buf) memcpy(&v39[v38], v68.buf, v68.size + 1);
                      else v39[v38] = 0;
                      v88 |= 2u;
                      v69 = v39;
                      v70 = v38 + v68.size;
                      // LOBYTE(v96) = 7;
                      if ( v39 )
                      {
                        v40 = v39;
                        v41 = v39;
                      }
                      else
                      {
                        v40 = fullpath;
                        v41 = 0;
                      }
                      v42 = v94->Gepard->LoadTexture(v40, 1, 1);
                      *(&array->TextureIndex2 + 2 * v31) = v42;
                      if ( v41 )
                      {
                        delete[] v39;
                        v69 = 0;
                      }
                      // LOBYTE(v96) = 0;
                      if ( v68.buf )
                      {
                        delete[] v68.buf;
                        v68.buf = 0;
                      }
                      MaterialBuffer = array;
                      v6 = is;
                      v43 = is;
                      *(&array->Flags + 2 * v31) |= 2u;
                      v43->ReadChunkValidate(0);
                      break;
                    case 1280067923:
                      if ( *(&MaterialBuffer->TextureIndex2 + 2 * v31) >= 0 )
 Logger.g->Panic(
                          "SGroup::LoadNew4DFile: Multiple effect textures not supported.");
                      { char *_tmp = v6->ReadString(); v71.buf = _tmp; v71.size = _tmp ? (int)strlen(_tmp) : 0; }
                      // LOBYTE(v96) = 4;
                      { // operator+ for SString concat
                        int _plen = (int)strlen(textureprefix);
                        int _slen = v71.size;
                        result.buf = (char *)operator new[](_plen + _slen + 1);
                        memcpy(result.buf, textureprefix, _plen);
                        if (v71.buf) memcpy(result.buf + _plen, v71.buf, _slen + 1);
                        else result.buf[_plen] = 0;
                        result.size = _plen + _slen;
                      }
                      v34 = result.buf;
                      v35 = fullpath;
                      // LOBYTE(v96) = 5;
                      if ( v34 )
                        v35 = v34;
                      *(&MaterialBuffer->TextureIndex2 + 2 * v31) = v94->Gepard->LoadTexture(v35, 1, 1);
                      if ( result.buf )
                      {
                        delete[] result.buf;
                        result.buf = 0;
                      }
                      // LOBYTE(v96) = 0;
                      if ( v71.buf )
                      {
                        delete[] v71.buf;
                        v71.buf = 0;
                      }
                      *(&MaterialBuffer->Flags + 2 * v31) |= 4u;
                      v6->ReadChunkValidate(0);
                      break;
                    default:
                      throw "Unsupported mesh component";
                  }
                }
                v6->ReadChunkValidate(0);
                v29 = v91 + 1;
              }
            }
            v4 = v94;
            v6->ReadChunkValidate(0);
          }
        }
        if ( !MaterialBuffer )
        {
          throw "Mesh has no material";
        }
      }
      else
      {
        Logger.g->Warning("Unsupported scene entry");
        v6->ReadChunkSkip();
      }
      v6->ReadChunkValidate(0);
    }
    v6->ReadChunkValidate(0);
    v96 = -1;
    v6->Release();
    return 1;
  }
  else
  {
    Logger.g->Log(0, "SGroup::Load4DFile: Couldn't open %s", filename);
    return 0;
  }
}

//----- (0044ADF0) --------------------------------------------------------

void STransformation::LogToFile(FILE *logfile)

{
  fprintf(
    logfile,
    "Translation(X=%f %08X Y=%f %08X Z=%f %08X) Scale=%f %08X Rotation(X=%f %08X Y=%f %08X Z=%f %08X W=%f %08X)\n",
    this->Translation.x,
    float_bits(this->Translation.x),
    this->Translation.y,
    float_bits(this->Translation.y),
    this->Translation.z,
    float_bits(this->Translation.z),
    this->Scale,
    float_bits(this->Scale),
    this->Rotation.x,
    float_bits(this->Rotation.x),
    this->Rotation.y,
    float_bits(this->Rotation.y),
    this->Rotation.z,
    float_bits(this->Rotation.z),
    this->Rotation.w,
    float_bits(this->Rotation.w));
}

//----- (0044AEA0) --------------------------------------------------------

void SGroup::LogTransforms(FILE *logfile)

{
  int v3;
  int v4;
  SMeshProp *array; // eax
  fprintf(
    logfile,
    "Translation(X=%f %08X Y=%f %08X Z=%f %08X) Scale=%f %08X Rotation(X=%f %08X Y=%f %08X Z=%f %08X W=%f %08X)\n",
    this->Transformation.Translation.x,
    float_bits(this->Transformation.Translation.x),
    this->Transformation.Translation.y,
    float_bits(this->Transformation.Translation.y),
    this->Transformation.Translation.z,
    float_bits(this->Transformation.Translation.z),
    this->Transformation.Scale,
    float_bits(this->Transformation.Scale),
    this->Transformation.Rotation.x,
    float_bits(this->Transformation.Rotation.x),
    this->Transformation.Rotation.y,
    float_bits(this->Transformation.Rotation.y),
    this->Transformation.Rotation.z,
    float_bits(this->Transformation.Rotation.z),
    this->Transformation.Rotation.w,
    float_bits(this->Transformation.Rotation.w));
  v3 = 0;
  if ( this->MeshArray.size > 0 )
  {
    v4 = 0;
    do
    {
      array = this->MeshArray.array;
      fprintf(
        logfile,
        "Translation(X=%f %08X Y=%f %08X Z=%f %08X) Scale=%f %08X Rotation(X=%f %08X Y=%f %08X Z=%f %08X W=%f %08X)\n",
        array[v4].Transformation.Translation.x,
        float_bits(array[v4].Transformation.Translation.x),
        array[v4].Transformation.Translation.y,
        float_bits(array[v4].Transformation.Translation.y),
        array[v4].Transformation.Translation.z,
        float_bits(array[v4].Transformation.Translation.z),
        array[v4].Transformation.Scale,
        float_bits(array[v4].Transformation.Scale),
        array[v4].Transformation.Rotation.x,
        float_bits(array[v4].Transformation.Rotation.x),
        array[v4].Transformation.Rotation.y,
        float_bits(array[v4].Transformation.Rotation.y),
        array[v4].Transformation.Rotation.z,
        float_bits(array[v4].Transformation.Rotation.z),
        array[v4].Transformation.Rotation.w,
        float_bits(array[v4].Transformation.Rotation.w));
      ++v3;
      ++v4;
    }
    while ( v3 < this->MeshArray.size );
  }
}

//----- (0044B070) --------------------------------------------------------

void SGroup::Precalculate()

{
  SGepard *Gepard; // ecx
  unsigned int Flags;
  float v4; // xmm2_4
  float v5; // xmm0_4
  float v6; // xmm0_4
  float v7; // xmm0_4
  float WorldTime;
  bool v9;
  STransformation *p_Transformation; // ecx
  SGepard *v11; // edx
  float Interpolation; // xmm2_4
  int v13;
  int v14;
  SMeshProp *array; // ecx
  float v16; // xmm2_4
  SMatrix4 *Matrix; // eax
  SMeshProp *v18; // ecx
  int Parent;
  D3DXMATRIX *p_world; // eax
  D3DXMATRIX *p_OriginMatrix;
  float v29;
  SMatrix4 result;
  STransformation trans;
  D3DXMATRIX dynamic;
  D3DXMATRIX world;
  D3DXMATRIX mat;
  if ( this->Visible )
  {
    Gepard = this->Gepard;
    if ( this->LastRecalculated != Gepard->FrameCount )
    {
      Flags = this->Flags;
      v4 = 0.001f;
      if ( (Flags & 8) != 0 )
      {
        v29 = (float)Gepard->WorldTime * 0.001f;
        v5 = sinf(v29 * this->treeBendFreqX + this->treeBendPhaseX);
        this->treeBendParams[0] = v5 * this->treeBendAmplitude;
        v6 = cosf(v29 * this->treeBendFreqZ + this->treeBendPhaseZ);
        this->treeBendParams[1] = v6 * this->treeBendAmplitude;
        v7 = sqrtf(this->treeBendParams[0] * this->treeBendParams[0] + 1.0f + this->treeBendParams[1] * this->treeBendParams[1]);
        v4 = 0.001f;
        this->treeBendParams[3] = v29;
        this->treeBendParams[2] = (float)(1.0f / v7) - 1.0f;
        Flags = this->Flags;
      }
      if ( (Flags & 0x10) != 0 )
      {
        WorldTime = (float)((double)this->Gepard->WorldTime);

        this->treeWobbleParams[0] = this->treeWobbleAmplitude;
        this->treeWobbleParams[3] = (float)WorldTime * v4;
      }
      if ( this->Recalculate || (this->Flags & 6) != 0 )
      {
        v9 = (this->Flags & 2) == 0;
        p_Transformation = &this->Transformation;
        v11 = this->Gepard;
        this->Recalculate = 0;
        this->LastRecalculated = v11->FrameCount;
        if ( !v9 )
        {
          Interpolation = (float)(v11->Interpolation);

          trans.Translation.x = (float)((float)(this->LastTransformation.Translation.x - p_Transformation->Translation.x)
                                      * Interpolation)
                              + p_Transformation->Translation.x;
          trans.Translation.y = (float)((float)(this->LastTransformation.Translation.y
                                              - this->Transformation.Translation.y)
                                      * Interpolation)
                              + this->Transformation.Translation.y;
          trans.Translation.z = (float)((float)(this->LastTransformation.Translation.z
                                              - this->Transformation.Translation.z)
                                      * Interpolation)
                              + this->Transformation.Translation.z;
          trans.Scale = (float)((float)(this->LastTransformation.Scale - this->Transformation.Scale) * Interpolation)
                      + this->Transformation.Scale;
          D3DXQuaternionSlerp(
            &trans.Rotation,
            &this->Transformation.Rotation,
            &this->LastTransformation.Rotation,
            Interpolation);
          p_Transformation = &trans;
        }
        v13 = 0;
        world = *(D3DXMATRIX *)p_Transformation->GetMatrix(&result);
        if ( this->MeshArray.size > 0 )
        {
          v14 = 0;
          do
          {
            if ( (this->Flags & 4) != 0 )
            {
              array = this->MeshArray.array;
              v16 = (float)(this->Gepard->Interpolation);

              trans.Translation.x = (float)((float)(array[v14].LastTransformation.Translation.x
                                                  - array[v14].Transformation.Translation.x)
                                          * v16)
                                  + array[v14].Transformation.Translation.x;
              trans.Translation.y = (float)((float)(array[v14].LastTransformation.Translation.y
                                                  - array[v14].Transformation.Translation.y)
                                          * v16)
                                  + array[v14].Transformation.Translation.y;
              trans.Translation.z = (float)((float)(array[v14].LastTransformation.Translation.z
                                                  - array[v14].Transformation.Translation.z)
                                          * v16)
                                  + array[v14].Transformation.Translation.z;
              trans.Scale = (float)((float)(array[v14].LastTransformation.Scale - array[v14].Transformation.Scale) * v16)
                          + array[v14].Transformation.Scale;
              D3DXQuaternionSlerp(
                &trans.Rotation,
                &array[v14].Transformation.Rotation,
                &array[v14].LastTransformation.Rotation,
                v16);
              Matrix = trans.GetMatrix(&result);
            }
            else
            {
              Matrix = this->MeshArray.array[v14].Transformation.GetMatrix(&result);
            }
            memcpy(&dynamic, Matrix, sizeof(D3DXMATRIX));
            p_OriginMatrix = &this->MeshArray.array[v14].OriginMatrix;
            D3DXMatrixMultiply(&mat, &dynamic, p_OriginMatrix);
            v18 = this->MeshArray.array;
            Parent = v18[v14].Parent;
            if ( Parent < 0 )
              p_world = &world;
            else
              p_world = &v18[Parent].WorldMatrix;
            D3DXMatrixMultiply(&v18[v14].WorldMatrix, &mat, p_world);
            ++v13;
            ++v14;
          }
          while ( v13 < this->MeshArray.size );
        }
      }
    }
  }
}

//----- (0044B490) --------------------------------------------------------

void SGroup::Release()

{
  if ( this->RefCount-- == 1 )
    delete this;
}

//----- (0044B4A0) --------------------------------------------------------

void SGroup::Replace(const SGroup *src)

{
  SGroup::SEffects *first; // edx
  SGroup::SEffects *current; // eax
  SGroup::SEffects *next; // eax
  int size;
  int v7;
  int v8;
  char *v9; // esi
  int v10;
  int maxsize;
  const SGroup *v12; // edx
  int v13;
  int v14;
  int v15;
  SMeshProp *v16; // eax
  int v17;
  int v18;
  char *v19; // eax
  int v20;
  void *v21; // eax
  SMeshProp *v22; // ecx
  SMeshProp *v23; // eax
  SMeshProp *v24; // eax
  SMeshProp *v25; // eax
  SMeshProp *v26; // eax
  int v27;
  SMesh *v28; // ecx
  SMeshProp *v29; // ecx
  SMeshProp *v30; // eax
  SGepard *Gepard; // ecx
  SShadowMask2 *ShadowMask2; // esi
  SMesh *ShadowMesh; // ecx
  SMeshProp *array;
  int v35;
  char *v36;
  SMeshProp *v37;
  int v38;
  int v39;
  int v40;
  int v41;
  char *v42;
  SMeshProp *v43;
  first = this->Effects.first;
  for ( this->Effects.current = first; first; first = this->Effects.current )
  {
    this->Gepard->RefreshIndices(first->effect);
    current = this->Effects.current;
    if ( this->Effects.Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = this->Effects.first;
      this->Effects.current = next;
    }
    else if ( current )
    {
      this->Effects.current = current->next;
    }
    else
    {
      this->Effects.current = 0;
    }
  }
  this->DrawType = src->DrawType;
  this->ambientMode = 0;
  size = this->MeshArray.size;
  if ( size && !this->MeshArray.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  v7 = 0;
  v40 = 0;
  if ( size > 0 )
  {
    v8 = 0;
    v38 = 0;
    do
    {
      // x64 fix: original used `*(_DWORD *)v9` for SString.buf and
      // `*((_DWORD *)v9 + 55)` for SPtr<SMesh>::ptr at offset 220 (x86).
      // Both layouts shift on x64 — use typed SMeshProp access.
      SMeshProp *mp = (SMeshProp *)((char *)this->MeshArray.array + v8);
      if (mp->Mesh.ptr)
      {
        // Original called vtable slot 1 (Release) via `+ 8` byte offset.
        mp->Mesh.ptr->Release();
        mp->Mesh.ptr = nullptr;
      }
      if (mp->MeshName.buf)
      {
        delete[] mp->MeshName.buf;
        mp->MeshName.buf = nullptr;
      }
      ++v7;
      // x64 fix: original `+= 320` assumed x86 sizeof(SMeshProp).
      v8 = v38 + (int)sizeof(SMeshProp);
      v40 = v7;
      v38 += (int)sizeof(SMeshProp);
      (void)v9; (void)v10;
    }
    while ( v7 < this->MeshArray.size );
  }
  maxsize = this->MeshArray.maxsize;
  this->MeshArray.size = 0;
  if ( maxsize < 0 )
  {
    array = this->MeshArray.array;
    this->MeshArray.maxsize = 0;
    this->MeshArray.array = (SMeshProp *)realloc(array, 0);
    maxsize = this->MeshArray.maxsize;
  }
  memset(this->MeshArray.array, 0, sizeof(SMeshProp) * maxsize);
  v12 = src;
  v39 = 0;
  if ( src->MeshArray.size > 0 )
  {
    v13 = 0;
    v35 = 0;
    do
    {
      v14 = this->MeshArray.maxsize;
      if ( this->MeshArray.size == v14 )
      {
        if ( v14 >= 16 )
          v15 = 6 * v14 / 5;
        else
          v15 = 16;
        v41 = v15;
        v16 = (SMeshProp *)realloc(this->MeshArray.array, sizeof(SMeshProp) * v15);
        v17 = this->MeshArray.maxsize;
        this->MeshArray.array = v16;
        memset(&v16[v17], 0, sizeof(SMeshProp) * (v41 - v17));
        v13 = v35;
        v12 = src;
        this->MeshArray.maxsize = v41;
      }
      v18 = this->MeshArray.size;
      this->MeshArray.size = v18 + 1;
      if ( v39 != v18 )
        Logger.g->Panic("SGroup::Replace: Damaged MeshArray");
      // x64 fix: same _DWORD ptr-truncation as the SGroup copy ctor — read/wrote
      // SString.buf as 4 bytes and treated offset+4 as size, both x86-only.
      {
        SMeshProp *src_mp = (SMeshProp *)((char *)v12->MeshArray.array + v13);
        SMeshProp *dst_mp = (SMeshProp *)((char *)this->MeshArray.array + v13);
        if (dst_mp->MeshName.buf)
        {
          delete[] dst_mp->MeshName.buf;
          dst_mp->MeshName.buf = nullptr;
        }
        if (src_mp->MeshName.size)
        {
          dst_mp->MeshName.size = src_mp->MeshName.size;
          dst_mp->MeshName.buf = new char[src_mp->MeshName.size + 1];
          memcpy(dst_mp->MeshName.buf, src_mp->MeshName.buf, src_mp->MeshName.size + 1);
        }
        else
        {
          dst_mp->MeshName.size = 0;
          dst_mp->MeshName.buf = nullptr;
        }
      }
      (void)v19; (void)v20; (void)v21; (void)v36; (void)v42;
      *(int *)((char *)&this->MeshArray.array->Parent + v13) = *(int *)((char *)&src->MeshArray.array->Parent + v13);
      v22 = src->MeshArray.array;
      v23 = this->MeshArray.array;
      memcpy((char *)&v23->OriginMatrix + v13, (char *)&v22->OriginMatrix + v13, sizeof(D3DXMATRIX));
      v24 = this->MeshArray.array;
      *(float *)((char *)&v24->Transformation.Translation.x + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Translation.y + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Translation.z + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Scale + v13) = 1.0;
      *(float *)((char *)&v24->Transformation.Rotation.z + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Rotation.y + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Rotation.x + v13) = 0.0;
      *(float *)((char *)&v24->Transformation.Rotation.w + v13) = 1.0;
      v25 = this->MeshArray.array;
      *(float *)((char *)&v25->WorldMatrix._43 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._42 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._41 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._34 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._32 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._31 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._24 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._23 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._21 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._14 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._13 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._12 + v13) = 0.0;
      *(float *)((char *)&v25->WorldMatrix._44 + v13) = 1.0;
      *(float *)((char *)&v25->WorldMatrix._33 + v13) = 1.0;
      *(float *)((char *)&v25->WorldMatrix._22 + v13) = 1.0;
      *(float *)((char *)&v25->WorldMatrix._11 + v13) = 1.0;
      // x64 fix: original read SPtr<SMesh>::ptr as `*(int *)`, truncating to 32 bits.
      {
        SMeshProp *src_mp = (SMeshProp *)((char *)src->MeshArray.array + v13);
        SMeshProp *dst_mp = (SMeshProp *)((char *)this->MeshArray.array + v13);
        if (dst_mp->Mesh.ptr)
        {
          dst_mp->Mesh.ptr->Release();
          dst_mp->Mesh.ptr = nullptr;
        }
        SMesh *src_mesh = src_mp->Mesh.ptr;
        dst_mp->Mesh.ptr = src_mesh;
        if (src_mesh)
          src_mesh->AddRef();
      }
      (void)v26; (void)v27; (void)v28; (void)v37; (void)v43;
      v12 = src;
      *(&this->MeshArray.array->Visible + v13) = *(&src->MeshArray.array->Visible + v13);
      *(&this->MeshArray.array->CanCastShadows + v13) = *(&src->MeshArray.array->CanCastShadows + v13);
      v29 = src->MeshArray.array;
      v30 = this->MeshArray.array;
      memcpy((char *)&v30->Bound[0] + v13, (char *)&v29->Bound[0] + v13, sizeof(D3DXVECTOR3) * 8);
      v13 += (int)sizeof(SMeshProp);
      ++v39;
      v35 = v13;
    }
    while ( v39 < src->MeshArray.size );
  }
  Gepard = this->Gepard;
  this->Recalculate = 1;
  Gepard->ReleaseTexture(this->ShadowTexture, 0);
  ShadowMask2 = this->ShadowMask2;
  this->ShadowTexture = -1;
  if ( ShadowMask2 )
  {
    delete ShadowMask2;
    this->ShadowMask2 = 0;
  }
  ShadowMesh = this->ShadowMesh;
  this->NeedNewShadow = 1;
  if ( ShadowMesh )
  {
    ShadowMesh->Release();
    this->ShadowMesh = 0;
  }
}

//----- (0044B9D0) --------------------------------------------------------

void SGroup::SaveStatus()

{
  unsigned int Flags;
  int v3;
  int v4;
  SMeshProp *array; // eax
  SMeshProp *v6; // edi
  Flags = this->Flags;
  if ( (Flags & 2) != 0 )
    this->LastTransformation = this->Transformation;
  if ( (Flags & 4) != 0 )
  {
    v3 = 0;
    if ( this->MeshArray.size > 0 )
    {
      v4 = 0;
      do
      {
        array = this->MeshArray.array;
        memcpy(&array[v4].LastTransformation.Translation, &array[v4].Transformation.Translation, sizeof(D3DXVECTOR3) + sizeof(float));
        array[v4].LastTransformation.Rotation = array[v4].Transformation.Rotation;
        v6 = this->MeshArray.array;
        if ( v6[v4].HasUVTranslate )
          v6[v4].LastUVTranslate = v6[v4].UVTranslate;
        ++v3;
        ++v4;
      }
      while ( v3 < this->MeshArray.size );
    }
  }
}

//----- (0044BA50) --------------------------------------------------------

void SGroup::SetActivationState(bool state)

{
  this->Activated = state;
}

//----- (0044BA60) --------------------------------------------------------

void SGroup::SetAmbient(D3DCOLORVALUE ambient)

{
  this->ambientMode = 1;
  this->ambientColor = ambient;
}

//----- (0044BA80) --------------------------------------------------------

void SGroup::SetAmbient(float ambient)

{
  _D3DCOLORVALUE v2;
  v2.r = ambient;
  v2.g = ambient;
  v2.b = ambient; v2.a = 1.0f;
  this->ambientMode = 1;
  this->ambientColor = v2;
}

//----- (0044BAC0) --------------------------------------------------------

void SGroup::SetColorizeProperties(int hue, float saturationMultiplier, float valueMultiplier)

{
  float v4; // xmm1_4
  float v5; // xmm3_4
  float v6; // xmm1_4
  float v7; // xmm2_4
  float v8; // xmm2_4
  float v9; // xmm3_4
  this->IsColorized = 1;
  v4 = (float)hue;
  if ( hue < 60 )
  {
    v5 = v4 / 60.0f;
    v6 = 0.0;
LABEL_12:
    v8 = 1.0f;
    goto LABEL_13;
  }
  if ( hue >= 120 )
  {
    if ( hue >= 180 )
    {
      if ( hue >= 240 )
      {
        v5 = 0.0;
        if ( hue >= 300 )
        {
          v6 = (float)(360.0f - v4) / 60.0f;
          goto LABEL_12;
        }
        v6 = 1.0f;
        v8 = (float)((float)hue - 240.0f) / 60.0f;
      }
      else
      {
        v8 = 0.0;
        v9 = 240.0f - v4;
        v6 = 1.0f;
        v5 = v9 / 60.0f;
      }
    }
    else
    {
      v5 = 1.0f;
      v8 = 0.0;
      v6 = v4 - 120.0f / 60.0f;
    }
  }
  else
  {
    v5 = 1.0f;
    v7 = 120.0f - v4;
    v6 = 0.0;
    v8 = v7 / 60.0f;
  }
LABEL_13:
  this->ColorizeConstants.SM = saturationMultiplier;
  this->ColorizeConstants.R = v8;
  this->ColorizeConstants.G = v5;
  this->ColorizeConstants.B = v6;
  this->ColorizeConstants.VM = valueMultiplier;
}

//----- (0044BBE0) --------------------------------------------------------

void *SGroup::SetEffect(void *effect)

{
  SGroup::SEffects *v3; // edx
  SGroup::SEffects *last; // eax
  v3 = (SGroup::SEffects *)operator new(sizeof(SGroup::SEffects));
  // x64 fix: original literal `0xCu` (= 12 bytes) zeroed the 3 ptrs on x86 only.
  // On x64 each ptr is 8 bytes, so 12 bytes left half of `next` and all of
  // `effect` uninitialized — the else branch below didn't set them, and
  // ~SGroup later read `next == 0xcdcdcdcd00000000` and dereferenced it.
  memset(v3, 0, sizeof(SGroup::SEffects));
  last = this->Effects.last;
  if ( last )
  {
    last->next = v3;
    v3->prev = this->Effects.last;
    this->Effects.last = v3;
    v3->next = 0;
  }
  else
  {
    this->Effects.last = v3;
    this->Effects.first = v3;
  }
  ++this->Effects.NumItems;
  this->Effects.current = v3;
  v3->effect = effect;
  return this->Effects.current;
}

//----- (0044BC50) --------------------------------------------------------

void SGroup::SetFlags(unsigned int object_flags)

{
  SShadowMask2 *ShadowMask2; // edi
  SMesh *ShadowMesh; // ecx
  this->Flags = object_flags;
  if ( (object_flags & 1) == 0 )
  {
    this->Gepard->ReleaseTexture(this->ShadowTexture, 0);
    ShadowMask2 = this->ShadowMask2;
    this->ShadowTexture = -1;
    if ( ShadowMask2 )
    {
      delete ShadowMask2;
      this->ShadowMask2 = 0;
    }
    ShadowMesh = this->ShadowMesh;
    this->NeedNewShadow = 1;
    if ( ShadowMesh )
    {
      ShadowMesh->Release();
      this->ShadowMesh = 0;
    }
  }
  if ( (this->Flags & 6) != 0 && this->Type != 1 )
    Logger.g->Panic("SGroup::SetFlags: Non dynamic objects cannot be interpolated");
  this->SaveStatus();
}

//----- (0044BD00) --------------------------------------------------------

void SGroup::SetMatrix(D3DXMATRIX m)

{
  float v3; // xmm0_4
  bool v4;
  D3DXMATRIX fin;
  memset(&fin, 0, sizeof(fin));
  fin._11 = 1.0f;
  fin.m[1][2] = 1.0f;
  fin.m[2][1] = 1.0f;
  fin._44 = 1.0f;
  D3DXMatrixMultiply(&m, &fin, &m);
  this->Transformation.Translation.x = m.m[3][0];
  this->Transformation.Translation.y = m.m[3][1];
  this->Transformation.Translation.z = m._43;
  float det = D3DXMatrixDeterminant(&m);
  v3 = 1.0f / cbrtf(det);
  D3DXMatrixScaling(&fin, v3, v3, v3);
  D3DXMatrixMultiply(&m, &fin, &m);
  D3DXQuaternionRotationMatrix(&this->Transformation.Rotation, &m);
  D3DXQuaternionNormalize(&this->Transformation.Rotation, &this->Transformation.Rotation);
  v4 = this->Type == 2;
  this->Recalculate = 1;
  this->NeedNewShadow = 1;
  if ( v4 )
    this->Gepard->TriggerRehash();
}

//----- (0044BE50) --------------------------------------------------------

void SGroup::SetMeshCanCastShadows(int idx, bool castShadows)

{
  if ( idx >= 0 && idx < this->MeshArray.size )
  {
    this->MeshArray.array[idx].CanCastShadows = castShadows;
    this->NeedNewShadow = 1;
  }
}

//----- (0044BE90) --------------------------------------------------------

void SGroup::SetMeshPosition(int idx, float x, float y, float z)

{
  float v6; // xmm1_4
  int v7;
  if ( idx >= 0 && idx < this->MeshArray.size )
  {
    v6 = 1.0f / this->Transformation.Scale;
    v7 = idx;
    this->MeshArray.array[v7].Transformation.Translation.x = (float)(1.0 / this->Transformation.Scale) * x;
    this->MeshArray.array[v7].Transformation.Translation.y = v6 * y;
    this->MeshArray.array[v7].Transformation.Translation.z = v6 * z;
    this->Recalculate = 1;
  }
}

//----- (0044BF00) --------------------------------------------------------

void SGroup::SetMeshProperties(int idx, float x, float y, float z, float dir, float tiltx, float tilty)

{
  this->SetMeshPosition(idx, x, y, z);
  this->SetMeshRotation(idx, dir, tiltx, tilty);
}

//----- (0044BF70) --------------------------------------------------------

void SGroup::SetMeshRotation2(int idx, float steer, float roll)

{
  float v6; // xmm0_4
  float v7; // xmm0_4
  float v8; // xmm0_4
  float v9; // xmm7_4
  float v10; // xmm3_4
  float v11; // xmm6_4
  D3DXQUATERNION v12;
  float v13;
  float idxa;
  float steera;
  float rolla;
  if ( idx >= 0 && idx < this->MeshArray.size )
  {
    float half_roll = -roll * 0.5f;
    v6 = cosf(half_roll);
    rolla = v6;
    v7 = sinf(half_roll);
    idxa = v7;
    v13 = v7 * 0.0f;
    float half_steer = steer * 0.5f;
    v8 = cosf(half_steer);
    steera = v8;
    v9 = sinf(half_steer);
    v10 = rolla * (float)(v9 * 0.0);
    v11 = (float)(v9 * 0.0) * v13;
    v12.x = (float)((float)((float)(steera * idxa) + v10) + v11) - (float)(v9 * v13);
    v12.y = (float)((float)((float)(steera * v13) + v10) + (float)(v9 * idxa)) - v11;
    v12.z = (float)((float)((float)(v9 * rolla) + (float)(steera * v13)) + v11) - (float)(idxa * (float)(v9 * 0.0));
    v12.w = (float)((float)((float)(steera * rolla) - (float)(idxa * (float)(v9 * 0.0))) - v11) - (float)(v9 * v13);
    this->MeshArray.array[idx].Transformation.Rotation = v12;
    this->Recalculate = 1;
  }
}

//----- (0044C0D0) --------------------------------------------------------

void SGroup::SetMeshRotation(int idx, float dir, float tiltx, float tilty)

{
  float v6; // xmm0_4
  float v7;
  float v8; // xmm1_4
  float v9; // xmm1_4
  float v10; // xmm0_4
  float v11; // xmm0_4
  float v12; // xmm0_4
  float v13; // xmm6_4
  float v14; // xmm5_4
  float v15; // xmm5_4
  float v16; // xmm4_4
  float v17; // xmm6_4
  float v18; // xmm1_4
  float v19; // xmm0_4
  float v20; // xmm5_4
  float v21; // xmm1_4
  float v22; // xmm4_4
  float v23;
  float v24;
  float v25;
  float v26;
  D3DXQUATERNION v27;
  if ( idx >= 0 && idx < this->MeshArray.size )
  {
    v6 = sqrtf(tiltx * tiltx + tilty * tilty);
    v7 = v6;
    if ( v6 >= 0.00001 )
    {
      float half_dir = dir * 0.5f;
      v10 = cosf(half_dir);
      v23 = v10;
      v11 = sinf(half_dir);
      v24 = v11;
      v26 = v11 * 0.0f;
      float half_tilt = atanf(v7) * 0.5f;
      v12 = cosf(half_tilt);
      v25 = v12;
      v13 = sinf(half_tilt);
      v14 = (-tilty) / v7;
      v15 = v14 * v13;
      v16 = (float)(tiltx / v7) * v13;
      v17 = v13 * 0.0f;
      v18 = (float)((float)(v15 * v23) + (float)(v25 * v26)) + (float)(v16 * v24);
      v19 = v15;
      v20 = v15 * v26;
      v27.x = v18 - (float)(v17 * v26);
      v21 = v16 * v23;
      v22 = v16 * v26;
      v27.y = (float)((float)(v21 + (float)(v25 * v26)) + (float)(v17 * v26)) - (float)(v19 * v24);
      v9 = (float)((float)((float)(v25 * v24) + (float)(v17 * v23)) + v20) - v22;
      v27.w = (float)((float)((float)(v25 * v23) - v20) - v22) - (float)(v17 * v24);
    }
    else
    {
      float half_dir = dir * 0.5f;
      v8 = cosf(half_dir);
      v27.w = v8;
      v9 = sinf(half_dir);
      v27.x = v9 * 0.0f;
      v27.y = v9 * 0.0f;
    }
    v27.z = v9;
    this->MeshArray.array[idx].Transformation.Rotation = v27;
    this->Recalculate = 1;
  }
}

//----- (0044C310) --------------------------------------------------------

void SGroup::SetMeshUVTransformActive(int idx, bool active)

{
  this->MeshArray.array[idx].HasUVTranslate = active;
}

//----- (0044C330) --------------------------------------------------------

void SGroup::SetMeshUVTransformValue(int idx, float value)

{
  if ( idx >= 0 && idx < this->MeshArray.size )
    this->MeshArray.array[idx].UVTranslate = value;
}

//----- (0044C360) --------------------------------------------------------

void SGroup::SetMeshVisible(int idx, bool visible)

{
  int v4;
  int v5;
  SMeshProp *array; // ebx
  int Parent;
  char *buf; // edi
  if ( idx >= 0 && idx < this->MeshArray.size )
  {
    this->MeshArray.array[idx].Visible = visible;
    v4 = idx + 1;
    if ( idx + 1 < this->MeshArray.size )
    {
      v5 = v4;
      do
      {
        array = this->MeshArray.array;
        Parent = array[v5].Parent;
        if ( Parent >= 0 )
        {
          buf = array[v5].MeshName.buf;
          if ( !buf || *buf != 95 )
            array[v5].Visible = array[Parent].Visible;
        }
        ++v4;
        ++v5;
      }
      while ( v4 < this->MeshArray.size );
    }
    this->NeedNewShadow = 1;
  }
}

//----- (0044C3E0) --------------------------------------------------------

void SGroup::SetPosition(float x, float y, float z)

{
  bool v4;
  v4 = this->Type == 2;
  this->Transformation.Translation.x = x;
  this->Transformation.Translation.y = y;
  this->Transformation.Translation.z = z;
  this->Recalculate = 1;
  this->NeedNewShadow = 1;
  if ( v4 )
    this->Gepard->TriggerRehash();
}

//----- (0044C420) --------------------------------------------------------

void SGroup::SetRelativeRotation(float x, float y, float z, float angle)

{
  D3DXQUATERNION rotquat;
  _DWORD v7[3];
  *(float *)v7 = x;
  *(float *)&v7[1] = y;
  *(float *)&v7[2] = z;
  D3DXQuaternionRotationAxis(&rotquat, (D3DXVECTOR3 *)v7, angle);
  D3DXQUATERNION *pRot = (D3DXQUATERNION *)((char *)this + 44);
  D3DXQuaternionMultiply(pRot, pRot, &rotquat);
  D3DXQuaternionNormalize(pRot, pRot);
}

//----- (0044C490) --------------------------------------------------------

void SGroup::SetRotation2(float pitch, float yaw, float roll)

{
  D3DXQuaternionRotationYawPitchRoll(&this->Transformation.Rotation, yaw, pitch, roll);
  this->Recalculate = 1;
  this->NeedNewShadow = 1;
}

//----- (0044C4E0) --------------------------------------------------------

void SGroup::SetRotation(float dir, float tiltx, float tiltz)

{
  float v6; // xmm0_4
  float v7; // xmm1_4
  float v8; // xmm1_4
  float v9; // xmm0_4
  float v10; // xmm0_4
  float v11; // xmm0_4
  float v12; // xmm1_4
  float v13; // xmm7_4
  float v14; // xmm6_4
  float v15; // xmm6_4
  float v16; // xmm5_4
  float v18;
  float v19;
  float v20;
  float v21;
  double v22;
  D3DXQUATERNION var10_4;
  v6 = sqrtf(tiltx * tiltx + tiltz * tiltz);
  v22 = v6;
  if ( v6 >= 0.00001 )
  {
    float half_dir = dir * 0.5f;
    v9 = cosf(half_dir);
    v18 = v9;
    v10 = sinf(half_dir);
    v19 = v10;
    v21 = v10 * 0.0f;
    float half_tilt = atanf((float)v22) * 0.5f;
    v11 = cosf(half_tilt);
    v20 = v11;
    v12 = sinf(half_tilt);
    v13 = (float)(tiltz / v22) * v12;
    v14 = (float)((-tiltx) / v22);

    v15 = v14 * v12;
    v16 = (float)(v12 * 0.0) * v21;
    var10_4.x = (float)((float)((float)(v13 * v18) + (float)(v20 * v21)) + v16) - (float)(v15 * v19);
    var10_4.z = (float)((float)((float)(v15 * v18) + (float)(v20 * v21)) + (float)(v13 * v19)) - v16;
    var10_4.y = (float)((float)((float)((float)(v12 * 0.0) * v18) + (float)(v20 * v19)) + (float)(v15 * v21))
              - (float)(v13 * v21);
    var10_4.w = (float)((float)((float)(v20 * v18) - (float)(v13 * v21)) - (float)((float)(v12 * 0.0) * v19))
              - (float)(v15 * v21);
  }
  else
  {
    float half_dir = dir * 0.5f;
    v7 = cosf(half_dir);
    var10_4.w = v7;
    v8 = sinf(half_dir);
    var10_4.y = v8;
    var10_4.x = v8 * 0.0f;
    var10_4.z = v8 * 0.0f;
  }
  this->NeedNewShadow = 1;
  this->Recalculate = 1;
  this->Transformation.Rotation = var10_4;
}

//----- (0044C730) --------------------------------------------------------

void SGroup::SetScale(float scale)

{
  this->Transformation.Scale = scale;
  this->Recalculate = 1;
  this->NeedNewShadow = 1;
}

//----- (0044C750) --------------------------------------------------------

void SGroup::SetSelection(int selection)

{
  this->Selection = selection;
}

//----- (0044C760) --------------------------------------------------------

void SGroup::SetTreeParams(float bendAmplitude, float wobbleAmplitude)

{
  this->treeBendAmplitude = bendAmplitude;
  this->treeWobbleAmplitude = wobbleAmplitude;
}

//----- (0044C790) --------------------------------------------------------

void SGroup::SetVisClass(int visclass)

{
  this->VisClass = visclass;
}

//----- (0044C7A0) --------------------------------------------------------

void SGroup::Show(bool visible)

{
  bool v2;
  if ( this->Visible != visible )
  {
    v2 = this->Type == 2;
    this->Visible = visible;
    if ( v2 )
      this->Gepard->TriggerRehash();
  }
}

// === SShadowMask2 ===

SShadowMask2::SShadowMask2(const SShadowMask *source)
{
  int w = (source->Width + 1) >> 1;
  Width = w;
  int p = (w + 3) >> 2;
  Pitch = p;
  unsigned int h = ((source->Height >> 1) + 3) & 0xFFFFFFFC;
  Height = h;
  Size = h * p;
  Data = new unsigned char[Size];
  // RLE compress from source — simplified stub: zero-fill
  memset(Data, 0, Size);
}

SShadowMask2::SShadowMask2(SStream *is)
{
  Width = is->ReadInt();
  Height = is->ReadInt();
  Pitch = is->ReadInt();
  int sz = is->ReadInt();
  Size = sz;
  Data = new unsigned char[sz];
  is->Read(Data, Size);
}

SShadowMask2::~SShadowMask2()
{
  if (Data) {
    delete[] Data;
    Data = 0;
  }
}

//----- (00465490) --------------------------------------------------------
static unsigned short grays[4] = { 0, 21162, 44373, 65535 };

void SShadowMask2::MakeBitmap(SBitmap *destination)
{
  if (destination->Height < Height || destination->Width < 4 * Pitch)
  {
    Logger.g->Panic("SShadowMask2::MakeBitmap: invalid destination bitmap");
    return;
  }

  if (destination->Format == D3DFMT_DXT1)
  {
    unsigned char *src = Data;
    unsigned char *dst = &destination->Data[destination->Start];
    int y = 0;
    if (Height > 0)
    {
      do
      {
        unsigned char *dstRow = dst;
        unsigned char *srcRow = src;
        int x = 0;
        if (Width > 0)
        {
          do
          {
            x += 4;
            unsigned int v13 = *srcRow | ((srcRow[Pitch] | ((srcRow[2 * Pitch] | (srcRow[2 * Pitch + Pitch] << 8)) << 8)) << 8);
            *(_DWORD *)dstRow = 0xFFFF;
            *((_DWORD *)dstRow + 1) = v13 ^ (2 * v13) ^ (v13 ^ (2 * v13) ^ ((unsigned int)~v13 >> 1)) & 0x55555555;
            dstRow += 8;
            srcRow++;
          }
          while (x < Width);
        }
        for (; x < destination->Width; x += 4)
        {
          *(_DWORD *)dstRow = 0xFFFF;
          dstRow += 8;
          *((_DWORD *)dstRow - 1) = 1431655765;
        }
        dst += destination->Pitch;
        src += 4 * Pitch;
        y += 4;
      }
      while (y < Height);
    }
    for (; y < destination->Height; y += 4)
    {
      unsigned char *dstRow = dst;
      int x = 0;
      for (; x < destination->Width; x += 4)
      {
        *(_DWORD *)dstRow = 0xFFFF;
        dstRow += 8;
        *((_DWORD *)dstRow - 1) = 1431655765;
      }
      dst += destination->Pitch;
    }
  }
  else if (destination->Format == D3DFMT_R5G6B5)
  {
    unsigned char *src = Data;
    unsigned char *dst = &destination->Data[destination->Start];
    int y = 0;
    if (Height > 0)
    {
      do
      {
        int x = 0;
        unsigned char *dstRow = dst;
        if (Width > 0)
        {
          unsigned char *srcRow = src;
          do
          {
            unsigned char v22 = *srcRow++;
            x += 4;
            int v23 = v22 & 3;
            unsigned int v24 = v22 >> 2;
            *(_WORD *)dstRow = grays[v23];
            int v25 = v24 & 3;
            v24 >>= 2;
            *((_WORD *)dstRow + 1) = grays[v25];
            *((_WORD *)dstRow + 2) = grays[v24 & 3];
            *((_WORD *)dstRow + 3) = *(_WORD *)((char *)grays + (v24 & 0xFFFFFFFC));
            dstRow += 8;
          }
          while (x < Width);
        }
        for (; x < destination->Width; x++)
        {
          *(_WORD *)dstRow = 0;
          dstRow += 2;
        }
        src += Pitch;
        dst += destination->Pitch;
        y++;
      }
      while (y < Height);
    }
    for (; y < destination->Height; y++)
    {
      memset(dst, 0, 2 * destination->Width);
      dst += destination->Pitch;
    }
  }
  else
  {
    Logger.g->Panic("SShadowMask2::MakeBitmap: invalid destination bitmap");
  }
}

//----- (00466490) --------------------------------------------------------
void SShadowMask2::Save(SStream *is)
{
  is->WriteInt(Width);
  is->WriteInt(Height);
  is->WriteInt(Pitch);
  is->WriteInt(Size);
  is->Write(Data, Size);
}

