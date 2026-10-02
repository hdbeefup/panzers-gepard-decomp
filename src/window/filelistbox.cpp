// window/filelistbox.cpp
// File browser list box
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "filelistbox.h"
#include "logger.h"

// Classes: SFileListBox
// Function count: 35

// Compare function for qsort
static int CompareItems(const void *a, const void *b)
{
  return strcmp(((const SFileListBoxItem *)a)->VisibleText, ((const SFileListBoxItem *)b)->VisibleText);
}

//----- (00488720) --------------------------------------------------------

SFileListBox::SFileListBox()

{
  this->m_ListBoxItems.array = 0;
  this->m_ListBoxItems.size = 0;
  this->m_ListBoxItems.maxsize = 0;
  this->m_nCurSel = -1;
  this->m_nTopIndex = 0;
  this->MaxInnerTextLines = 200;
  this->CurActive = -1;
  this->MarginBottom = 6;
  this->MarginTop = 6;
  this->MarginRight = 6;
  this->MarginLeft = 6;
  this->SliderMarginBottom = 6;
  this->SliderMarginTop = 6;
  this->SliderMarginRight = 6;
  this->SliderMarginLeft = 6;
}

//----- (00488810) --------------------------------------------------------

SFileListBox::~SFileListBox()

{
  bool v4; // cc
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  int *TextLineFrames; // eax
  int *TextLineBackFrames; // eax
  SFileListBoxItem *array;
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
    this->m_ListBoxItems.array = (SFileListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 532 * maxsize);
  this->m_nTopIndex = 0;
  this->Update();
  if ( this->BackFrame >= 0 )
  {
    TextLineFrames = this->TextLineFrames;
    if ( TextLineFrames )
    {
      delete[] TextLineFrames;
      this->TextLineFrames = 0;
    }
    TextLineBackFrames = this->TextLineBackFrames;
    if ( TextLineBackFrames )
    {
      delete[] TextLineBackFrames;
      this->TextLineBackFrames = 0;
    }
  }
  if ( this->m_ListBoxItems.array )
  {
    free(this->m_ListBoxItems.array);
    this->m_ListBoxItems.array = 0;
  }
  // base destructor called automatically
  // base destructor called automatically
}

//----- (00488B30) --------------------------------------------------------

const char *SFileListBox::AddItem(const char *lpszVisibleText, const char *lpszFileName, unsigned int dwColor, unsigned int dwItemData)

{
  int size;
  int maxsize;
  int v12;
  SFileListBoxItem *v13; // eax
  int v14;
  int i;
  int v16;
  int v17;
  SFileListBoxItem *v18; // edi
  const char *lpszFileNamea;
  strncpy(this->m_ListBoxItem.VisibleText, lpszVisibleText, 261);
  strncpy(this->m_ListBoxItem.Filename, lpszFileName, 261);
  this->m_ListBoxItem.ItemData = dwItemData;
  this->m_ListBoxItem.Color = dwColor;
  size = this->m_ListBoxItems.size;
  maxsize = this->m_ListBoxItems.maxsize;
  lpszFileNamea = (const char *)size;
  if ( size == maxsize )
  {
    if ( maxsize >= 16 )
      v12 = 6 * maxsize / 5;
    else
      v12 = 16;
    v13 = (SFileListBoxItem *)realloc(this->m_ListBoxItems.array, 532 * v12);
    v14 = this->m_ListBoxItems.maxsize;
    this->m_ListBoxItems.array = v13;
    memset(&v13[v14], 0, 532 * (v12 - v14));
    size = this->m_ListBoxItems.size;
    this->m_ListBoxItems.maxsize = v12;
    lpszFileNamea = (const char *)size;
  }
  this->m_ListBoxItems.size = size + 1;
  memcpy(&this->m_ListBoxItems.array[size], &this->m_ListBoxItem, sizeof(SFileListBoxItem));
  for ( i = this->m_ListBoxItems.size; i > this->MaxInnerTextLines; i = this->m_ListBoxItems.size )
  {
    if ( i <= 0 )
      Logger.g->Panic("SDArray::operator[]: invalid index (%d)", 0);
    v16 = i - 1;
    this->m_ListBoxItems.size = v16;
    if ( v16 > 0 )
    {
      v17 = 0;
      do
      {
        v18 = &this->m_ListBoxItems.array[v17++];
        memcpy(v18, &v18[1], sizeof(SFileListBoxItem));
        v16 = this->m_ListBoxItems.size;
      }
      while ( v17 < v16 );
    }
    memset(&this->m_ListBoxItems.array[v16], 0, sizeof(SFileListBoxItem));
  }
  this->Update();
  return lpszFileNamea;
}

