// 3dengine/spline.cpp
// Spline paths for camera/movement
// Decompiled from: gameSplit/sspline.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <math.h>
#include <d3dx9math.h>

#include "spline.h"
#include "gepard.h"
#include "terrain.h"
#include "stream.h"
#include "logger.h"

// Classes: SSpline
// Function count: 26

//----- (00459BC0) --------------------------------------------------------

SSpline::SSpline(SGepard *_Gepard, bool _middlespline, int _checkboxnr)

{
  this->Nodes.first = 0;
  this->Nodes.last = 0;
  this->Nodes.current = 0;
  this->Nodes.Closed = 0;
  this->Nodes.NumItems = 0;
  this->CheckBoxNr = _checkboxnr;
  this->FirstLeftX = -1.0;
  this->FirstLeftY = -1.0;
  this->FirstLeftZ = -1.0;
  this->FirstCenterX = -1.0;
  this->FirstCenterY = -1.0;
  this->FirstCenterZ = -1.0;
  this->FirstRightX = -1.0;
  this->FirstRightY = -1.0;
  this->FirstRightZ = -1.0;
  this->FirstLeftJoinedX = -1.0;
  this->FirstLeftJoinedY = -1.0;
  this->FirstLeftJoinedZ = -1.0;
  this->FirstRightJoinedX = -1.0;
  this->FirstRightJoinedY = -1.0;
  this->FirstRightJoinedZ = -1.0;
  this->LastLeftX = -1.0;
  this->LastLeftY = -1.0;
  this->LastLeftZ = -1.0;
  this->LastCenterX = -1.0;
  this->LastCenterY = -1.0;
  this->LastCenterZ = -1.0;
  this->LastRightX = -1.0;
  this->LastRightY = -1.0;
  this->LastRightZ = -1.0;
  this->LastLeftJoinedX = -1.0;
  this->LastLeftJoinedY = -1.0;
  this->LastLeftJoinedZ = -1.0;
  this->LastRightJoinedX = -1.0;
  this->LastRightJoinedY = -1.0;
  this->LastRightJoinedZ = -1.0;
  this->UAlign = -1;
  this->URatio = -1.0;
  this->VEndPos = -1.0;
  this->VStartPos = -1.0;
  this->JoinedVPos = -1.0;
  this->RiverLength = -1.0;
  this->RiverStripChain = 0;
  this->MiddleSpline = _middlespline;
  this->Closed = 0;
  this->Altitude = 0.0;
  this->Altitude2 = 0.5;
  this->VisibilityState = 1;
  this->Size = 1.0;
  srand((unsigned int)_time64(0));
  this->Seed = rand();
  this->Gepard = _Gepard;
  this->lpD3DDev = _Gepard->lpD3DDev;
  this->lastx = -1.0;
  this->lastz = -1.0;
  this->lastxkul = -1.0;
  this->lastzkul = -1.0;
  memset(this->lpLine, 0, sizeof(this->lpLine));
}

//----- (00459DD0) --------------------------------------------------------

SSpline::~SSpline()

{
  this->Nodes.DeleteAll();
}

// Helper: allocate and initialize a new SNode as a control point
static SNode* NewControlPointNode(float x, float y, float z, int heightFrom)
{
  SNode* node = (SNode*)operator new(sizeof(SNode));
  memset(node, 0, sizeof(SNode));
  node->ControlPoint = true;
  node->HeightFrom = heightFrom;
  node->x = x;
  node->y = y;
  node->z = z;
  return node;
}

// Helper: link node at end of chain
static void ChainAppend(SChain<SNode>* chain, SNode* node)
{
  SNode* last = chain->last;
  if (last)
  {
    last->next = node;
    node->prev = chain->last;
    chain->last = node;
    node->next = 0;
  }
  else
  {
    chain->first = node;
    chain->last = node;
    node->next = 0;
    chain->first->prev = 0;
  }
  ++chain->NumItems;
}

//----- (00459DF0) --------------------------------------------------------

void SSpline::AddNode(float x, float y, float z, bool forced)

{
  if ( !this->Closed || forced )
  {
    SNode* node = NewControlPointNode(x, y, z, this->Gepard->HeightFrom);
    ChainAppend(&this->Nodes, node);
  }
}

//----- (00459EA0) --------------------------------------------------------

void SSpline::Close()

{
  bool Closed; // al
  SNode *last; // ecx
  SNode *v4; // ecx
  SNode *first; // eax
  SNode *v6; // eax
  if ( !this->Closed )
  {
    Closed = this->Nodes.Closed;
    if ( Closed && !this->Nodes.last )
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    last = this->Nodes.last;
    if ( last )
    {
      if ( Closed && !this->Nodes.first )
        Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
      if ( this->Nodes.first != last )
      {
        v4 = (SNode *)operator new(sizeof(SNode));
        if ( this->Nodes.Closed && !this->Nodes.first )
          Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
        first = this->Nodes.first;
        memcpy(v4, first, sizeof(SNode));
        v4->Selected = 0;
        v6 = this->Nodes.last;
        if ( v6 )
        {
          v6->next = v4;
          v4->prev = this->Nodes.last;
          this->Nodes.last = v4;
          v4->next = 0;
          ++this->Nodes.NumItems;
          this->Closed = 1;
        }
        else
        {
          this->Nodes.first = v4;
          this->Nodes.last = v4;
          v4->next = 0;
          this->Nodes.first->prev = 0;
          ++this->Nodes.NumItems;
          this->Closed = 1;
        }
      }
    }
  }
}

//----- (00459F80) --------------------------------------------------------

void SSpline::CopySelectionFrom(SSpline *from)

