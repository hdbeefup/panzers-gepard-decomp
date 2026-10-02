// window/playerlistbox.cpp
// Player list in lobby
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "playerlistbox.h"
#include "logger.h"
#include "timer.h"

// Forward declaration for SMulti (defined in network module)
struct SMulti {
    static SMulti *instance;
    char m_strLocalPlayerName[260];
};

// Compare function for qsort - compares PlayerListBoxItems by PlayerName
static int CompareItems(const void *a, const void *b)
{
    const SPlayerListBoxItem *itemA = (const SPlayerListBoxItem *)a;
    const SPlayerListBoxItem *itemB = (const SPlayerListBoxItem *)b;
    return _stricmp(itemA->PlayerName, itemB->PlayerName);
}

// Classes: SPlayerListBox
// Function count: 27

//----- (0048EB30) --------------------------------------------------------

SPlayerListBox::SPlayerListBox()

{
  this->m_ListBoxItems.array = 0;
  this->m_ListBoxItems.size = 0;
  this->m_ListBoxItems.maxsize = 0;
  this->m_nCurSel = -1;
  this->m_nTopIndex = 0;
  this->MaxInnerTextLines = 3000;
  this->StateFont = -1;
  this->TextLineBackFrames = 0;
  this->StatusFrame = 0;
  this->ReadyFrame = 0;
  this->PlayerNameFrame = 0;
  this->CurActive = -1;
  this->LastClickTime = 0;
}

//----- (0048EC10) --------------------------------------------------------

SPlayerListBox::~SPlayerListBox()

{
  bool v4; // cc
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  int *TextLineBackFrames; // eax
  int *StatusFramePtr; // eax
  int *ReadyFramePtr; // eax
  int *PlayerNameFramePtr; // eax
  SPlayerListBoxItem *array;
  v4 = this->m_ListBoxItems.size <= -1;
  if ( !v4 )
  {
    this->m_nCurSel = -1;
    this->Update();
    m_nTopIndex = this->m_nTopIndex;
    if ( m_nTopIndex <= -1 )
    {
      VisibleTextLines = this->VisibleTextLines;
      if ( VisibleTextLines + m_nTopIndex <= -1 )
        this->SetTopIndex(-VisibleTextLines);
    }
  }
  if ( this->m_ListBoxItems.size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SPlayerListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 268 * maxsize);
  this->m_nTopIndex = 0;
  this->Update();
  if ( this->BackFrame >= 0 )
  {
    TextLineBackFrames = this->TextLineBackFrames;
    if ( TextLineBackFrames )
    {
      delete[] TextLineBackFrames;
      this->TextLineBackFrames = 0;
    }
    StatusFramePtr = this->StatusFrame;
    if ( StatusFramePtr )
    {
      delete[] StatusFramePtr;
      this->StatusFrame = 0;
    }
    ReadyFramePtr = this->ReadyFrame;
    if ( ReadyFramePtr )
    {
      delete[] ReadyFramePtr;
      this->ReadyFrame = 0;
    }
    PlayerNameFramePtr = this->PlayerNameFrame;
    if ( PlayerNameFramePtr )
    {
      delete[] PlayerNameFramePtr;
      this->PlayerNameFrame = 0;
    }
  }
  Board->ReleaseFont(this->StateFont);
  if ( this->m_ListBoxItems.array )
  {
    free(this->m_ListBoxItems.array);
    this->m_ListBoxItems.array = 0;
  }
  // base destructor called automatically
  // base destructor called automatically
}

//----- (0048EFD0) --------------------------------------------------------

int SPlayerListBox::AddItem(UserStatus status, const char *PlayerName, unsigned int Color)