//----- (00488CE0) --------------------------------------------------------

void SFileListBox::Create(int font, int NumberOfTextLines, bool bSelectAble, int bBackGround, int bScrollbars)

{
  char v6; // al
  int v7;
  char v8; // bl
  int v10;
  int VisibleTextLines;
  int v13;
  int v14;
  int v15;
  int v16;
  v6 = bScrollbars;
  v7 = font;
  v8 = bBackGround;
  this->Font = font;
  this->scrollbars = v6;
  if ( v8 )
  {
    this->SetBackgroundColor(0x4C50B4A8u);
    v7 = this->Font;
  }
  Board->GetTextExtent(v7, 0, 0, &font, &bScrollbars, 1.0f);
  v10 = (unsigned char)NumberOfTextLines;
  NumberOfTextLines = (unsigned char)NumberOfTextLines;
  this->Resize(
    this->Width,
    this->MarginTop + this->MarginBottom + bScrollbars * (unsigned char)NumberOfTextLines);
  this->RowHeight = bScrollbars;
  SDXWidget::Create((int)this);
  VisibleTextLines = v10;
  this->SelectAble = bSelectAble;
  this->VisibleTextLines = v10;
  if ( v8 )
  {
    v13 = Board->CreateFrame(
            FT_BOX,
            this->BackFrame,
            this->MarginLeft,
            this->MarginTop,
            0,
            0);
    Board->SetBoxColor(v13, 860927144u);
    Board->ResizeFrame(
      v13,
      this->Width - this->MarginRight - this->MarginLeft,
      this->Height - this->MarginBottom - this->MarginTop);
    VisibleTextLines = this->VisibleTextLines;
  }
  v14 = this->Width - this->MarginRight - this->MarginLeft;
  v15 = v14 - 22;
  if ( !this->scrollbars )
    v15 = this->Width - this->MarginRight - this->MarginLeft;
  bBackGround = v15;
  this->TextLineFrames = (int *)operator new[](4 * VisibleTextLines);
  v16 = 0;
  for ( this->TextLineBackFrames = (int *)operator new[](4 * this->VisibleTextLines); v16 < this->VisibleTextLines; ++v16 )
  {
    this->TextLineBackFrames[v16] = Board->CreateFrame(
                                      FT_BOX,
                                      this->BackFrame,
                                      this->MarginLeft,
                                      this->MarginTop + v16 * this->RowHeight,
                                      0,
                                      1);
    Board->ResizeFrame(this->TextLineBackFrames[v16], v14, this->RowHeight);
    this->TextLineFrames[v16] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v16], 5, 0, 0, 1);
    Board->ResizeFrame(this->TextLineFrames[v16], bBackGround, this->RowHeight);
  }
  if ( this->scrollbars )
  {
    this->InsertChild(&this->Slider);
    this->Slider.SetMargin(this->SliderMarginLeft, this->SliderMarginRight, this->SliderMarginTop, this->SliderMarginBottom);
    this->Slider.Create((int)this, this->Width, this->Height, NumberOfTextLines);
  }
  this->Update();
}

//----- (00488F50) --------------------------------------------------------

int SFileListBox::DeleteItem(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  if ( nIndex == this->m_nCurSel )
    this->SetCurSel(-1);
  this->m_ListBoxItems.Remove(nIndex);
  if ( this->m_nCurSel >= this->m_ListBoxItems.size )
    this->SetCurSel(-1);
  this->Update();
  return nIndex;
}

