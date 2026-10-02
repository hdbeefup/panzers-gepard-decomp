// window/droplist.cpp
// Drop-down list widget
// Decompiled from: gameSplit/sdroplist.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

extern unsigned char g_MenuRace;

#include "droplist.h"
#include "logger.h"

// Classes: SDropList
// Function count: 31

//----- (00484DB0) --------------------------------------------------------

SDropList::SDropList()

{
  this->m_ListBoxItems.array = 0;
  this->m_ListBoxItems.size = 0;
  this->m_ListBoxItems.maxsize = 0;
  this->m_nCurSel = -1;
  this->m_nCurActive = -1;
  this->m_nTopIndex = 0;
  *(_WORD *)&this->Dropped = 256;
  this->Enabled = 1;
  this->DropLineFrames = 0;
  *(_WORD *)&this->Active = 0;
}

//----- (00484E30) --------------------------------------------------------

SDropList::~SDropList()

{
  int maxsize;
  SDropListItem *array;
  if ( this->Dropped )
    this->Drop();
  if ( this->m_ListBoxItems.size > -1 && this->m_nCurSel != -1 )
  {
    if ( this->Dropped )
      this->Drop();
    this->m_nCurSel = -1;
    this->Update();
  }
  if ( this->m_ListBoxItems.size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SDropListItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 272 * maxsize);
  this->Update();
  if ( this->BackFrame >= 0 )
    Board->ReleaseFont(this->ButtonFont);
  if ( this->m_ListBoxItems.array )
  {
    free(this->m_ListBoxItems.array);
    this->m_ListBoxItems.array = 0;
  }
  // base destructor called automatically
}

//----- (004850D0) --------------------------------------------------------

int SDropList::AddItem(int a2, const char *lpszItem, bool itemEnabled, unsigned int dwItemData)

{
  int size;
  int maxsize;
  int v8;
  SDropListItem *v9; // eax
  int v10;
  int itemEnableda;
  if ( this->Dropped )
    this->Drop();
  strncpy(this->m_ListBoxItem.ItemText, lpszItem, 0x104u);
  this->m_ListBoxItem.ItemData = dwItemData;
  this->m_ListBoxItem.ItemEnabled = itemEnabled;
  size = this->m_ListBoxItems.size;
  maxsize = this->m_ListBoxItems.maxsize;
  itemEnableda = size;
  if ( size == maxsize )
  {
    if ( maxsize >= 16 )
      v8 = 6 * maxsize / 5;
    else
      v8 = 16;
    v9 = (SDropListItem *)realloc(this->m_ListBoxItems.array, 272 * v8);
    v10 = this->m_ListBoxItems.maxsize;
    this->m_ListBoxItems.array = v9;
    memset(&v9[v10], 0, 272 * (v8 - v10));
    size = this->m_ListBoxItems.size;
    this->m_ListBoxItems.maxsize = v8;
    itemEnableda = size;
  }
  this->m_ListBoxItems.size = size + 1;
  qmemcpy(&this->m_ListBoxItems.array[size], &this->m_ListBoxItem, sizeof(this->m_ListBoxItems.array[size]));
  this->Update();
  return itemEnableda;
}

//----- (004851D0) --------------------------------------------------------

void SDropList::Create(int a2, int font, bool background, unsigned int droppedbackground_color)