{
  int size;
  int v6;
  int v7;
  SPlayerListBoxItem *v8; // edi
  int maxsize;
  int v10;
  SPlayerListBoxItem *v11; // eax
  int v12;
  int statusa;
  int statusb;
  this->m_ListBoxItem.status = status;
  strncpy(this->m_ListBoxItem.PlayerName, PlayerName, 0x104u);
  size = this->m_ListBoxItems.size;
  this->m_ListBoxItem.Color = Color;
  for ( statusa = size; size > this->MaxInnerTextLines; statusa = size )
  {
    if ( size <= 0 )
      Logger.g->Panic("SDArray::operator[]: invalid index (%d)", 0);
    v6 = size - 1;
    statusb = 0;
    this->m_ListBoxItems.size = size - 1;
    if ( size - 1 > 0 )
    {
      v7 = 0;
      do
      {
        v8 = &this->m_ListBoxItems.array[v7++];
        qmemcpy(v8, &v8[1], sizeof(SPlayerListBoxItem));
        v6 = this->m_ListBoxItems.size;
        ++statusb;
      }
      while ( statusb < v6 );
    }
    memset(&this->m_ListBoxItems.array[v6], 0, sizeof(this->m_ListBoxItems.array[v6]));
    size = this->m_ListBoxItems.size;
  }
  maxsize = this->m_ListBoxItems.maxsize;
  if ( size == maxsize )
  {
    if ( maxsize >= 16 )
      v10 = 6 * maxsize / 5;
    else
      v10 = 16;
    v11 = (SPlayerListBoxItem *)realloc(this->m_ListBoxItems.array, 268 * v10);
    v12 = this->m_ListBoxItems.maxsize;
    this->m_ListBoxItems.array = v11;
    memset(&v11[v12], 0, 268 * (v10 - v12));
    size = this->m_ListBoxItems.size;
    this->m_ListBoxItems.maxsize = v10;
    statusa = size;
  }
  this->m_ListBoxItems.size = size + 1;
  qmemcpy(&this->m_ListBoxItems.array[size], &this->m_ListBoxItem, sizeof(this->m_ListBoxItems.array[size]));
  if ( !this->SelectAble )
    this->EnsureIndexIsVisible(this->m_ListBoxItems.size - 1, 0);
  this->QSort(0);
  this->Update();
  return statusa;
}

//----- (0048F190) --------------------------------------------------------

void SPlayerListBox::Create(int numberofrows, int statuswidth, int playernamewidth)

{
  int v4;
  int v5;
  int v7;
  int v9;
  int v10;
  int width;
  v4 = numberofrows;
  v5 = statuswidth;
  statuswidth += playernamewidth + 54;
  v7 = statuswidth;
  this->Font = 0;
  this->VisibleTextLines = v4;
  this->SelectAble = 1;
  Board->GetTextExtent(0, 0, 0, &width, &numberofrows, 1.0f);
  v9 = numberofrows * this->VisibleTextLines;
  this->RowHeight = numberofrows;
  this->Resize(v7, v9 + 12);
  SDXWidget::Create((int)this);
  this->TextLineBackFrames = (int *)operator new[](4 * this->VisibleTextLines);
  this->StatusFrame = (int *)operator new[](4 * this->VisibleTextLines);
  v10 = 0;
  for ( this->PlayerNameFrame = (int *)operator new[](4 * this->VisibleTextLines); v10 < this->VisibleTextLines; ++v10 )
  {
    this->TextLineBackFrames[v10] = Board->CreateFrame(FT_BOX, this->BackFrame, 6, numberofrows * v10 + 6, 0, 1);
    Board->ResizeFrame(this->TextLineBackFrames[v10], v5 + playernamewidth + 42, numberofrows);
    this->StatusFrame[v10] = Board->CreateFrame(FT_SPRITE, this->TextLineBackFrames[v10], 5, 3, 0, 1);
    Board->ResizeFrame(this->StatusFrame[v10], v5, numberofrows);
    this->PlayerNameFrame[v10] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v10], v5 + 10, 0, 0, 1);
    Board->ResizeFrame(this->PlayerNameFrame[v10], playernamewidth, numberofrows);
  }
  this->InsertChild(&this->Slider);
  this->Slider.Create((int)this, statuswidth, this->Height, this->VisibleTextLines);
  this->StateFont = Board->LoadFixedFont("menu/multi_status.png", 13, 13, 5, 5, 0, X2);
  this->Update();
}

//----- (0048F3A0) --------------------------------------------------------