{
  SNode *i; // eax
  SNode *current; // eax
  SNode *next; // eax
  SNode *v5; // eax
  SNode *first; // eax
  from->Nodes.current = from->Nodes.first;
  this->Nodes.current = this->Nodes.first;
  for ( i = from->Nodes.current; i; i = from->Nodes.current )
  {
    this->Nodes.current->Selected = i->Selected;
    current = from->Nodes.current;
    if ( from->Nodes.Closed )
    {
      if ( !current )
        goto LABEL_18;
      next = current->next;
      if ( !next )
        next = from->Nodes.first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    from->Nodes.current = next;
    v5 = this->Nodes.current;
    if ( this->Nodes.Closed )
    {
      if ( !v5 )
LABEL_18:
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = v5->next;
      if ( !first )
        first = this->Nodes.first;
    }
    else if ( v5 )
    {
      first = v5->next;
    }
    else
    {
      first = 0;
    }
    this->Nodes.current = first;
  }
}

//----- (0045A060) --------------------------------------------------------

void SSpline::DeleteSelectedNodes()

{
  SNode *first; // eax
  SNode *current; // ebx
  SNode *next; // edi
  SNode *prev; // ebx
  SNode *v6; // eax
  SNode *v7; // eax
  SNode *v8; // eax
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; first = this->Nodes.current )
  {
    if ( first->Selected )
    {
      current = this->Nodes.current;
      if ( current )
      {
        next = current->next;
        prev = current->prev;
        if ( next )
          next->prev = prev;
        if ( prev )
          prev->next = next;
        operator delete(this->Nodes.current);
        if ( next )
        {
          v6 = next;
        }
        else
        {
          this->Nodes.last = prev;
          v6 = 0;
        }
        this->Nodes.current = v6;
        if ( !prev )
          this->Nodes.first = next;
        --this->Nodes.NumItems;
      }
      if ( this->Closed )
      {
        if ( this->Nodes.Closed && !this->Nodes.first )
          Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
        if ( this->Nodes.current == this->Nodes.first )
          this->Closed = 0;
      }
    }
    else
    {
      v7 = this->Nodes.current;
      if ( this->Nodes.Closed )
      {
        if ( !v7 )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        v8 = v7->next;
        if ( !v8 )
          v8 = this->Nodes.first;
        this->Nodes.current = v8;
      }
      else if ( v7 )
      {
        this->Nodes.current = v7->next;
      }
      else
      {
        this->Nodes.current = 0;
      }
    }
  }
}

//----- (0045A140) --------------------------------------------------------

void SSpline::DeselectAllNode()

{
  SNode *first; // eax
  SNode *current; // eax
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; this->Nodes.current = first )
  {
    first->Selected = 0;
    current = this->Nodes.current;
    if ( this->Nodes.Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = current->next;
      if ( !first )
        first = this->Nodes.first;
    }
    else if ( current )
    {
      first = current->next;
    }
    else
    {
      first = 0;
    }
  }
}

//----- (0045A190) --------------------------------------------------------

void SSpline::DeselectNode(int index)

{
  SNode *first; // eax
  int i;
  first = this->Nodes.first;
  if ( !first )
    Logger.g->Panic("Any items was not found in the SChain  ( T& operator[](int index) )");
  for ( i = 0; i < index; ++i )
  {
    first = first->next;
    if ( !first )
      Logger.g->Panic("T& operator[](int index): Error in SChain: the given index i");
  }
  first->Selected = 0;
}

//----- (0045A1E0) --------------------------------------------------------

void SSpline::DrawLines()

{
  IDirect3DDevice9 *lpD3DDev; // eax
  SNode *current; // edi
  bool Closed; // dl
  SNode *v5; // eax
  SNode *v6; // ecx
  SNode *v7; // eax
  float Altitude; // xmm0_4
  SNode *v9; // eax
  SNode *next; // eax
  bool v11; // dl
  SNode *v12; // eax
  SNode *first; // ecx
  SNode *v14; // eax
  float Altitude2; // xmm0_4
  SNode *v16; // eax
  SNode *v17; // eax
  SNode *v18; // eax
  SNode *v19; // eax
  D3DXMATRIX move;
  memset(&move, 0, sizeof(move));
  move._11 = 1.0;
  move._22 = 1.0;
  move._33 = 1.0;
  move._44 = 1.0;
  this->Nodes.current = this->Nodes.first;
  lpD3DDev = this->lpD3DDev;
  lpD3DDev->SetTransform((D3DTRANSFORMSTATETYPE)256, &move);
  current = this->Nodes.current;
  if ( current )
  {
    do
    {
      Closed = this->Nodes.Closed;
      v5 = this->Nodes.current;
      if ( Closed )
      {
        if ( !v5 )
          goto LABEL_65;
        if ( v5->next )
        {
LABEL_9:
          this->lpLine[0].x = current->x;
          v7 = this->Nodes.current;
          if ( v7->ControlPoint )
          {
            if ( v7->HeightFrom == 1 )
              Altitude = this->Altitude;
            else
              Altitude = this->Altitude2;
          }
          else
          {
            Altitude = v7->y;
          }
          this->lpLine[0].y = Altitude;
          this->lpLine[0].z = v7->z;
          v9 = this->Nodes.current;
          if ( this->Nodes.Closed )
          {
            if ( !v9 )
              goto LABEL_65;
            next = v9->next;
            if ( !next )
              next = this->Nodes.first;
          }
          else if ( v9 )
          {
            next = v9->next;
          }
          else
          {
            next = 0;
          }
          this->lpLine[1].x = next->x;
          v11 = this->Nodes.Closed;
          v12 = this->Nodes.current;
          if ( v11 )
          {
            if ( !v12 )
              goto LABEL_65;
            first = v12->next;
            if ( !v12->next )
              first = this->Nodes.first;
          }
          else if ( v12 )
          {
            first = v12->next;
          }
          else
          {
            first = 0;
          }
          if ( first->ControlPoint )
          {
            if ( v11 )
            {
              if ( !v12 )
                goto LABEL_65;
              v14 = v12->next;
              if ( !v14 )
                v14 = this->Nodes.first;
            }
            else if ( v12 )
            {
              v14 = v12->next;
            }
            else
            {
              v14 = 0;
            }
            if ( v14->HeightFrom == 1 )
              Altitude2 = this->Altitude;
            else
              Altitude2 = this->Altitude2;
          }
          else
          {
            if ( v11 )
            {
              if ( !v12 )
                goto LABEL_65;
              v16 = v12->next;
              if ( !v16 )
                v16 = this->Nodes.first;
            }
            else if ( v12 )
            {
              v16 = v12->next;
            }
            else
            {
              v16 = 0;
            }
            Altitude2 = v16->y;
          }
          this->lpLine[1].y = Altitude2;
          v17 = this->Nodes.current;
          if ( this->Nodes.Closed )
          {
            if ( !v17 )
LABEL_65:
              Logger.g->Panic("GetNext: In a closed chain <current> is NULL");
            v18 = v17->next;
            if ( !v18 )
              v18 = this->Nodes.first;
          }
          else if ( v17 )
          {
            v18 = v17->next;
          }
          else
          {
            v18 = 0;
          }
          this->lpLine[1].z = v18->z;
          this->lpD3DDev->SetFVF(2u);
          this->lpD3DDev->DrawPrimitiveUP(D3DPT_LINELIST, 1u, this->lpLine, 12u);
          Closed = this->Nodes.Closed;
          v5 = this->Nodes.current;
          goto LABEL_55;
        }
        v6 = this->Nodes.first;
      }
      else
      {
        if ( !v5 )
          goto LABEL_55;
        v6 = v5->next;
      }
      if ( v6 )
        goto LABEL_9;
LABEL_55:
      if ( Closed )
      {
        if ( !v5 )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        v19 = v5->next;
        if ( !v19 )
          v19 = this->Nodes.first;
      }
      else if ( v5 )
      {
        v19 = v5->next;
      }
      else
      {
        v19 = 0;
      }
      current = v19;
      this->Nodes.current = v19;
    }
    while ( v19 );
  }
}