{
  unsigned int v5;
  unsigned int v7;
  int v8;
  int v9;
  bool v10;
  int v11;
  int Width;
  int v14;
  int v16;
  int v17;
  int Height;
  v5 = droppedbackground_color;
  this->DroppedBackgroundColor = droppedbackground_color;
  if ( background )
  {
    if ( g_MenuRace )
      v7 = v5 - 1879048192;
    else
      v7 = v5 + 0x40000000;
    this->SetBackgroundColor(v7);
  }
  this->Font = font;
  Board->GetTextExtent(font,
    0,
    0,
    (int *)&droppedbackground_color,
    &this->FontHeight,
    1.0f);
  this->SetPosition(this->X, this->Y, this->Width, this->FontHeight + 2);
  SDXWidget::Create(a2);
  if ( background )
  {
    v8 = Board->CreateFrame(FT_BOX, this->BackFrame, 2, 2, 0, 0);
    Board->SetBoxColor(v8, this->DroppedBackgroundColor + 1409286144);
    Board->ResizeFrame(v8, this->Width - 4, this->Height - 4);
  }
  v9 = Board->CreateFrame(FT_FIXTEXT, this->BackFrame, 5, 0, 0, 1);
  v10 = this->Font == 0;
  v11 = v9;
  Width = this->Width;
  this->TextFrame = v11;
  Height = this->Height;
  if ( v10 )
  {
    Board->ResizeFrame(v11, Width - 22, Height);
    v14 = Board->CreateFrame(FT_SPRITE, this->BackFrame, this->Width - 17, 1, 0, 1);
    this->ButtonFrame = v14;
    if ( g_MenuRace )
      v16 = Board->LoadFixedFont("menu/pig_arrows_small.png", 16, 17, 6, 12, 0, X2);
    else
      v16 = Board->LoadFixedFont("menu/rabbit_arrows_small.png", 16, 17, 6, 12, 0, X2);
  }
  else
  {
    Board->ResizeFrame(v11, Width - 27, Height);
    v17 = Board->CreateFrame(FT_SPRITE, this->BackFrame, this->Width - 22, 1, 0, 1);
    this->ButtonFrame = v17;
    if ( g_MenuRace )
      v16 = Board->LoadFixedFont("menu/pig_arrows_medium.png", 21, 22, 6, 12, 0, X2);
    else
      v16 = Board->LoadFixedFont("menu/rabbit_arrows_medium.png", 21, 22, 6, 12, 0, X2);
  }
  this->ButtonFont = v16;
  this->Update();
}

//----- (004853A0) --------------------------------------------------------

int SDropList::DeleteItem(int nIndex)

{
  int size;
  int v4;
  int v5;
  SDropListItem *v6; // edi
  int v8;
  size = this->m_ListBoxItems.size;
  if ( nIndex >= size || nIndex < 0 )
    return -1;
  v4 = size - 1;
  v8 = nIndex;
  this->m_ListBoxItems.size = v4;
  if ( nIndex < v4 )
  {
    v5 = nIndex;
    do
    {
      v6 = &this->m_ListBoxItems.array[v5++];
      ++v8;
      qmemcpy(v6, &v6[1], sizeof(SDropListItem));
      v4 = this->m_ListBoxItems.size;
    }
    while ( v8 < v4 );
  }
  memset(&this->m_ListBoxItems.array[v4], 0, sizeof(this->m_ListBoxItems.array[v4]));
  this->Update();
  return nIndex;
}

//----- (00485440) --------------------------------------------------------

void SDropList::Drop()