int SPlayerListBox::DeleteItem(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  this->m_ListBoxItems.Remove(nIndex);
  if ( this->m_nCurSel >= this->m_ListBoxItems.size )
    this->SetCurSel(-1);
  this->QSort(0);
  this->Update();
  return nIndex;
}

//----- (0048F400) --------------------------------------------------------

int SPlayerListBox::DeletePlayer(const char *PlayerName)

{
  int v3;
  int size;
  int i;
  int result;
  SDArray<SPlayerListBoxItem> *p_m_ListBoxItems; // ebx
  int m_nTopIndex;
  int VisibleTextLines;
  int v12;
  int v13;
  v3 = 0;
  size = this->m_ListBoxItems.size;
  if ( size > 0 )
  {
    for ( i = 0; _stricmp(this->m_ListBoxItems.array[i].PlayerName, PlayerName); ++i )
    {
      if ( ++v3 >= this->m_ListBoxItems.size )
        return -1;
    }
    p_m_ListBoxItems = &this->m_ListBoxItems;
    if ( v3 == this->m_nCurSel && p_m_ListBoxItems->size > -1 )
    {
      this->m_nCurSel = -1;
      this->Update();
      m_nTopIndex = this->m_nTopIndex;
      if ( m_nTopIndex <= -1 )
      {
        VisibleTextLines = this->VisibleTextLines;
        if ( VisibleTextLines + m_nTopIndex <= -1 )
          this->SetTopIndex(-VisibleTextLines);
      }
    }
    this->m_ListBoxItems.Remove(v3);
    if ( this->m_nCurSel >= p_m_ListBoxItems->size && p_m_ListBoxItems->size > -1 )
    {
      this->m_nCurSel = -1;
      this->Update();
      v12 = this->m_nTopIndex;
      if ( v12 <= -1 )
      {
        v13 = this->VisibleTextLines;
        if ( v13 + v12 <= -1 )
          this->SetTopIndex(-v13);
      }
    }
    this->QSort(0);
    this->Update();
    size = p_m_ListBoxItems->size;
  }
  result = -1;
  if ( v3 < size )
    return v3;
  return result;
}

//----- (0048F520) --------------------------------------------------------

int SPlayerListBox::EnsureIndexIsVisible(int nIndex, bool bRolling)

{
  int v3;
  int size;
  int m_nTopIndex;
  int i;
  int v10;
  v3 = nIndex;
  size = this->m_ListBoxItems.size;
  if ( nIndex < size && nIndex >= 0 && size < this->VisibleTextLines )
    return nIndex;
  if ( !bRolling )
  {
    m_nTopIndex = this->m_nTopIndex;
    if ( nIndex < m_nTopIndex + this->VisibleTextLines && nIndex > m_nTopIndex )
      return nIndex;
  }
  if ( nIndex >= size || nIndex < 0 )
    return -1;
  for ( i = nIndex + this->VisibleTextLines; i > size; --v3 )
    --i;
  if ( v3 >= size || v3 < 0 )
  {
    this->Update();
    return -1;
  }
  else
  {
    v10 = this->m_nTopIndex;
    this->m_nTopIndex = v3;
    this->Update();
    this->Update();
    return v10;
  }
}

//----- (0048F5D0) --------------------------------------------------------

unsigned int SPlayerListBox::GetColor(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].Color;
}

//----- (0048F600) --------------------------------------------------------

int SPlayerListBox::GetCount()

{
  return this->m_ListBoxItems.size;
}

//----- (0048F610) --------------------------------------------------------

int SPlayerListBox::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (0048F620) --------------------------------------------------------

int SPlayerListBox::GetPlayerIndex(const char *PlayerName)

{
  int v3;
  int i;
  v3 = 0;
  if ( this->m_ListBoxItems.size <= 0 )
    return -1;
  for ( i = 0; _stricmp(this->m_ListBoxItems.array[i].PlayerName, PlayerName); ++i )
  {
    if ( ++v3 >= this->m_ListBoxItems.size )
      return -1;
  }
  return v3;
}

//----- (0048F670) --------------------------------------------------------

const char *SPlayerListBox::GetPlayerName(int nIndex)

