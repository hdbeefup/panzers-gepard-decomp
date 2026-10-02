// window/listbox.cpp
// List box widget
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "core_common.h"
#include "listbox.h"
#include "logger.h"
#include "timer.h"

// Classes: SListBox
// Function count: 41

//----- (0048CD80) --------------------------------------------------------

SListBox::SListBox()

{
  this->m_ListBoxItems.array = 0;
  this->m_ListBoxItems.size = 0;
  this->m_ListBoxItems.maxsize = 0;
  this->m_nCurSel = -1;
  this->m_nTopIndex = 0;
  this->MaxInnerTextLines = 2000;
  this->CurActive = -1;
  this->MarginBottom = 6;
  this->MarginTop = 6;
  this->MarginRight = 6;
  this->MarginLeft = 6;
  this->SliderMarginBottom = 6;
  this->SliderMarginTop = 6;
  this->SliderMarginRight = 6;
  this->SliderMarginLeft = 6;
  this->LastClickTime = 0;
}

//----- (0048CE80) --------------------------------------------------------

SListBox::~SListBox()

{
  int m_nTopIndex;
  int VisibleTextLines;
  int *TextLineFrames;
  int *TextLineBackFrames;
  if ( this->m_ListBoxItems.size >= 0 )
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
  this->m_ListBoxItems.Clear();
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
}

//----- (0048D210) --------------------------------------------------------

int SListBox::AddItem(const char *lpszItem, unsigned int dwColor, unsigned int dwItemData)