//----- (00488FC0) --------------------------------------------------------

int SFileListBox::EnsureIndexIsVisible(int nIndex, bool bRolling)

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
    this->Slider.SetRows(this->m_ListBoxItems.size, v3);
    this->Update();
    return v10;
  }
}

//----- (00489070) --------------------------------------------------------

unsigned int SFileListBox::GetColor(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].Color;
}

//----- (004890A0) --------------------------------------------------------

int SFileListBox::GetCount()

{
  return this->m_ListBoxItems.size;
}

//----- (004890B0) --------------------------------------------------------

int SFileListBox::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (004890C0) --------------------------------------------------------

unsigned int SFileListBox::GetFileName(int nIndex, char *lpszBuffer, int nSizeOfBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer || nSizeOfBuffer <= 0 )
    return -1;
  strncpy(lpszBuffer, this->m_ListBoxItems.array[nIndex].Filename, nSizeOfBuffer);
  return this->GetTextLen(nIndex);
}

//----- (00489120) --------------------------------------------------------

unsigned int SFileListBox::GetFileNameLen(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return strlen(this->m_ListBoxItems.array[nIndex].Filename);
}

//----- (00489170) --------------------------------------------------------

unsigned int SFileListBox::GetItemData(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].ItemData;
}

//----- (004891A0) --------------------------------------------------------

unsigned int SFileListBox::GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer || nSizeOfBuffer <= 0 )
    return -1;
  strncpy(lpszBuffer, this->m_ListBoxItems.array[nIndex].VisibleText, nSizeOfBuffer);
  return this->GetTextLen(nIndex);
}

//----- (00489200) --------------------------------------------------------

unsigned int SFileListBox::GetTextLen(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return strlen(this->m_ListBoxItems.array[nIndex].VisibleText);
}

//----- (00489240) --------------------------------------------------------

int SFileListBox::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (00489250) --------------------------------------------------------

bool SFileListBox::OnAction(SWidget *sender, int action, int param)

{
  switch ( action )
  {
    case 341345:
      this->EnsureIndexIsVisible(this->m_nTopIndex - 1, 1);
      return 1;
    case 341346:
      this->EnsureIndexIsVisible(this->m_nTopIndex + 1, 1);
      return 1;
    case 341347:
      this->SetTopIndex(param);
      return 1;
    default:
      return 0;
  }
}

//----- (004892B0) --------------------------------------------------------

void SFileListBox::OnMouseDown(int button, int x, int y, int shift)

{
  int v6;
  int MarginTop;
  int v8;
  if ( this->SelectAble )
  {
    v6 = y;
    if ( button == 1 )
    {
      MarginTop = this->MarginTop;
      v8 = this->Height - MarginTop;
      if ( y >= v8 )
        v6 = v8 - 1;
      if ( this->SetCurSel(this->m_nTopIndex + (v6 - MarginTop) / this->RowHeight) != -2 )
      {
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0,
          -1);
        this->SendAction(287777, this->m_nCurSel);
      }
    }
    SDXWidget::OnMouseDown(button, x, v6, shift);
  }
}

//----- (00489350) --------------------------------------------------------

void SFileListBox::OnMouseMove(int x, int y, int shift)

{
  if ( this->SelectAble )
  {
    this->CurActive = this->m_nTopIndex + (y - this->MarginTop) / this->RowHeight;
    this->Update();
    SDXWidget::OnMouseMove(x, y, shift);
  }
}

//----- (004893A0) --------------------------------------------------------

void SFileListBox::OnMouseOut()

{
  if ( this->SelectAble )
  {
    this->CurActive = -1;
    this->Update();
  }
  SDXWidget::OnMouseOut();
}

//----- (004893D0) --------------------------------------------------------

void SFileListBox::OnMouseWheel(int button, int x, int y, int delta)

{
  this->SendAction((button <= 0) + 341345, 0);
}

//----- (004893F0) --------------------------------------------------------

void SFileListBox::QSort(int InitialRow)

{
  qsort(
    &this->m_ListBoxItems.array[InitialRow],
    this->m_ListBoxItems.size - InitialRow,
    sizeof(SFileListBoxItem),
    CompareItems);
  this->Update();
}