{
  if ( nIndex < 0 || nIndex >= this->m_ListBoxItems.size )
    return "";
  else
    return this->m_ListBoxItems.array[nIndex].PlayerName;
}

//----- (0048F6A0) --------------------------------------------------------

UserStatus SPlayerListBox::GetStatus(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return (UserStatus)0;
  else
    return this->m_ListBoxItems.array[nIndex].status;
}

//----- (0048F6D0) --------------------------------------------------------

int SPlayerListBox::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (0048F6E0) --------------------------------------------------------

bool SPlayerListBox::OnAction(SWidget *sender, int action, int param)

{
  int v5;
  switch ( action )
  {
    case 341345:
      v5 = this->m_nTopIndex - 1;
LABEL_3:
      this->EnsureIndexIsVisible(v5, 1);
LABEL_4:
      this->SendAction(84198434, this->m_nCurSel);
      return 1;
    case 341346:
      v5 = this->m_nTopIndex + 1;
      goto LABEL_3;
    case 341347:
      this->SetTopIndex(param);
      goto LABEL_4;
  }
  return 0;
}

//----- (0048F740) --------------------------------------------------------

void SPlayerListBox::OnMouseDown(int button, int x, int y, int shift)

{
  int v6;
  int HeightVal;
  int m_nCurSel;
  if ( this->SelectAble )
  {
    v6 = y;
    if ( button == 1 )
    {
      HeightVal = this->Height;
      if ( y >= HeightVal - 6 )
        v6 = HeightVal - 7;
      if ( this->m_nCurSel == this->m_nTopIndex + (v6 - 6) / this->RowHeight
        && (unsigned int)Timer.GetTickValue() - this->LastClickTime <= 0x2EE )
      {
        m_nCurSel = this->m_nCurSel;
        this->LastClickTime = 0;
        this->SendAction(84198435, m_nCurSel);
        return;
      }
      if ( this->SetCurSel(this->m_nTopIndex + (v6 - 6) / this->RowHeight) != -2 )
      {
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0,
          -1);
        this->SendAction(84198433, this->m_nCurSel);
        this->LastClickTime = (unsigned int)Timer.GetTickValue();

      }
    }
    SDXWidget::OnMouseDown(button, x, v6, shift);
  }
}

//----- (0048F840) --------------------------------------------------------

void SPlayerListBox::OnMouseMove(int x, int y, int shift)

{
  int v5;
  int v6;
  SPlayerListBoxItem *array; // eax
  int v8;
  int v9;
  char *Text; // esi
  unsigned int v14;
  char *v15; // edi
  char *v23; // ecx
  char *v25; // esi
  unsigned int v26;
  char *v27; // edi
  int v29;
  char v30; // al
  int v31;
  char v32; // al
  char buf[260];
  char buf2[260];
  if ( this->SelectAble )
  {
    this->SetTooltipText("");
    this->LastClickTime = 0;
    v5 = this->m_nTopIndex + (y - 6) / this->RowHeight;
    this->CurActive = v5;
    if ( v5 < this->m_ListBoxItems.size && v5 >= 0 )
    {
      v6 = v5;
      array = this->m_ListBoxItems.array;
      buf2[0] = 0;
      v8 = (unsigned char)array[v6].status - 1;
      if ( v8 )
      {
        v9 = v8 - 1;
        if ( v9 )
        {
          if ( v9 == 1 )
          {
            Text = GetText("SWINE_PLAYERLIST_AWAY");
            v14 = strlen(Text) + 1;
            v15 = &buf[259];
            while ( *++v15 )
              ;
            qmemcpy(v15, Text, v14);
          }
          else
          {
            strcpy(buf2, GetText("SWINE_PLAYERLIST_AVAILABLE"));
          }
        }
        else
        {
          strcpy(buf2, GetText("SWINE_PLAYERLIST_PLAYING"));
        }
      }
      else
      {
        strcpy(buf2, GetText("SWINE_PLAYERLIST_INLOBBY"));
      }
      if ( SMulti::instance
        && !_stricmp(this->m_ListBoxItems.array[this->CurActive].PlayerName, SMulti::instance->m_strLocalPlayerName) )
      {
        v23 = &buf[259];
        while ( *++v23 )
          ;
        strcpy(v23, ", ");
        v25 = GetText("SWINE_YOU");
        v26 = strlen(v25) + 1;
        v27 = &buf[259];
        while ( *++v27 )
          ;
        qmemcpy(v27, v25, v26);
        v29 = 0;
        do
        {
          v30 = buf2[v29++];
          buf[v29 - 1] = v30;
        }
        while ( v30 );
      }
      else
      {
        v31 = 0;
        do
        {
          v32 = buf2[v31++];
          buf[v31 - 1] = v32;
        }
        while ( v32 );
      }
      this->SetTooltipText(buf);
    }
    this->Update();
    SDXWidget::OnMouseMove(x, y, shift);
  }
}