{
  int v5;
  const char *v7;
  SListBoxItem *v8;
  int v10;
  SListBoxItem *array;
  signed int v12;
  signed int v13;
  const char *v14;
  const char *v15;
  unsigned int v16;
  unsigned int v17;
  int v18;
  int v19;
  char *v20;
  int v21;
  char v22;
  signed int v23;
  char *v24;
  int v25;
  char v26;
  SDArray<SListBoxItem> *p_m_ListBoxItems;
  int size;
  int maxsize;
  int v31;
  SListBoxItem *v32;
  SListBoxItem *v33;
  int v35;
  SDArray<SListBoxItem> *v36;
  int v37;
  int v38;
  int v39;
  SListBoxItem *v40;
  const char *v42;
  int BackFrame;
  char *Source;
  int bufheight;
  const char *v46;
  signed int v47;
  SListBoxItem *p_m_ListBoxItem;
  float scaleFactor;
  int v50;
  signed int v51;
  int v52;
  int bufwidth;
  char *lpszItema;
  BackFrame = this->BackFrame;
  v5 = this->Width - this->MarginRight - this->MarginLeft - 22;
  if ( !this->scrollbars )
    v5 = this->Width - this->MarginRight - this->MarginLeft;
  v52 = v5;
  v7 = lpszItem;
  scaleFactor = Board->GetTextEffectiveScaleFactor(BackFrame);
  Board->GetTextExtent(this->Font, lpszItem, strlen(lpszItem), &bufwidth, &bufheight, scaleFactor);
  if ( (!this->nobreaklines && bufwidth >= v52) || strchr(lpszItem, 10) )
  {
    lpszItema = new char[strlen(lpszItem) + 1];
    v12 = 0;
    v47 = 0;
    v51 = 0;
    v13 = 0;
    p_m_ListBoxItem = &this->m_ListBoxItem;
    while ( 1 )
    {
      v50 = 0;
      Source = (char *)&v7[v12];
      while ( 1 )
      {
        v42 = &v7[v13];
        v14 = strchr(&v7[v13], 32);
        v15 = strchr(v42, 10);
        v46 = v15;
        if ( !v15 )
          goto LABEL_16;
        if ( !v14 )
        {
          v14 = v15;
          goto LABEL_17;
        }
        if ( v15 < v14 )
          v14 = v15;
        else
LABEL_16:
          v46 = 0;
LABEL_17:
        if ( v14 )
          v16 = (unsigned int)(v14 - v7);
        else
          v16 = strlen(v7);
        v17 = v16 - v47;
        strncpy(lpszItema, Source, v16 - v47);
        v13 = v16 + 1;
        lpszItema[v17] = 0;
        Board->GetTextExtent(this->Font, lpszItema, strlen(lpszItema), &bufwidth, &bufheight, scaleFactor);
        v18 = bufwidth;
        v19 = v52;
        if ( bufwidth < v52 )
        {
          v20 = lpszItema;
          v21 = (int)((char *)p_m_ListBoxItem - lpszItema);
          do
          {
            v22 = *v20++;
            v20[v21 - 1] = v22;
          }
          while ( v22 );
          ++v50;
          v19 = v52;
          v51 = v13;
          goto LABEL_25;
        }
        if ( !v50 )
          break;
LABEL_25:
        if ( v46 || v18 >= v19 || v13 >= (int)strlen(v7) )
          goto LABEL_37;
      }
      v23 = v47;
      do
      {
        if ( --v13 > v23 )
        {
          do
          {
            if ( (unsigned char)(v7[v13] + 0x80) > 0x3Fu )
              break;
            --v13;
          }
          while ( v13 > v23 );
        }
        Board->GetTextExtent(this->Font,
          lpszItema,
          v13 - v23,
          &bufwidth,
          &bufheight,
          scaleFactor);
      }
      while ( bufwidth >= v52 );
      v24 = lpszItema;
      v25 = (int)((char *)p_m_ListBoxItem - lpszItema);
      lpszItema[v13 - v23] = 0;
      do
      {
        v26 = *v24++;
        v24[v25 - 1] = v26;
      }
      while ( v26 );
      v51 = v13;
LABEL_37:
      this->m_ListBoxItem.ItemData = dwItemData;
      this->m_ListBoxItem.Color = dwColor;
      p_m_ListBoxItems = &this->m_ListBoxItems;
      size = p_m_ListBoxItems->size;
      maxsize = p_m_ListBoxItems->maxsize;
      v50 = size;
      if ( size == maxsize )
      {
        if ( maxsize >= 16 )
          v31 = 6 * maxsize / 5;
        else
          v31 = 16;
        v32 = (SListBoxItem *)realloc(p_m_ListBoxItems->array, 272 * v31);
        p_m_ListBoxItems->array = v32;
        memset(&v32[p_m_ListBoxItems->maxsize], 0, 272 * (v31 - p_m_ListBoxItems->maxsize));
        size = p_m_ListBoxItems->size;
        p_m_ListBoxItems->maxsize = v31;
        v50 = size;
      }
      v33 = p_m_ListBoxItem;
      p_m_ListBoxItems->size = size + 1;
      memcpy(&this->m_ListBoxItems.array[size], v33, sizeof(SListBoxItem));
      v13 = v51;
      v12 = v51;
      v47 = v51;
      if ( v51 >= (int)strlen(v7) )
      {
        delete[] lpszItema;
        goto LABEL_44;
      }
    }
  }
  v8 = &this->m_ListBoxItem;
  strncpy(this->m_ListBoxItem.ItemText, lpszItem, sizeof(this->m_ListBoxItem.ItemText));
  this->m_ListBoxItem.ItemData = dwItemData;
  this->m_ListBoxItem.Color = dwColor;
  v10 = this->m_ListBoxItems.Add();
  array = this->m_ListBoxItems.array;
  v50 = v10;
  memcpy(&array[v10], v8, sizeof(SListBoxItem));
LABEL_44:
  v35 = this->m_ListBoxItems.size;
  v36 = &this->m_ListBoxItems;
  if ( v35 > this->MaxInnerTextLines )
  {
    do
    {
      if ( v35 <= 0 )
        Logger.g->Panic("SDArray::operator[]: invalid index (%d)", 0);
      v37 = v35 - 1;
      v38 = 0;
      v36->size = v37;
      if ( v37 > 0 )
      {
        v39 = 0;
        do
        {
          v40 = &v36->array[v39];
          ++v38;
          ++v39;
          memcpy(v40, &v40[1], sizeof(SListBoxItem));
          v37 = this->m_ListBoxItems.size;
          v36 = &this->m_ListBoxItems;
        }
        while ( v38 < v37 );
      }
      memset(&v36->array[v37], 0, sizeof(SListBoxItem));
      v35 = v36->size;
    }
    while ( v36->size > this->MaxInnerTextLines );
  }
  this->Update();
  return v50;
}