{
  int *DropLineFrames; // eax
  SWidget *WindowOrScalerParent; // eax
  int v7;
  int v8;
  int v9;
  unsigned int DroppedBackgroundColor;
  unsigned int v11;
  int v12;
  int v13;
  int v14;
  unsigned int v15;
  int v16;
  int v17;
  float scaleFactor;
  int x;
  int y;
  if ( this->Dropped )
  {
    DropLineFrames = this->DropLineFrames;
    if ( DropLineFrames )
    {
      delete[] DropLineFrames;
      this->DropLineFrames = 0;
    }
    Board->DestroyFrame(this->DropBoxFrame);
    Board->DestroyFrame(this->DropBlackFrame);
    Board->DestroyFrame(this->DropFrame);
    this->ReleaseMouse();
    this->Dropped = 0;
    this->SendAction(279748, 0);
  }
  else
  {
    if ( this->m_ListBoxItems.size )
    {
      this->m_nCurActive = -1;
      this->GetWindowOrParentScalerPosition(&x, &y, &scaleFactor);
      WindowOrScalerParent = this->GetWindowOrScalerParent();
      v7 = Board->CreateFrame(
             FT_BOX,
             WindowOrScalerParent->GetFrame(),
             x,
             this->Height + y + 1,
             0,
             1);
      v8 = Board->CreateFrame(FT_BOX, v7, 0, 0, 0, 0);
      v9 = this->FontHeight + 3;
      this->DropFrame = v8;
      Board->ResizeFrame(v8, this->Width, this->m_ListBoxItems.size * v9);
      DroppedBackgroundColor = this->DroppedBackgroundColor;
      if ( g_MenuRace )
        v11 = DroppedBackgroundColor - 1879048192;
      else
        v11 = DroppedBackgroundColor + 0x40000000;
      Board->SetBoxColor(this->DropFrame, v11);
      v12 = Board->CreateFrame(FT_BOX, this->DropFrame, 0, 0, 0, 0);
      v13 = this->FontHeight + 3;
      this->DropBlackFrame = v12;
      Board->ResizeFrame(v12, this->Width, this->m_ListBoxItems.size * v13);
      Board->SetBoxColor(this->DropBlackFrame, 1883193151u);
      v14 = Board->CreateFrame(FT_BOX, this->DropBlackFrame, 2, 2, 0, 0);
      v15 = this->DroppedBackgroundColor;
      this->DropBoxFrame = v14;
      Board->SetBoxColor(v14, v15 + 1409286144);
      Board->ResizeFrame(
        this->DropBoxFrame,
        this->Width - 4,
        this->m_ListBoxItems.size * (this->FontHeight + 3) - 4);
      v16 = 0;
      this->DropLineFrames = (int *)operator new[](4 * this->m_ListBoxItems.size);
      if ( this->m_ListBoxItems.size > 0 )
      {
        v17 = 0;
        do
        {
          this->DropLineFrames[v16] = Board->CreateFrame(
                                        FT_FIXTEXT,
                                        this->DropFrame,
                                        5,
                                        v16 * (this->FontHeight + 3),
                                        0,
                                        1);
          Board->ResizeFrame(this->DropLineFrames[v16], this->Width - 14, this->FontHeight);
          Board->SetText(
            this->DropLineFrames[v16],
            this->Font,
            0,
            (const char *)&this->m_ListBoxItems.array[v17]);
          Board->SetTextColor(this->DropLineFrames[v16++], 15790320u);
          ++v17;
        }
        while ( v16 < this->m_ListBoxItems.size );
      }
      this->CaptureMouse();
      this->Dropped = 1;
    }
    this->SendAction(279748, 0);
  }
}

//----- (00485740) --------------------------------------------------------

int SDropList::GetCount()

{
  return this->m_ListBoxItems.size;
}

//----- (00485750) --------------------------------------------------------

int SDropList::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (00485760) --------------------------------------------------------

unsigned int SDropList::GetItemData(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].ItemData;
}

//----- (00485790) --------------------------------------------------------

unsigned int SDropList::GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer || nSizeOfBuffer <= 0 )
    return -1;
  strncpy(lpszBuffer, this->m_ListBoxItems.array[nIndex].ItemText, nSizeOfBuffer);
  return this->GetTextLen(nIndex);
}

//----- (004857F0) --------------------------------------------------------

unsigned int SDropList::GetTextLen(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return strlen(this->m_ListBoxItems.array[nIndex].ItemText);
}

//----- (00485830) --------------------------------------------------------

int SDropList::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (00485840) --------------------------------------------------------

bool SDropList::IsItemEnabled(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return 0;
  qmemcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(this->m_ListBoxItem));
  return this->m_ListBoxItem.ItemEnabled;
}

//----- (00485890) --------------------------------------------------------

bool SDropList::OnKeyDown(int keycode, bool repeat)