//----- (0048FA80) --------------------------------------------------------

void SPlayerListBox::OnMouseOut()

{
  if ( this->SelectAble )
  {
    this->CurActive = -1;
    this->Update();
  }
  SDXWidget::OnMouseOut();
}

//----- (0048FAB0) --------------------------------------------------------

void SPlayerListBox::OnMouseWheel(int button, int x, int y, int delta)

{
  this->SendAction((button <= 0) + 341345, 0);
}

//----- (0048FAD0) --------------------------------------------------------

void SPlayerListBox::QSort(int InitialRow)

{
  int m_nCurSel;
  int v4;
  char *v5; // eax
  bool v6;
  const char *v7; // eax
  void *v8; // ebx
  const char *v9; // ecx
  int v10;
  int v11;
  void *playername;
  unsigned int playername_4;
  const char *Src;
  m_nCurSel = this->m_nCurSel;
  if ( m_nCurSel < 0 || m_nCurSel >= this->m_ListBoxItems.size )
  {
    v7 = "";
    Src = "";
  }
  else
  {
    v4 = 268 * m_nCurSel;
    v5 = this->m_ListBoxItems.array->PlayerName;
    v6 = &v5[v4] == 0;
    v7 = &v5[v4];
    Src = v7;
    if ( v6 )
    {
      v8 = 0;
      playername = 0;
      goto LABEL_7;
    }
  }
  playername_4 = strlen(v7);
  v8 = operator new[](playername_4 + 1);
  playername = v8;
  memcpy(v8, Src, playername_4 + 1);
LABEL_7:
  qsort(
    &this->m_ListBoxItems.array[InitialRow],
    this->m_ListBoxItems.size - InitialRow,
    0x10Cu,
    CompareItems);
  v9 = "";
  if ( v8 )
    v9 = (const char *)v8;
  v10 = 0;
  // x64: was `InitialRowa = (int)v9; ... v9 = (const char*)InitialRowa`
  // register-spill round-trip across _stricmp; v9 isn't modified inside
  // the loop, so the restore is redundant. Truncates an 8-byte char*.
  if ( this->m_ListBoxItems.size <= 0 )
  {
LABEL_13:
    v10 = -1;
  }
  else
  {
    v11 = 0;
    while ( _stricmp(this->m_ListBoxItems.array[v11].PlayerName, v9) )
    {
      ++v10;
      ++v11;
      if ( v10 >= this->m_ListBoxItems.size )
        goto LABEL_13;
    }
  }
  this->SetCurSel(v10);
  this->SendAction(84198433, this->m_nCurSel);
  if ( playername )
    delete[] playername;
}

//----- (0048FCA0) --------------------------------------------------------

void SPlayerListBox::ResetContent()

{
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  SPlayerListBoxItem *array;
  if ( this->m_ListBoxItems.size > -1 )
  {
    this->m_nCurSel = -1;
    this->Update();
    m_nTopIndex = this->m_nTopIndex;
    if ( m_nTopIndex <= -1 )
    {
      VisibleTextLines = this->VisibleTextLines;
      if ( VisibleTextLines + m_nTopIndex <= -1 )
        this->SetTopIndex(-VisibleTextLines);
    }
  }
  if ( this->m_ListBoxItems.size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SPlayerListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 268 * maxsize);
  this->m_nTopIndex = 0;
  this->Update();
}