//----- (0048D630) --------------------------------------------------------

void SListBox::Create(int font, unsigned char NumberOfTextLines, bool bSelectAble, char bBackGround, int bScrollbars, int bNoBreakLines, int bAlignText, int bAlignVerticalCenter)

{
  int v9;
  char v10;
  int v12;
  int VisibleTextLines;
  int v15;
  int v16;
  int v17;
  int v18;
  int textWidth;
  int textHeight;
  v9 = font;
  v10 = bBackGround;
  this->aligntext = bAlignText;
  this->alignverticalcenter = bAlignVerticalCenter;
  this->nobreaklines = bNoBreakLines;
  v12 = bScrollbars;
  this->Font = font;
  this->scrollbars = v12;
  if ( v10 )
  {
    this->SetBackgroundColor(0x4C50B4A8u);
    v9 = this->Font;
  }
  textWidth = 0;
  textHeight = 0;
  Board->GetTextExtent(v9, 0, 0, &textWidth, &textHeight, 1.0f);
  this->Resize(this->Width, this->MarginTop + this->MarginBottom + textHeight * NumberOfTextLines);
  this->RowHeight = textHeight;
  SDXWidget::Create((int)this);
  VisibleTextLines = NumberOfTextLines;
  this->SelectAble = bSelectAble;
  this->VisibleTextLines = NumberOfTextLines;
  if ( v10 )
  {
    v15 = Board->CreateFrame(
            FT_BOX,
            this->BackFrame,
            this->MarginLeft,
            this->MarginTop,
            0,
            0);
    Board->SetBoxColor(v15, 860927144u);
    Board->ResizeFrame(
      v15,
      this->Width - this->MarginRight - this->MarginLeft,
      this->Height - this->MarginBottom - this->MarginTop);
    VisibleTextLines = this->VisibleTextLines;
  }
  v16 = this->Width - this->MarginRight - this->MarginLeft;
  v17 = v16 - 22;
  if ( !this->scrollbars )
    v17 = this->Width - this->MarginRight - this->MarginLeft;
  int frameWidth = v17;
  this->TextLineFrames = (int *)operator new[](4 * VisibleTextLines);
  v18 = 0;
  for ( this->TextLineBackFrames = (int *)operator new[](4 * this->VisibleTextLines); v18 < this->VisibleTextLines; ++v18 )
  {
    this->TextLineBackFrames[v18] = Board->CreateFrame(
                                      FT_BOX,
                                      this->BackFrame,
                                      this->MarginLeft,
                                      this->MarginTop + v18 * this->RowHeight,
                                      0,
                                      1);
    Board->ResizeFrame(this->TextLineBackFrames[v18], v16, this->RowHeight);
    this->TextLineFrames[v18] = Board->CreateFrame(
                                  (SFrameType)(this->nobreaklines + 2),
                                  this->TextLineBackFrames[v18],
                                  5,
                                  0,
                                  0,
                                  1);
    Board->ResizeFrame(this->TextLineFrames[v18], frameWidth, this->RowHeight);
  }
  if ( this->scrollbars )
  {
    this->InsertChild(&this->Slider);
    this->Slider.SetMargin(this->SliderMarginLeft, this->SliderMarginRight, this->SliderMarginTop, this->SliderMarginBottom);
    this->Slider.Create((int)this, this->Width, this->Height, VisibleTextLines);
  }
  this->Update();
}

//----- (0048D8D0) --------------------------------------------------------

int SListBox::DeleteItem(int nIndex)

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

//----- (0048D940) --------------------------------------------------------

int SListBox::EnsureIndexIsVisible(int nIndex, bool bRolling)

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
    return v10;
  }
}

//----- (0048D9F0) --------------------------------------------------------

unsigned int SListBox::GetColor(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].Color;
}

//----- (0048DA20) --------------------------------------------------------

int SListBox::GetCount()