//----- (0045A460) --------------------------------------------------------

void SSpline::DrawNodes(bool top, SHillRing *_hillring, SSplineRing *_level)

{
  int v5;
  SNode *first; // eax
  float _magassag; // xmm0_4
  SNode *current; // edx
  SNode *v9; // ecx
  bool ControlPoint; // al
  D3DCOLORVALUE v11; // xmm0
  float y; // xmm1_4
  SNode *v13; // eax

  // Color constants (original values from SSE constants, reconstructed)
  D3DCOLORVALUE colorControlPointHover = {1.0f, 1.0f, 1.0f, 1.0f};
  D3DCOLORVALUE colorSelected = {1.0f, 1.0f, 0.0f, 1.0f};
  D3DCOLORVALUE colorControlPoint = {0.0f, 1.0f, 0.0f, 1.0f};
  D3DCOLORVALUE colorNormal = {1.0f, 0.0f, 0.0f, 1.0f};

  v5 = 0;
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; this->Nodes.current = first )
  {
    if ( first->ControlPoint )
    {
      if ( first->HeightFrom == 1 )
        _magassag = this->Altitude;
      else
        _magassag = this->Altitude2;
    }
    else
    {
      _magassag = first->y;
    }
    this->Gepard->CollectNode(_hillring, _level, first, v5, _magassag);
    current = this->Nodes.current;
    if ( this->Nodes.Closed && !this->Nodes.last )
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    if ( current != this->Nodes.last || !this->Closed )
    {
      v9 = this->Nodes.current;
      if ( top && current->ControlPoint )
      {
        this->Gepard->DrawNode(0, current->x, current->y + 0.02f, current->z, colorControlPointHover);
        v9 = this->Nodes.current;
      }
      ControlPoint = v9->ControlPoint;
      if ( v9->Selected )
      {
        v11 = colorSelected;
      }
      else if ( ControlPoint )
      {
        v11 = colorControlPoint;
      }
      else
      {
        v11 = colorNormal;
      }
      if ( ControlPoint )
      {
        if ( v9->HeightFrom == 1 )
          y = this->Altitude;
        else
          y = this->Altitude2;
        this->Gepard->DrawNode(0, v9->x, y, v9->z, v11);
      }
      else
      {
        this->Gepard->DrawNode(0, v9->x, v9->y, v9->z, v11);
      }
    }
    v13 = this->Nodes.current;
    if ( this->Nodes.Closed )
    {
      if ( !v13 )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = v13->next;
      if ( !first )
        first = this->Nodes.first;
    }
    else if ( v13 )
    {
      first = v13->next;
    }
    else
    {
      first = 0;
    }
    ++v5;
  }
}

//----- (0045A610) --------------------------------------------------------

void SSpline::GetCenter(float *_cx, float *_cz)

{
  *_cx = 20.0;
  *_cz = 20.0;
}

//----- (0045A750) --------------------------------------------------------

bool SSpline::IsAxleSame(float _x1, float _z1, float _x2, float _z2)

{
  return fabsf(_x1 - _x2) < 0.001f
      || fabsf(_z1 - _z2) < 0.001f;
}

//----- (0045A7B0) --------------------------------------------------------

bool SSpline::IsFloatNear(float f1, float f2)

{
  float v3; // xmm1_4
  v3 = fabsf(f1 - f2);
  return v3 < 0.02f;
}

//----- (0045A7E0) --------------------------------------------------------

bool SSpline::IsSectionSame(float _x1, float _z1, float _x2, float _z2)

{
  float v6; // xmm0_4
  float v7; // xmm0_4
  bool v9;
  bool v10;
  int v12;
  float v13; // xmm3_4
  float v14; // xmm4_4
  int v15;
  float v17; // xmm0_4
  float v18; // xmm0_4
  float v19; // xmm0_4
  float v20; // xmm0_4
  int v21;
  float v22; // xmm4_4
  int v23;
  bool v24; // cc
  double X;
  double v28;
  double v29;
  double v30;
  double v31;
  if ( fabsf(_z1 - _z2) >= 0.001f
    || fabsf((float)floor(_z1) - _z1) >= 0.001f
    || fabsf((float)floor(_z2) - _z2) >= 0.001f )
  {
LABEL_17:
    v13 = _x2;
    goto LABEL_18;
  }
  X = _x1;
  v30 = floor(_x1);
  v6 = (float)v30;
  v7 = fabsf(v6 - _x1);
  v9 = v7 > 0.001f;
  v10 = 0.001f == v7;
  v12 = !v9 && !v10;
  v13 = _x2;
  v14 = (float)floor(_x2);
  if ( fabsf(v14 - _x2) < 0.001f )
    v12 = 2;
  v15 = v12 - 1;
  if ( !v15 )
  {
    if ( _x2 <= _x1 )
    {
      v29 = ceil(X);
      v18 = (float)ceil((double)_x2);
      if ( fabsf((float)v29 - v18) < 0.001f )
        return 1;
      goto LABEL_17;
    }
    goto LABEL_14;
  }
  if ( v15 == 1 )
  {
    if ( _x2 > _x1 )
    {
      v28 = ceil(X);
      v17 = (float)ceil((double)_x2);
      if ( fabsf((float)v28 - v17) < 0.001f )
        return 1;
      goto LABEL_17;
    }
LABEL_14:
    if ( fabsf((float)v30 - v14) < 0.001f )
      return 1;
    goto LABEL_17;
  }
  if ( fabsf((float)v30 - v14) < 0.001f )
    return 1;
LABEL_18:
  if ( fabsf(_x1 - v13) >= 0.001f
    || fabsf((float)floor(_x1) - _x1) >= 0.001f
    || fabsf((float)floor(_x2) - _x2) >= 0.001f )
  {
    return 0;
  }
  v31 = floor(_z1);
  v19 = (float)v31;
  v20 = fabsf(v19 - _z1);
  v21 = v20 < 0.001f;
  v22 = (float)floor(_z2);
  if ( fabsf(v22 - _z2) < 0.001f )
    v21 = 2;
  v23 = v21 - 1;
  if ( v23 )
  {
    if ( v23 == 1 && _z2 > _z1 )
      goto LABEL_29;
LABEL_25:
    v24 = fabsf((float)v31 - v22) >= 0.001f;
    return !v24;
  }
  if ( _z2 > _z1 )
    goto LABEL_25;
LABEL_29:
  {
    float cz1 = (float)ceil(_z1);
    float cz2 = (float)ceil(_z2);
    v24 = fabsf(cz1 - cz2) >= 0.001f;
    return !v24;
  }
}