//----- (004894B0) --------------------------------------------------------

void SFileListBox::ResetContent()

{
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  SFileListBoxItem *array;
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
    this->m_ListBoxItems.array = (SFileListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 532 * maxsize);
  this->m_nTopIndex = 0;
  this->Update();
}

//----- (00489580) --------------------------------------------------------

unsigned int SFileListBox::SetColor(int nIndex, unsigned int dwColor)

{
  SFileListBoxItem *v3; // eax
  SFileListBoxItem *v4; // edi
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(SFileListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.Color;
  this->m_ListBoxItem.Color = dwColor;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SFileListBoxItem));
  return result;
}

//----- (004895F0) --------------------------------------------------------

int SFileListBox::SetCurSel(int nSelect)

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

//----- (00489660) --------------------------------------------------------

unsigned int SFileListBox::SetFileName(int nIndex, char *lpszBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer )
    return -1;
  memcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(SFileListBoxItem));
  strncpy(this->m_ListBoxItem.Filename, lpszBuffer, 0x104u);
  memcpy(&this->m_ListBoxItems.array[nIndex], &this->m_ListBoxItem, sizeof(SFileListBoxItem));
  this->Update();
  return this->GetTextLen(nIndex);
}

//----- (00489700) --------------------------------------------------------

unsigned int SFileListBox::SetItemData(int nIndex, unsigned int dwItemData)

{
  SFileListBoxItem *v3; // eax
  SFileListBoxItem *v4; // edi
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(SFileListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.ItemData;
  this->m_ListBoxItem.ItemData = dwItemData;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SFileListBoxItem));
  return result;
}

//----- (00489770) --------------------------------------------------------

void SFileListBox::SetMargin(int left, int right, int top, int bottom)

{
  this->MarginLeft = left;
  this->MarginRight = right;
  this->MarginTop = top;
  this->MarginBottom = bottom;
}

//----- (004897A0) --------------------------------------------------------

void SFileListBox::SetSliderMargin(int left, int right, int top, int bottom)

{
  this->SliderMarginLeft = left;
  this->SliderMarginRight = right;
  this->SliderMarginTop = top;
  this->SliderMarginBottom = bottom;
}

//----- (004897D0) --------------------------------------------------------

unsigned int SFileListBox::SetText(int nIndex, char *lpszBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer )
    return -1;
  memcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(SFileListBoxItem));
  strncpy(this->m_ListBoxItem.VisibleText, lpszBuffer, 0x104u);
  memcpy(&this->m_ListBoxItems.array[nIndex], &this->m_ListBoxItem, sizeof(SFileListBoxItem));
  this->Update();
  return this->GetTextLen(nIndex);
}

//----- (00489860) --------------------------------------------------------

int SFileListBox::SetTopIndex(int nIndex)

{
  int m_nTopIndex;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (004898A0) --------------------------------------------------------

void SFileListBox::Update()

{
  int i;
  int *TextLineBackFrames; // eax
  int v7;
  int v8;
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
      if ( this->SelectAble && (v7 = i + this->m_nTopIndex, v7 < this->m_ListBoxItems.size) && v7 == this->CurActive )
      {
        Board->SetText(this->TextLineFrames[i], this->Font + 1, 0, this->m_ListBoxItems.array[v7].VisibleText);
        Board->SetTextColor(this->TextLineFrames[i], 0xFFFFFFu);
      }
      else
      {
        v8 = i + this->m_nTopIndex;
        if ( v8 >= this->m_ListBoxItems.size )
        {
          Board->SetText(this->TextLineFrames[i], this->Font, 0, "");
        }
        else
        {
          Board->SetText(this->TextLineFrames[i], this->Font, 0, this->m_ListBoxItems.array[v8].VisibleText);
          Board->SetTextColor(this->TextLineFrames[i], this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
        }
      }
    }
    if ( this->scrollbars )
      this->Slider.SetRows(this->m_ListBoxItems.size, this->m_nTopIndex);
  }
}