{
  SWidget *Up; // ecx
  int v6;
  if ( !Options->GetKeyboardMode() )
  {
    switch ( keycode )
    {
      case 38:
        this->SendAction(324866, 0);
        return 1;
      case 40:
        this->SendAction(324866, 1);
        return 1;
      case 37:
      case 39:
      case 13:
        return 1;
    }
  }
  if ( keycode != 38 )
  {
    if ( keycode == 40 )
    {
      if ( !this->Dropped && this->Down )
      {
        this->SetActive(0);
        this->Down->SetFocus();
        return 1;
      }
    }
    else
    {
      if ( keycode == 37 )
      {
        v6 = this->m_nCurSel - 1;
      }
      else
      {
        if ( keycode != 39 )
          return 0;
        v6 = this->m_nCurSel + 1;
      }
      if ( this->IsItemEnabled(v6) )
      {
        this->SetCurSel(v6);
        this->SendAction(279745, this->m_nCurSel);
      }
    }
    return 1;
  }
  if ( this->Dropped )
    return 1;
  Up = this->Up;
  if ( !Up )
    return 1;
  if ( this->Active )
  {
    this->Active = 0;
    this->Update();
    Up = this->Up;
  }
  Up->SetFocus();
  return 1;
}

//----- (004859C0) --------------------------------------------------------

void SDropList::OnMouseDown(int button, int x, int y, int shift)

{
  bool v7; // al
  int Height;
  int v9;
  int ya;
  int yb;
  if ( this->Dropable )
  {
    if ( button == 1 )
    {
      if ( this->Dropped )
      {
        this->Pressed = 0;
        v7 = x >= 0 && x < this->Width && y >= 0 && y < this->Height;
        this->Active = v7;
        if ( x < 0
          || x >= this->Width
          || (Height = this->Height, y < Height)
          || (ya = this->FontHeight + 3, y >= Height + ya * this->m_ListBoxItems.size) )
        {
          this->Drop();
        }
        else
        {
          v9 = (y - Height) / ya;
          yb = (y - Height) / ya;
          if ( this->m_ListBoxItems.array[v9].ItemEnabled )
          {
            this->Drop();
            this->SetCurSel(yb);
            Concert->PlaySound(
              "menu/button_down.wav",
              -12.0f,
              0,
              -1);
            this->SendAction(279745, this->m_nCurSel);
          }
        }
      }
      else if ( x >= 0 && x < this->Width && y >= 0 && y < this->Height )
      {
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0,
          -1);
        this->Pressed = 1;
        this->CaptureMouse();
      }
    }
    this->Update();
    SDXWidget::OnMouseDown(button, x, y, shift);
  }
}

//----- (00485B40) --------------------------------------------------------

void SDropList::OnMouseMove(int x, int y, int shift)

{
  int Height;
  int v6;
  if ( this->Dropable && this->Dropped )
  {
    if ( x < 0
      || x >= this->Width
      || (Height = this->Height, y < Height)
      || y >= Height + (this->FontHeight + 3) * this->m_ListBoxItems.size )
    {
      if ( this->m_ListBoxItems.size > -1 )
      {
        this->m_nCurActive = -1;
        this->Update();
      }
    }
    else
    {
      v6 = (y - Height) / (this->FontHeight + 3);
      if ( this->m_ListBoxItems.array[v6].ItemEnabled )
      {
        this->SetCurActive(v6);
        SDXWidget::OnMouseMove(x, y, shift);
        return;
      }
    }
  }
  SDXWidget::OnMouseMove(x, y, shift);
}

//----- (00485C00) --------------------------------------------------------

void SDropList::OnMouseOut()

{
  if ( this->Dropable )
  {
    this->Active = 0;
    this->Update();
    this->SendAction(279747, 0);
  }
  SDXWidget::OnMouseOut();
}

//----- (00485C40) --------------------------------------------------------

void SDropList::OnMouseOver()