//----- (0045AD00) --------------------------------------------------------

int SSpline::IsSegmentTouch(float x, float z, int *xaxle, int *zaxle)

{
  int v5;
  int v6;
  float v7; // xmm2_4
  v5 = (int)(x + 0.5f);
  v6 = (int)(z + 0.5f);
  v7 = fabsf((float)v5 - x);
  if ( fabsf((float)v6 - z) >= 0.02f )
  {
    if ( v7 >= 0.02f )
    {
      return 0;
    }
    else
    {
      *xaxle = v5;
      return 1;
    }
  }
  else if ( v7 >= 0.02f )
  {
    *zaxle = v6;
    return 2;
  }
  else
  {
    *xaxle = v5;
    *zaxle = v6;
    return 3;
  }
}

//----- (0045ADB0) --------------------------------------------------------

void SSpline::KillSubNodes()

{
  SNode *first; // eax
  SNode *v3; // ebx
  SNode *v4; // edi
  SNode *prev; // ebx
  SNode *v6; // eax
  SNode *current; // eax
  SNode *next; // eax
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; first = this->Nodes.current )
  {
    if ( first->ControlPoint )
    {
      current = this->Nodes.current;
      if ( this->Nodes.Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = current->next;
        if ( !next )
          next = this->Nodes.first;
        this->Nodes.current = next;
      }
      else if ( current )
      {
        this->Nodes.current = current->next;
      }
      else
      {
        this->Nodes.current = 0;
      }
    }
    else
    {
      v3 = this->Nodes.current;
      if ( v3 )
      {
        v4 = v3->next;
        prev = v3->prev;
        if ( v4 )
          v4->prev = prev;
        if ( prev )
          prev->next = v4;
        operator delete(this->Nodes.current);
        if ( v4 )
        {
          v6 = v4;
        }
        else
        {
          this->Nodes.last = prev;
          v6 = 0;
        }
        this->Nodes.current = v6;
        if ( !prev )
          this->Nodes.first = v4;
        --this->Nodes.NumItems;
      }
    }
  }
}

//----- (0045AE60) --------------------------------------------------------

void SSpline::MoveNode(SNode *_node, float x, float y, float z)

{
  bool Closed; // al
  bool v6; // al
  if ( this->Closed )
  {
    Closed = this->Nodes.Closed;
    if ( Closed && !this->Nodes.last )
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    if ( _node == this->Nodes.last )
    {
      if ( Closed && !this->Nodes.first )
        Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
      this->Nodes.first->x = x;
      if ( this->Nodes.Closed && !this->Nodes.first )
        Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
      this->Nodes.first->y = y;
      if ( this->Nodes.Closed && !this->Nodes.first )
        Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
      this->Nodes.first->z = z;
    }
    v6 = this->Nodes.Closed;
    if ( v6 && !this->Nodes.first )
      Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
    if ( _node == this->Nodes.first )
    {
      if ( v6 && !this->Nodes.last )
        Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
      this->Nodes.last->x = x;
      if ( this->Nodes.Closed && !this->Nodes.last )
        Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
      this->Nodes.last->y = y;
      if ( this->Nodes.Closed && !this->Nodes.last )
        Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
      this->Nodes.last->z = z;
    }
  }
  _node->x = x;
  _node->y = y;
  _node->z = z;
}

//----- (0045AFD0) --------------------------------------------------------

void SSpline::RecalculateControlPoints()

{
  SNode *first; // edi
  SNode *current; // eax
  SNode *next; // eax
  first = this->Nodes.first;
  this->Nodes.current = first;
  if ( first )
  {
    do
    {
      if ( first->ControlPoint )
        first->y = (float)this->Gepard->Terrain->GetHeight(first->x, first->z);
      current = this->Nodes.current;
      if ( this->Nodes.Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = current->next;
        if ( !next )
          next = this->Nodes.first;
      }
      else if ( current )
      {
        next = current->next;
      }
      else
      {
        next = 0;
      }
      first = next;
      this->Nodes.current = next;
    }
    while ( next );
  }
}

//----- (0045B050) --------------------------------------------------------

void SSpline::Reduce(float ratio)

{
  SNode *first; // eax
  SNode *current; // eax
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; this->Nodes.current = first )
  {
    first->x = (float)((float)(20.0f - first->x) * ratio) + first->x;
    this->Nodes.current->z = (float)((float)(20.0f - first->z) * ratio) + first->z;
    current = this->Nodes.current;
    if ( this->Nodes.Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = current->next;
      if ( !first )
        first = this->Nodes.first;
    }
    else if ( current )
    {
      first = current->next;
    }
    else
    {
      first = 0;
    }
  }
}

//----- (0045B0F0) --------------------------------------------------------

void SSpline::RefreshSubNodes()