{
  return this->m_ListBoxItems.size;
}

//----- (0048DA30) --------------------------------------------------------

int SListBox::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (0048DA40) --------------------------------------------------------

unsigned int SListBox::GetItemData(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].ItemData;
}

//----- (0048DA70) --------------------------------------------------------

unsigned int SListBox::GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer || nSizeOfBuffer <= 0 )
    return -1;
  strncpy(lpszBuffer, this->m_ListBoxItems.array[nIndex].ItemText, nSizeOfBuffer);
  return this->GetTextLen(nIndex);
}

//----- (0048DAD0) --------------------------------------------------------

unsigned int SListBox::GetTextLen(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return strlen(this->m_ListBoxItems.array[nIndex].ItemText);
}

//----- (0048DB10) --------------------------------------------------------

int SListBox::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (0048DB20) --------------------------------------------------------

int SListBox::GetVisibleTextLines()

{
  return this->VisibleTextLines;
}

//----- (0048DB30) --------------------------------------------------------

bool SListBox::IsIndexVisible(int nIndex)

{
  int m_nTopIndex;
  m_nTopIndex = this->m_nTopIndex;
  return nIndex >= m_nTopIndex && nIndex < m_nTopIndex + this->VisibleTextLines;
}

//----- (0048DB60) --------------------------------------------------------

bool SListBox::OnAction(SWidget *sender, int action, int param)

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

//----- (0048DBC0) --------------------------------------------------------

void SListBox::OnMouseDown(int button, int x, int y, int shift)

{
  int v6;
  int MarginTop;
  int v8;
  int m_nCurSel;
  if ( this->SelectAble )
  {
    v6 = y;
    if ( button == 1 )
    {
      MarginTop = this->MarginTop;
      v8 = this->Height - MarginTop;
      if ( y >= v8 )
        v6 = v8 - 1;
      if ( this->m_nCurSel == this->m_nTopIndex + (v6 - MarginTop) / this->RowHeight
        && (unsigned int)Timer.GetTickValue() - this->LastClickTime <= 0x2EE )
      {
        m_nCurSel = this->m_nCurSel;
        this->LastClickTime = 0;
        this->SendAction(312354, m_nCurSel);
        return;
      }
      if ( this->SetCurSel(this->m_nTopIndex + (v6 - this->MarginTop) / this->RowHeight) != -2 )
      {
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0.0f,
          -1);
        this->SendAction(312353, this->m_nCurSel);
        this->LastClickTime = (unsigned int)Timer.GetTickValue();

      }
    }
    SDXWidget::OnMouseDown(button, x, v6, shift);
  }
}

//----- (0048DCD0) --------------------------------------------------------

void SListBox::OnMouseMove(int x, int y, int shift)

{
  int v6;
  if ( this->SelectAble )
  {
    v6 = (y - this->MarginTop) / this->RowHeight;
    this->LastClickTime = 0;
    this->CurActive = this->m_nTopIndex + v6;
    this->Update();
    SDXWidget::OnMouseMove(x, y, shift);
  }
}

//----- (0048DD30) --------------------------------------------------------

void SListBox::OnMouseOut()

{
  if ( this->SelectAble )
  {
    this->CurActive = -1;
    this->Update();
  }
  SDXWidget::OnMouseOut();
}

//----- (0048DD60) --------------------------------------------------------

void SListBox::OnMouseWheel(int button, int x, int y, int delta)

{
  this->SendAction((button <= 0) + 341345, 0);
}

//----- (0048DE00) --------------------------------------------------------

void SListBox::ResetContent()

{
  int m_nTopIndex;
  int VisibleTextLines;
  if ( this->m_ListBoxItems.size >= 0 )
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
  this->m_ListBoxItems.Clear();
  this->m_nTopIndex = 0;
  this->Update();
}

//----- (0048DED0) --------------------------------------------------------

unsigned int SListBox::SetColor(int nIndex, unsigned int dwColor)

{
  SListBoxItem *v3;
  SListBoxItem *v4;
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(SListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.Color;
  this->m_ListBoxItem.Color = dwColor;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SListBoxItem));
  return result;
}