{
  if ( this->Dropable )
  {
    this->SendAction(324865, 0);
    Concert->PlaySound(
      "menu/button_over.wav",
      -30.0f,
      0,
      -1);
    if ( !this->Active )
    {
      this->Active = 1;
      this->Update();
    }
    this->SendAction(279746, 0);
  }
}

//----- (00485CB0) --------------------------------------------------------

void SDropList::OnMouseUp(int button, int x, int y, int shift)

{
  bool v8; // al
  if ( this->Dropable && this->Pressed && button == 1 )
  {
    this->Pressed = 0;
    v8 = x >= 0 && x < this->Width && y >= 0 && y < this->Height;
    this->Active = v8;
    this->ReleaseMouse();
    if ( this->Active )
      this->Drop();
    this->Update();
  }
}

//----- (00485D20) --------------------------------------------------------

void SDropList::ResetContent()

{
  int maxsize;
  SDropListItem *array;
  if ( this->Dropped )
    this->Drop();
  if ( this->m_ListBoxItems.size > -1 && this->m_nCurSel != -1 )
  {
    if ( this->Dropped )
      this->Drop();
    this->m_nCurSel = -1;
    this->Update();
  }
  if ( this->m_ListBoxItems.size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SDropListItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 272 * maxsize);
  this->Update();
}

//----- (00485DF0) --------------------------------------------------------

void SDropList::SetActive(bool active)

{
  if ( this->Active != active )
  {
    this->Active = active;
    this->Update();
  }
}

//----- (00485E10) --------------------------------------------------------

int SDropList::SetCurActive(int nSelect)

{
  int m_nCurActive;
  if ( nSelect >= this->m_ListBoxItems.size || nSelect < -1 )
    return -1;
  m_nCurActive = this->m_nCurActive;
  this->m_nCurActive = nSelect;
  this->Update();
  return m_nCurActive;
}

//----- (00485E50) --------------------------------------------------------

int SDropList::SetCurSel(int nSelect)

{
  int m_nCurSel;
  if ( nSelect >= this->m_ListBoxItems.size )
    return -1;
  if ( nSelect < -1 )
    return -1;
  m_nCurSel = this->m_nCurSel;
  if ( m_nCurSel == nSelect )
    return -1;
  if ( this->Dropped )
  {
    this->Drop();
    m_nCurSel = this->m_nCurSel;
  }
  this->m_nCurSel = nSelect;
  this->Update();
  return m_nCurSel;
}

//----- (00485EB0) --------------------------------------------------------

void SDropList::SetDropable(bool dropable)

{
  this->Dropable = dropable;
  this->Update();
}

//----- (00485ED0) --------------------------------------------------------

void SDropList::SetFocus()

{
  SDXWidget::SetFocus();
  if ( !this->Active )
  {
    this->Active = 1;
    this->Update();
  }
}

//----- (00485F00) --------------------------------------------------------

unsigned int SDropList::SetItemData(int nIndex, unsigned int dwItemData)

{
  SDropListItem *v3; // eax
  SDropListItem *v4; // edi
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  qmemcpy(&this->m_ListBoxItem, v3, sizeof(this->m_ListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.ItemData;
  this->m_ListBoxItem.ItemData = dwItemData;
  qmemcpy(v4, &this->m_ListBoxItem, sizeof(SDropListItem));
  return result;
}

//----- (00485F70) --------------------------------------------------------

int SDropList::SetItemEnabled(int nIndex, bool enabled)

{
  SDropListItem *v3; // ebx
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  qmemcpy(&this->m_ListBoxItem, v3, sizeof(this->m_ListBoxItem));
  this->m_ListBoxItem.ItemEnabled = enabled;
  qmemcpy(v3, &this->m_ListBoxItem, sizeof(SDropListItem));
  this->Update();
  return 0;
}

//----- (00485FE0) --------------------------------------------------------

unsigned int SDropList::SetText(int nIndex, char *lpszBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer )
    return -1;
  qmemcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(this->m_ListBoxItem));
  strncpy(this->m_ListBoxItem.ItemText, lpszBuffer, 0x104u);
  qmemcpy(&this->m_ListBoxItems.array[nIndex], &this->m_ListBoxItem, sizeof(this->m_ListBoxItems.array[nIndex]));
  this->Update();
  return this->GetTextLen(nIndex);
}