{
  SChain<SNode> *p_Nodes; // esi
  SNode *current; // eax
  bool v4;
  SNode *v5; // eax
  SNode *next; // edi
  SNode *prev; // eax
  SNode *v8; // ecx
  SNode *v9; // eax
  SNode *first; // ecx
  bool Closed; // al
  float v12; // xmm2_4
  float v13; // xmm1_4
  int v14;
  int v15;
  float v16; // xmm2_4
  int v17;
  bool v18; // dl
  SNode *v19; // edi
  SNode *v20; // eax
  SNode *v21; // ecx
  float v22; // xmm0_4
  float v23; // xmm4_4
  float v24; // xmm5_4
  float v25; // xmm0_4
  float v26; // xmm2_4
  float v27; // xmm3_4
  SNode *v28; // eax
  SNode *Next2; // eax
  float v31; // xmm0_4
  float v32; // xmm1_4
  SNode *last; // eax
  SNode *v34; // eax
  SNode *LastButOne; // eax
  float z; // xmm1_4
  float x; // xmm2_4
  int v38;
  int v39;
  float v40; // xmm3_4
  float v41; // xmm0_4
  int v42;
  float Height; // st7
  float _z1; // xmm2_4
  int v45;
  SNode *v46; // eax
  bool IsSectionSame_result; // al
  SNode *v48; // edi
  bool v49; // al
  SNode *v50; // ecx
  SNode *v51; // eax
  bool v52; // al
  bool v53; // dl
  bool v54; // dh
  SNode *v55; // ecx
  SNode *v56; // eax
  float v57; // xmm0_4
  bool v58; // al
  float v59; // xmm0_4
  SNode *v60; // ecx
  SNode *v61; // eax
  SNode *v62; // edx
  SNode *v63; // ecx
  SNode *v65;
  float v66;
  float _x1;
  bool v68;
  float v69;
  float v70;
  SNode *v71;
  SNode *v72;
  int v73;
  bool v74;
  float v76;
  float v77;
  float v78;
  float v79;
  int v80 = -1;
  SNode *v81;
  float v82;
  SNode *v83;
  int v84;
  float v85;
  int v86;
  int v87;
  int v88;
  int v89;
  int v90 = -1;
  float v91;
  D3DXVECTOR2 pos1;
  D3DXVECTOR2 pos2;
  D3DXVECTOR2 tan1;
  D3DXVECTOR2 tan2;
  D3DXVECTOR2 outPt;
  SChain<SNode> *i;

  p_Nodes = &this->Nodes;
  this->Nodes.current = this->Nodes.first;
  current = this->Nodes.current;
  for ( i = &this->Nodes; current; current = this->Nodes.current )
  {
    v4 = !current->ControlPoint;
    v5 = p_Nodes->current;
    if ( v4 )
    {
      if ( v5 )
      {
        next = v5->next;
        prev = v5->prev;
        v71 = prev;
        if ( next )
          next->prev = prev;
        if ( prev )
          prev->next = next;
        operator delete(p_Nodes->current);
        if ( next )
        {
          v8 = next;
        }
        else
        {
          p_Nodes->last = v71;
          v8 = 0;
        }
        p_Nodes->current = v8;
        if ( !v71 )
          p_Nodes->first = next;
        --p_Nodes->NumItems;
      }
    }
    else if ( p_Nodes->Closed )
    {
      if ( !v5 )
LABEL_148:
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      v9 = v5->next;
      if ( v9 )
        p_Nodes->current = v9;
      else
        p_Nodes->current = p_Nodes->first;
    }
    else if ( v5 )
    {
      p_Nodes->current = v5->next;
    }
    else
    {
      p_Nodes->current = 0;
    }
  }
  if ( this->Nodes.NumItems >= 2 )
  {
    first = p_Nodes->first;
    p_Nodes->current = p_Nodes->first;
    Closed = this->Nodes.Closed;
    if ( Closed && !first )
      Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
    v12 = first->x;
    if ( Closed && !first )
      Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
    v13 = first->z;
    v14 = (int)(v12 + 0.5f);
    v88 = v14;
    v15 = (int)(v13 + 0.5f);
    v89 = v15;
    v16 = fabsf((float)v14 - v12);
    if ( fabsf((float)v15 - v13) >= 0.02f )
    {
      v17 = v80;
      if ( v16 < 0.02f )
        v17 = v14;
      v14 = v17;
      v15 = v90;
    }
    else
    {
      if ( v16 < 0.02f )
        goto LABEL_35;
      v14 = v80;
    }
    v89 = v15;
    v88 = v14;
LABEL_35:
    if ( v14 == -1 || v15 == -1 )
      Logger.g->Panic("lastxaxle or lastzaxle is NULL. CP is how can be NULL?!");
    v81 = 0;
    v83 = 0;
    v84 = v90;
    v87 = v90;
    while ( 1 )
    {
      v18 = p_Nodes->Closed;
      v68 = v18;
      if ( v18 )
      {
        if ( !first )
LABEL_150:
          Logger.g->Panic("GetNext: In a closed chain <current> is NULL");
        v19 = first->next;
        if ( first->next )
          v20 = first->next;
        else
          v20 = p_Nodes->first;
      }
      else
      {
        if ( !first )
          return;
        v20 = first->next;
        v19 = first->next;
      }
      if ( !v20 )
        return;
      if ( !v83 )
      {
        if ( !v18 || v19 )
          v83 = v19;
        else
          v83 = p_Nodes->first;
      }
      v72 = this->Nodes.current;
      v76 = v72->x;
      v78 = v72->z;
      if ( !v18 || v19 )
        v65 = v19;
      else
        v65 = p_Nodes->first;
      v21 = v19;
      p_Nodes = i;
      v85 = v65->x;
      if ( v18 && !v19 )
        v21 = i->first;
      v69 = v21->z;
      // Compute distance between current and next node
      {
        float dx = v85 - v76;
        float dz = v69 - v78;
        v22 = sqrtf(dx * dx + dz * dz);
      }
      v23 = v76;
      v24 = v78;
      v25 = fabsf(v22);
      v26 = v85;
      v27 = v69;
      pos1.x = v76;
      pos1.y = v78;
      pos2.x = v85;
      pos2.y = v69;
      v73 = (int)((float)(v25 * 50.0f) * 1.5707963f);
      if ( v68 )
      {
        if ( v19 && v19->next )
          goto LABEL_66;
        v28 = p_Nodes->first;
        if ( !v19 )
          v28 = v28->next;
      }
      else
      {
        if ( !v19 )
          goto LABEL_67;
        v28 = v19->next;
      }
      if ( !v28 )
      {
LABEL_67:
        if ( this->Closed && v83 )
        {
          v66 = v83->x;
          v69 = v83->z;
        }
        else
        {
          v66 = v85;
        }
        goto LABEL_71;
      }
LABEL_66:
      v66 = p_Nodes->GetNext2()->x;
      Next2 = p_Nodes->GetNext2();
      v24 = pos1.y;
      v23 = pos1.x;
      v27 = pos2.y;
      v26 = pos2.x;
      v69 = Next2->z;
LABEL_71:
      if ( v81 )
      {
        v31 = v81->x;
        v32 = v81->z;
      }
      else
      {
        if ( !this->Closed )
          goto LABEL_79;
        last = this->Nodes.last;
        if ( !last )
          goto LABEL_79;
        v34 = last->prev;
        if ( this->Nodes.Closed && !v34 )
          Logger.g->Panic("GetLastButOne: In a closed chain <last->prev> is NULL");
        if ( v34 )
        {
          v82 = p_Nodes->GetLastButOne()->x;
          LastButOne = p_Nodes->GetLastButOne();
          v24 = pos1.y;
          v23 = pos1.x;
          v27 = pos2.y;
          v32 = LastButOne->z;
          v26 = pos2.x;
          v31 = v82;
        }
        else
        {
LABEL_79:
          v31 = v23;
          v32 = v24;
        }
      }
      v81 = this->Nodes.current;
      tan1.x = v26 - v31;
      tan1.y = v27 - v32;
      tan2.x = v66 - v23;
      tan2.y = v69 - v24;
      v86 = 0;
      if ( v73 >= 0 )
      {
        z = v78;
        x = v76;
        while ( 1 )
        {
          v38 = (int)(x + 0.5f);
          v39 = (int)(z + 0.5f);
          v40 = fabsf((float)v38 - x);
          v41 = fabsf((float)v39 - z);
          if ( v40 >= 0.02f )
          {
            if ( v41 < 0.02f )
            {
              v38 = v87;
              v42 = 2;
              v84 = (int)(z + 0.5f);
              goto LABEL_90;
            }
          }
          else if ( v41 < 0.02f )
          {
            v84 = (int)(z + 0.5f);
            v42 = 3;
            goto LABEL_89;
          }
          if ( v40 < 0.02f )
            break;
LABEL_137:
          D3DXVec2Hermite(&outPt, &pos1, &tan1, &pos2, &tan2, (float)v86 / (float)v73);
          x = outPt.x;
          z = outPt.y;
          v76 = outPt.x;
          v78 = outPt.y;
          if ( ++v86 > v73 )
            goto LABEL_138;
        }
        v39 = v84;
        v42 = 1;
LABEL_89:
        v87 = (int)(x + 0.5f);
LABEL_90:
        if ( v88 == v38 && v89 == v39 )
          goto LABEL_137;
        _x1 = (float)v38;
        Height = (float)this->Gepard->Terrain->GetHeight(x, z);
        _z1 = (float)v84;
        v70 = (float)v84;
        v45 = v42 - 1;
        if ( v45 )
        {
          if ( v45 == 1 )
            _x1 = v76;
        }
        else
        {
          _z1 = v78;
          v70 = v78;
        }
        v46 = this->Nodes.current;
        v79 = v46->x;
        v77 = v46->z;
        IsSectionSame_result = this->IsSectionSame(_x1, _z1, v79, v77);
        v48 = p_Nodes->current;
        v74 = IsSectionSame_result;
        v49 = p_Nodes->Closed;
        if ( v49 )
        {
          if ( !v48 )
            goto LABEL_150;
          v50 = v48->next;
          if ( !v48->next )
            v50 = p_Nodes->first;
LABEL_105:
          if ( v49 )
          {
            if ( !v48 )
              goto LABEL_150;
            v51 = v48->next;
            if ( !v48->next )
              v51 = p_Nodes->first;
LABEL_110:
            v52 = this->IsSectionSame(_x1, v70, v51->x, v50->z);
            v53 = p_Nodes->Closed;
            v54 = v52;
            if ( v53 )
            {
              if ( !v48 )
                goto LABEL_150;
              v55 = v48->next;
              if ( !v48->next )
                v55 = p_Nodes->first;
LABEL_119:
              if ( v53 )
              {
                if ( !v48 )
                  goto LABEL_150;
                v56 = v48->next;
                if ( !v48->next )
                  v56 = p_Nodes->first;
LABEL_124:
                v57 = fabsf(v79 - v56->x);
                if ( v57 >= 0.001f )
                {
                  v59 = fabsf(v77 - v55->z);
                  v58 = v59 < 0.001f;
                }
                else
                {
                  v58 = 1;
                }
                if ( v74 || v54 && !v58 )
                  goto LABEL_137;
                v60 = (SNode *)operator new(sizeof(SNode));
                memset(v60, 0, sizeof(SNode));
                v61 = p_Nodes->last;
                if ( v61 )
                {
                  v62 = p_Nodes->current;
                  if ( v62 == v61 )
                  {
                    v61->next = v60;
                    v60->prev = p_Nodes->last;
                    p_Nodes->last = v60;
                    goto LABEL_135;
                  }
                  v60->prev = v62;
                  v60->next = p_Nodes->current->next;
                  p_Nodes->current->next = v60;
                  v60->next->prev = v60;
                }
                else
                {
                  p_Nodes->last = v60;
                  p_Nodes->first = v60;
                  v60->prev = 0;
LABEL_135:
                  v60->next = 0;
                }
                ++p_Nodes->NumItems;
                p_Nodes->current = v60;
                v60->x = _x1;
                v91 = Height;
                v60->y = v91;
                v88 = v87;
                v60->z = v70;
                v89 = v84;
                goto LABEL_137;
              }
            }
            else
            {
              if ( !v48 )
              {
                v55 = 0;
                goto LABEL_119;
              }
              v55 = v48->next;
            }
            if ( v48 )
              v56 = v48->next;
            else
              v56 = 0;
            goto LABEL_124;
          }
        }
        else
        {
          if ( !v48 )
          {
            v50 = 0;
            goto LABEL_105;
          }
          v50 = v48->next;
        }
        if ( v48 )
          v51 = v48->next;
        else
          v51 = 0;
        goto LABEL_110;
      }
LABEL_138:
      v63 = p_Nodes->current;
      if ( p_Nodes->Closed )
      {
        if ( !v63 )
          goto LABEL_148;
        first = v63->next;
        if ( first )
        {
LABEL_145:
          p_Nodes->current = first;
        }
        else
        {
          first = p_Nodes->first;
          p_Nodes->current = p_Nodes->first;
        }
      }
      else
      {
        if ( !v63 )
        {
          first = 0;
          goto LABEL_145;
        }
        first = v63->next;
        p_Nodes->current = first;
      }
    }
  }
}