//----- (0048FD70) --------------------------------------------------------

unsigned int SPlayerListBox::SetColor(int nIndex, unsigned int dwColor)

{
  SPlayerListBoxItem *v3; // eax
  SPlayerListBoxItem *v4; // edi
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  qmemcpy(&this->m_ListBoxItem, v3, sizeof(this->m_ListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.Color;
  this->m_ListBoxItem.Color = dwColor;
  qmemcpy(v4, &this->m_ListBoxItem, sizeof(SPlayerListBoxItem));
  return result;
}

//----- (0048FDE0) --------------------------------------------------------

int SPlayerListBox::SetCurSel(int nSelect)

{
  int v2;
  int m_nCurSel;
  int m_nTopIndex;
  int VisibleTextLines;
  v2 = nSelect;
  if ( nSelect < this->m_ListBoxItems.size && nSelect >= -1 )
  {
    m_nCurSel = this->m_nCurSel;
    this->m_nCurSel = nSelect;
    this->Update();
    m_nTopIndex = this->m_nTopIndex;
    if ( m_nTopIndex <= nSelect )
    {
      VisibleTextLines = this->VisibleTextLines;
      if ( VisibleTextLines + m_nTopIndex > nSelect )
        return m_nCurSel;
      v2 = nSelect - VisibleTextLines + 1;
    }
    this->SetTopIndex(v2);
    return m_nCurSel;
  }
  return -2;
}

//----- (0048FE50) --------------------------------------------------------

int SPlayerListBox::SetPlayerName(const char *oldPlayerName, const char *newPlayerName)

{
  int v4;
  int v5;
  int m_nCurSel;
  int v7;
  char *PlayerNamePtr; // eax
  bool v9;
  const char *v10; // eax
  const char *v11; // esi
  int size;
  const char *v13; // edx
  int v14;
  int m_nTopIndex;
  int VisibleTextLines;
  int result;
  void *block;
  unsigned int v22;
  const char *oldPlayerNamea;
  const char *newPlayerNamea;
  v4 = 0;
  if ( this->m_ListBoxItems.size <= 0 )
    goto LABEL_30;
  v5 = 0;
  while ( _stricmp(this->m_ListBoxItems.array[v5].PlayerName, oldPlayerName) )
  {
    ++v4;
    ++v5;
    if ( v4 >= this->m_ListBoxItems.size )
      goto LABEL_30;
  }
  strcpy(this->m_ListBoxItems.array[v4].PlayerName, newPlayerName);
  m_nCurSel = this->m_nCurSel;
  if ( m_nCurSel < 0 || m_nCurSel >= this->m_ListBoxItems.size )
  {
    v10 = "";
    oldPlayerNamea = "";
  }
  else
  {
    v7 = 268 * m_nCurSel;
    PlayerNamePtr = this->m_ListBoxItems.array->PlayerName;
    v9 = &PlayerNamePtr[v7] == 0;
    v10 = &PlayerNamePtr[v7];
    oldPlayerNamea = v10;
    if ( v9 )
    {
      v11 = 0;
      block = 0;
      goto LABEL_12;
    }
  }
  v22 = strlen(v10);
  block = operator new[](v22 + 1);
  memcpy(block, oldPlayerNamea, v22 + 1);
  v11 = (const char *)block;
LABEL_12:
  qsort(
    this->m_ListBoxItems.array,
    this->m_ListBoxItems.size,
    0x10Cu,
    CompareItems);
  size = this->m_ListBoxItems.size;
  v13 = "";
  if ( v11 )
    v13 = v11;
  v14 = 0;
  newPlayerNamea = v13;
  if ( size <= 0 )
  {
LABEL_18:
    v14 = -1;
  }
  else
  {
    // x64: was `&v15[(unsigned int)array->PlayerName]` byte arithmetic with
    // `v15 += 268` (x86 sizeof(SPlayerListBoxItem)) — truncates the 8-byte
    // PlayerName base on x64. Use typed array indexing; v14 already tracks
    // the same iteration count.
    while ( 1 )
    {
      v9 = _stricmp(this->m_ListBoxItems.array[v14].PlayerName, v13) == 0;
      size = this->m_ListBoxItems.size;
      if ( v9 )
        break;
      ++v14;
      v13 = newPlayerNamea;
      if ( v14 >= size )
        goto LABEL_18;
    }
  }
  if ( v14 >= size || v14 < -1 )
    goto LABEL_27;
  this->m_nCurSel = v14;
  this->Update();
  m_nTopIndex = this->m_nTopIndex;
  if ( m_nTopIndex > v14 )
    goto LABEL_24;
  VisibleTextLines = this->VisibleTextLines;
  if ( VisibleTextLines + m_nTopIndex <= v14 )
  {
    v14 = v14 - VisibleTextLines + 1;
LABEL_24:
    if ( v14 < this->m_ListBoxItems.size && v14 >= 0 )
    {
      this->m_nTopIndex = v14;
      this->Update();
    }
  }
LABEL_27:
  this->SendAction(84198433, this->m_nCurSel);
  if ( block )
    delete[] block;
  this->Update();
LABEL_30:
  result = -1;
  if ( v4 < this->m_ListBoxItems.size )
    return v4;
  return result;
}

//----- (00490070) --------------------------------------------------------

int SPlayerListBox::SetTopIndex(int nIndex)

{
  int m_nTopIndex;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (004900B0) --------------------------------------------------------

void SPlayerListBox::Update()

{
  int i;
  int *TextLineBackFrames; // eax
  int v7;
  int v8;
  int v9;
  if ( this->BackFrame >= 0 )
  {
    for ( i = 0; i < this->VisibleTextLines; ++i )
    {
      TextLineBackFrames = this->TextLineBackFrames;
      if ( i + this->m_nTopIndex == this->m_nCurSel )
        Board->SetBoxColor(
          TextLineBackFrames[i],
          0x80000000);
      else
        Board->SetBoxColor(TextLineBackFrames[i], 0);
      v7 = i + this->m_nTopIndex;
      if ( v7 >= this->m_ListBoxItems.size )
      {
        Board->ShowFrame(this->StatusFrame[i], 0);
        Board->SetText(this->PlayerNameFrame[i], this->Font, 0, "");
      }
      else
      {
        v8 = 0;
        switch ( this->m_ListBoxItems.array[v7].status )
        {
          case Available:
            v8 = 0;
            break;
          case InLobby:
            v8 = 2;
            break;
          case Playing:
            v8 = 3;
            break;
          case Away:
            v8 = 4;
            break;
          default:
            break;
        }
        Board->SetSpriteGlyph(this->StatusFrame[i], this->StateFont, v8);
        Board->ShowFrame(this->StatusFrame[i], 1);
        v9 = i + this->m_nTopIndex;
        if ( v9 == this->CurActive )
        {
          Board->SetText(this->PlayerNameFrame[i], this->Font + 1, 0, this->m_ListBoxItems.array[v9].PlayerName);
          Board->SetTextColor(this->PlayerNameFrame[i], 0xFFFFFFu);
        }
        else
        {
          Board->SetText(this->PlayerNameFrame[i], this->Font, 0, this->m_ListBoxItems.array[v9].PlayerName);
          Board->SetTextColor(this->PlayerNameFrame[i], this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
        }
      }
    }
    this->Slider.SetRows(this->m_ListBoxItems.size, this->m_nTopIndex);
  }
}

//----- (00490290) --------------------------------------------------------

int SPlayerListBox::UpdateItem(int nIndex, UserStatus status, const char *PlayerName, unsigned int Color)

{
  SPlayerListBoxItem *p_m_ListBoxItem; // esi
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  p_m_ListBoxItem = &this->m_ListBoxItem;
  this->m_ListBoxItem.status = status;
  strncpy(this->m_ListBoxItem.PlayerName, PlayerName, 0x104u);
  this->m_ListBoxItem.Color = Color;
  qmemcpy(&this->m_ListBoxItems.array[nIndex], p_m_ListBoxItem, sizeof(this->m_ListBoxItems.array[nIndex]));
  this->Update();
  return nIndex;
}