//----- (0048DF40) --------------------------------------------------------

int SListBox::SetCurSel(int nSelect)

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

//----- (0048DFB0) --------------------------------------------------------

unsigned int SListBox::SetItemData(int nIndex, unsigned int dwItemData)

{
  SListBoxItem *v3;
  SListBoxItem *v4;
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(SListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.ItemData;
  this->m_ListBoxItem.ItemData = dwItemData;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SListBoxItem));
  return result;
}

//----- (0048E020) --------------------------------------------------------

void SListBox::SetMargin(int left, int right, int top, int bottom)

{
  this->MarginLeft = left;
  this->MarginRight = right;
  this->MarginTop = top;
  this->MarginBottom = bottom;
}

//----- (0048E050) --------------------------------------------------------

void SListBox::SetSliderMargin(int left, int right, int top, int bottom)

{
  this->SliderMarginLeft = left;
  this->SliderMarginRight = right;
  this->SliderMarginTop = top;
  this->SliderMarginBottom = bottom;
}

//----- (0048E080) --------------------------------------------------------

unsigned int SListBox::SetText(int nIndex, char *lpszBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer )
    return -1;
  memcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(SListBoxItem));
  strncpy(this->m_ListBoxItem.ItemText, lpszBuffer, 0x104u);
  memcpy(&this->m_ListBoxItems.array[nIndex], &this->m_ListBoxItem, sizeof(SListBoxItem));
  this->Update();
  return this->GetTextLen(nIndex);
}

//----- (0048E110) --------------------------------------------------------

int SListBox::SetTopIndex(int nIndex)

{
  int m_nTopIndex;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (0048E150) --------------------------------------------------------

void SListBox::Update()

{
  int v4;
  int size;
  int VisibleTextLines;
  int v7;
  int *TextLineBackFrames;
  int v10;
  int v11;
  int v14;
  char emptyText[260] = {0};
  if ( this->BackFrame >= 0 )
  {
    v4 = 0;
    v14 = 0;
    if ( this->alignverticalcenter )
    {
      size = this->m_ListBoxItems.size;
      VisibleTextLines = this->VisibleTextLines;
      if ( size < VisibleTextLines )
      {
        v4 = (VisibleTextLines - size) / 2;
        v14 = v4;
      }
    }
    v7 = 0;
    if ( this->VisibleTextLines > 0 )
    {
      while ( 1 )
      {
        TextLineBackFrames = this->TextLineBackFrames;
        if ( v7 + this->m_nTopIndex == this->m_nCurSel )
          Board->SetBoxColor(
            TextLineBackFrames[v7],
            0x80000000);
        else
          Board->SetBoxColor(
            TextLineBackFrames[v7],
            0);
        if ( v7 < v4 )
          break;
        if ( this->SelectAble
          && (v10 = v7 + this->m_nTopIndex - v4, v10 < this->m_ListBoxItems.size)
          && v10 == this->CurActive )
        {
          Board->SetText(this->TextLineFrames[v7], this->Font + 1, this->aligntext, this->m_ListBoxItems.array[v10].ItemText);
          Board->SetTextColor(this->TextLineFrames[v7], 0xFFFFFFu);
        }
        else
        {
          v11 = v7 + this->m_nTopIndex - v4;
          if ( v11 >= this->m_ListBoxItems.size )
            break;
          Board->SetText(this->TextLineFrames[v7], this->Font, this->aligntext, this->m_ListBoxItems.array[v11].ItemText);
          Board->SetTextColor(
            this->TextLineFrames[v7],
            this->m_ListBoxItems.array[v7 + this->m_nTopIndex - v14].Color);
          v4 = v14;
        }
LABEL_17:
        if ( ++v7 >= this->VisibleTextLines )
          goto LABEL_18;
      }
      Board->SetText(this->TextLineFrames[v7], this->Font, this->aligntext, emptyText);
      goto LABEL_17;
    }
LABEL_18:
    if ( this->scrollbars )
      this->Slider.SetRows(this->m_ListBoxItems.size, this->m_nTopIndex);
  }
}