//----- (0045B9B0) --------------------------------------------------------

void SSpline::ReplaceNodesFrom(SSpline *from, SSpline *from2)

{
  SChain<SNode> *p_Nodes; // esi
  SNode *current; // eax
  SNode *v7; // eax
  SNode *next; // eax
  SNode *v9; // edx
  SNode *v10; // eax
  int v11;
  SNode *v12; // ecx
  float v13; // xmm3_4
  SNode *v14; // eax
  float Altitude2; // xmm1_4
  SNode *v18; // eax
  SNode *first; // eax
  SNode *v20; // eax
  SNode *v21; // eax
  SNode *v23; // eax
  bool Closed; // dl
  SNode *v25; // eax
  SNode *v26; // ecx
  SNode *v27; // eax
  float x; // xmm2_4
  float z; // xmm3_4
  SNode *v30; // eax
  SNode *prev; // ecx
  SNode *v32; // eax
  bool v33; // dl
  SNode *v34; // eax
  SNode *v35; // ecx
  float v36; // xmm1_4
  SNode *v37; // ecx
  float v38; // xmm0_4
  SNode *v39_next; // eax
  SNode *v40; // ecx
  SNode *v41; // ecx
  SNode *v42_nn; // ecx
  float v43; // xmm0_4
  float v44; // xmm0_4
  float v45; // xmm2_4
  float j; // xmm0_4
  SNode *v47; // eax
  SNode *v48; // eax
  SNode *v49; // ecx
  SNode *v50; // eax
  SNode *v51; // edx
  float v52; // xmm0_4
  float v53; // xmm0_4
  SNode *v54; // eax
  float v57;
  SNode *v58;
  float v59;
  float v60;
  float v61;
  SNode *v62;
  float v63;
  float v64;
  float v65;
  float v66;
  float v67;
  float v68;
  float v69;
  float y;
  float v71;
  float v72;
  D3DXVECTOR2 pos1;
  D3DXVECTOR2 pos2;
  D3DXVECTOR2 tan1;
  D3DXVECTOR2 tan2;
  D3DXVECTOR2 outPt;

  p_Nodes = &this->Nodes;
  this->Nodes.DeleteAll();
  from->Nodes.current = from->Nodes.first;
  from2->Nodes.current = from2->Nodes.first;
  srand(this->Seed);
  current = from->Nodes.current;
  if ( current )
  {
    for ( ; from2->Nodes.current; )
    {
      do
      {
        if ( current->ControlPoint )
          break;
        v7 = from->Nodes.current;
        if ( from->Nodes.Closed )
        {
          if ( !v7 )
            goto LABEL_149;
          current = v7->next;
          if ( !current )
            current = from->Nodes.first;
        }
        else
        {
          current = v7 ? v7->next : 0;
        }
        from->Nodes.current = current;
      }
      while ( current );
      next = from2->Nodes.current;
      if ( next )
      {
        v9 = from2->Nodes.current;
        do
        {
          next = v9;
          if ( v9->ControlPoint )
            break;
          v10 = from2->Nodes.current;
          if ( from2->Nodes.Closed )
          {
            if ( !v10 )
              goto LABEL_149;
            next = v10->next;
            if ( !next )
              next = from2->Nodes.first;
          }
          else
          {
            next = v10 ? v10->next : 0;
          }
          from2->Nodes.current = next;
          v9 = next;
        }
        while ( next );
      }
      if ( from->Nodes.current && next )
      {
        v11 = rand();
        v12 = from2->Nodes.current;
        v13 = (float)((float)((float)((float)((float)v11 / 32767.0f) * 0.0f) + (float)((float)((float)v11 / 32767.0f) * 0.0f))
                    - 0.0f)
            + this->Size;
        v14 = from->Nodes.current;
        Altitude2 = this->Altitude2;
        v67 = (float)((float)((float)(v12->x - v14->x) * Altitude2) * v13) + v14->x;
        v65 = (float)((float)((float)(v12->y + from2->Altitude) - v14->y) * Altitude2) + v14->y;
        v63 = (float)((float)((float)(v12->z - v14->z) * Altitude2) * v13) + v14->z;

        SNode* newNode = NewControlPointNode(v67, v65, v63, this->Gepard->HeightFrom);
        ChainAppend(p_Nodes, newNode);

        v18 = from->Nodes.current;
        if ( from->Nodes.Closed )
        {
          if ( !v18 )
            goto LABEL_149;
          first = v18->next;
          if ( !first )
            first = from->Nodes.first;
        }
        else if ( v18 )
        {
          first = v18->next;
        }
        else
        {
          first = 0;
        }
        from->Nodes.current = first;
        v20 = from2->Nodes.current;
        if ( from2->Nodes.Closed )
        {
          if ( !v20 )
LABEL_149:
            Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
          v21 = v20->next;
          if ( !v21 )
            v21 = from2->Nodes.first;
          from2->Nodes.current = v21;
        }
        else if ( v20 )
        {
          from2->Nodes.current = v20->next;
        }
        else
        {
          from2->Nodes.current = 0;
        }
      }
      current = from->Nodes.current;
      if ( !current )
        break;
    }
  }
  v58 = 0;
  v23 = p_Nodes->first;
  p_Nodes->current = p_Nodes->first;
  v62 = 0;
  if ( this->Nodes.current )
  {
    do
    {
      Closed = p_Nodes->Closed;
      if ( Closed )
      {
        if ( !v23 )
          goto LABEL_151;
        v25 = v23->next;
        if ( v25 )
          v26 = v25;
        else
          v26 = p_Nodes->first;
      }
      else
      {
        if ( !v23 )
          return;
        v26 = v23->next;
        v25 = v23->next;
      }
      if ( !v26 )
        return;
      if ( !v58 )
      {
        if ( Closed && !v25 )
          v25 = p_Nodes->first;
        v58 = v25;
      }
      v27 = this->Nodes.current;
      x = v27->x;
      pos1.x = x;
      z = v27->z;
      v68 = x;
      v66 = z;
      pos1.y = z;
      if ( v62 )
      {
        v57 = v62->x;
        v59 = v62->z;
        goto LABEL_77;
      }
      if ( this->Closed )
      {
        v30 = this->Nodes.last;
        if ( v30 )
        {
          prev = v30->prev;
          if ( Closed && !prev )
LABEL_150:
            Logger.g->Panic("GetLastButOne: In a closed chain <last->prev> is NULL");
        }
        else
        {
          prev = 0;
        }
        v57 = prev->x;
        if ( !v30 )
        {
          v32 = 0;
LABEL_75:
          v59 = v32->z;
          goto LABEL_77;
        }
        v32 = v30->prev;
        if ( !this->Nodes.Closed )
          goto LABEL_75;
        if ( !v32 )
          goto LABEL_150;
        v59 = v32->z;
      }
      else
      {
        v57 = x;
        v59 = z;
      }
LABEL_77:
      v33 = p_Nodes->Closed;
      v34 = p_Nodes->current;
      if ( v33 )
      {
        if ( !v34 )
          goto LABEL_151;
        v35 = v34->next;
        if ( !v34->next )
          v35 = p_Nodes->first;
      }
      else if ( v34 )
      {
        v35 = v34->next;
      }
      else
      {
        v35 = 0;
      }
      v36 = v35->x;
      pos2.x = v36;
      if ( v33 )
      {
        if ( !v34 )
LABEL_151:
          Logger.g->Panic("GetNext: In a closed chain <current> is NULL");
        v37 = v34->next;
        if ( !v34->next )
          v37 = p_Nodes->first;
      }
      else if ( v34 )
      {
        v37 = v34->next;
      }
      else
      {
        v37 = 0;
      }
      v38 = v37->z;
      v69 = v38;
      pos2.y = v38;
      if ( v33 )
      {
        if ( !v34 )
          Logger.g->Panic("GetNext2: In a closed chain <current> is NULL");
        v39_next = v34->next;
        if ( v39_next && v39_next->next )
          goto LABEL_101;
        v40 = p_Nodes->first;
        if ( !v39_next )
          v40 = v40->next;
      }
      else
      {
        if ( !v34 || (v39_next = v34->next) == 0 )
        {
LABEL_118:
          if ( this->Closed && v58 )
          {
            v60 = v58->x;
            v61 = v58->z;
          }
          else
          {
            v60 = v36;
            v61 = v38;
          }
          goto LABEL_122;
        }
        v40 = v39_next->next;
      }
      if ( !v40 )
        goto LABEL_118;
LABEL_101:
      if ( v33 )
      {
        if ( !v39_next || (v41 = v39_next->next) == 0 )
        {
          v41 = p_Nodes->first;
          if ( !v39_next )
            v41 = v41->next;
        }
      }
      else if ( v39_next )
      {
        v41 = v39_next->next;
      }
      else
      {
        v41 = 0;
      }
      v60 = v41->x;
      if ( v33 )
      {
        if ( !v39_next || (v42_nn = v39_next->next) == 0 )
        {
          v42_nn = p_Nodes->first;
          if ( !v39_next )
          {
            v61 = v42_nn->next->z;
            goto LABEL_122;
          }
        }
      }
      else
      {
        if ( v39_next )
        {
          v61 = v39_next->next->z;
          goto LABEL_122;
        }
        v42_nn = 0;
      }
      v61 = v42_nn->z;
LABEL_122:
      v62 = this->Nodes.current;
      // Compute distance between current and next node
      {
        float dx = pos2.x - pos1.x;
        float dz = pos2.y - pos1.y;
        v43 = sqrtf(dx * dx + dz * dz);
      }
      v44 = fabsf(v43);
      v72 = 1.0f / (float)(int)(v44 + 0.5f);
      v45 = v72;
      v64 = v72;
      if ( (float)((float)(1.0f - v72) + 0.001f) >= v72 )
      {
        for ( j = v69; ; j = pos2.y )
        {
          tan1.x = v36 - v57;
          tan1.y = j - v59;
          tan2.x = v60 - v68;
          tan2.y = v61 - v66;
          D3DXVec2Hermite(&outPt, &pos1, &tan1, &pos2, &tan2, v45);
          v47 = p_Nodes->current;
          y = this->Nodes.current->y;
          if ( p_Nodes->Closed )
          {
            if ( !v47 )
              goto LABEL_151;
            v48 = v47->next;
            if ( !v48 )
              v48 = p_Nodes->first;
          }
          else if ( v47 )
          {
            v48 = v47->next;
          }
          else
          {
            v48 = 0;
          }
          v71 = v48->y;
          v49 = (SNode *)operator new(sizeof(SNode));
          memset(v49, 0, sizeof(SNode));
          v50 = p_Nodes->last;
          if ( v50 )
          {
            v51 = p_Nodes->current;
            if ( v51 != v50 )
            {
              v49->prev = v51;
              v49->next = p_Nodes->current->next;
              p_Nodes->current->next = v49;
              v49->next->prev = v49;
              goto LABEL_137;
            }
            v50->next = v49;
            v49->prev = p_Nodes->last;
            p_Nodes->last = v49;
          }
          else
          {
            p_Nodes->last = v49;
            p_Nodes->first = v49;
            v49->prev = 0;
          }
          v49->next = 0;
LABEL_137:
          ++p_Nodes->NumItems;
          v52 = outPt.x;
          p_Nodes->current = v49;
          this->Nodes.current->x = v52;
          v53 = (float)(v71 - y) * v64;
          v45 = v64 + v72;
          v64 = v45;
          this->Nodes.current->y = v53 + y;
          this->Nodes.current->z = outPt.y;
          if ( (float)((float)(1.0f - v72) + 0.001f) < v45 )
            break;
          v36 = pos2.x;
          v66 = pos1.y;
          v68 = pos1.x;
        }
      }
      v54 = p_Nodes->current;
      if ( p_Nodes->Closed )
      {
        if ( !v54 )
          goto LABEL_149;
        v23 = v54->next;
        if ( !v23 )
          v23 = p_Nodes->first;
      }
      else if ( v54 )
      {
        v23 = v54->next;
      }
      else
      {
        v23 = 0;
      }
      p_Nodes->current = v23;
    }
    while ( this->Nodes.current );
  }
}