//----- (00486070) --------------------------------------------------------

int SDropList::SetTopIndex(int nIndex)

{
  int m_nTopIndex;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (004860B0) --------------------------------------------------------

void SDropList::Update()

{
  int v2;
  int v3;
  int *DropLineFrames; // eax
  int v6;
  int m_nCurSel;
  int v9;
  if ( this->BackFrame >= 0 )
  {
    if ( !this->DropLineFrames )
    {
      if ( this->Dropable )
      {
        Board->ShowFrame(this->ButtonFrame, 1);
        m_nCurSel = this->m_nCurSel;
        if ( m_nCurSel >= 0 && m_nCurSel < this->m_ListBoxItems.size )
        {
          if ( this->Pressed )
          {
            Board->SetSpriteGlyph(this->ButtonFrame, this->ButtonFont, 8);
            Board->SetTextColor(this->TextFrame, 0xFFFFFFu);
            Board->SetText(
              this->TextFrame,
              this->Font + 2,
              0,
              (const char *)&this->m_ListBoxItems.array[this->m_nCurSel]);
          }
          else if ( this->Active )
          {
            Board->SetSpriteGlyph(this->ButtonFrame, this->ButtonFont, 7);
            Board->SetTextColor(this->TextFrame, 0xFFFFFFu);
            Board->SetText(
              this->TextFrame,
              this->Font + 1,
              0,
              (const char *)&this->m_ListBoxItems.array[this->m_nCurSel]);
          }
          else
          {
            Board->SetTextColor(this->TextFrame, 15790320);
            Board->SetText(
              this->TextFrame,
              this->Font,
              0,
              (const char *)&this->m_ListBoxItems.array[this->m_nCurSel]);
            Board->SetSpriteGlyph(this->ButtonFrame, this->ButtonFont, 6);
          }
          return;
        }
      }
      else
      {
        v6 = this->m_nCurSel;
        if ( v6 >= 0 && v6 < this->m_ListBoxItems.size )
        {
          Board->SetTextColor(this->TextFrame, 15790320u);
          Board->SetText(
            this->TextFrame,
            this->Font,
            0,
            (const char *)&this->m_ListBoxItems.array[this->m_nCurSel]);
          Board->ShowFrame(this->ButtonFrame, 0);
          return;
        }
      }
      Board->SetTextColor(this->TextFrame, 15790320u);
      Board->SetText(this->TextFrame, this->Font, 0, "");
      return;
    }
    v2 = 0;
    if ( this->m_ListBoxItems.size > 0 )
    {
      v3 = 0;
      v9 = 0;
      do
      {
        if ( this->m_nCurActive == v2 )
        {
          Board->SetTextColor(this->DropLineFrames[this->m_nCurActive], 0xFFFFFF);
          Board->SetText(
            this->DropLineFrames[this->m_nCurActive],
            this->Font + 1,
            0,
            (const char *)&this->m_ListBoxItems.array[this->m_nCurActive]);
          v3 = v9;
        }
        else
        {
          DropLineFrames = this->DropLineFrames;
          if ( *(&this->m_ListBoxItems.array->ItemEnabled + v3) )
            Board->SetTextColor(DropLineFrames[v2], 15790320);
          else
            Board->SetTextColor(DropLineFrames[v2], 0x666666);
          Board->SetText(this->DropLineFrames[v2], this->Font, 0, &this->m_ListBoxItems.array->ItemText[v3]);
        }
        ++v2;
        v3 += 272;
        v9 = v3;
      }
      while ( v2 < this->m_ListBoxItems.size );
    }
  }
}