//----- (0045C150) --------------------------------------------------------

void SSpline::ReplaceNodesFrom(SSpline *from)

{
  SNode *first; // eax
  SNode *current; // eax
  float nodeX;
  float z;
  float y;
  this->Nodes.DeleteAll();
  first = from->Nodes.first;
  for ( from->Nodes.current = first; first; from->Nodes.current = first )
  {
    nodeX = first->x;
    y = first->y;
    z = first->z;
    if ( !this->Closed )
    {
      SNode* newNode = NewControlPointNode(nodeX, y, z, this->Gepard->HeightFrom);
      ChainAppend(&this->Nodes, newNode);
    }
    current = from->Nodes.current;
    if ( from->Nodes.Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = current->next;
      if ( !first )
        first = from->Nodes.first;
    }
    else if ( current )
    {
      first = current->next;
    }
    else
    {
      first = 0;
    }
  }
}

//----- (0045C2B0) --------------------------------------------------------

void SSpline::SelectAllNode()

{
  SNode *first; // eax
  SNode *current; // eax
  first = this->Nodes.first;
  for ( this->Nodes.current = first; first; this->Nodes.current = first )
  {
    first->Selected = 1;
    current = this->Nodes.current;
    if ( this->Nodes.Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = current->next;
      if ( !first )
        first = this->Nodes.first;
    }
    else if ( current )
    {
      first = current->next;
    }
    else
    {
      first = 0;
    }
  }
}

//----- (0045C300) --------------------------------------------------------

void SSpline::SelectNode(SNode *_node)

{
  _node->Selected = 1;
}

//----- (0045C310) --------------------------------------------------------

void SSpline::SetAltitude2(float _altitude)

{
  this->Altitude2 = _altitude;
}

//----- (0045C330) --------------------------------------------------------

void SSpline::SetAltitude(float _altitude)

{
  this->Altitude = _altitude;
}
