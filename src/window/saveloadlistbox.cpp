// window/saveloadlistbox.cpp
// Save/load file list box
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "saveloadlistbox.h"
#include "logger.h"
#include "timer.h"

// Classes: SSaveLoadListBox
// Function count: 28

//----- (00491090) --------------------------------------------------------
// CompareTime - comparison function for qsort, compares SSaveLoadListBoxItems by SYSTEMTIME
static int __cdecl CompareTime(const void *elem1, const void *elem2)
{
  const SSaveLoadListBoxItem *a = (const SSaveLoadListBoxItem *)elem1;
  const SSaveLoadListBoxItem *b = (const SSaveLoadListBoxItem *)elem2;

  // Compare year
  if ( a->time.wYear < b->time.wYear )
    return 1;
  if ( a->time.wYear != b->time.wYear )
    return -1;
  // Compare month
  if ( a->time.wMonth < b->time.wMonth )
    return 1;
  if ( a->time.wMonth != b->time.wMonth )
    return -1;
  // Compare day
  if ( a->time.wDay < b->time.wDay )
    return 1;
  if ( a->time.wDay != b->time.wDay )
    return -1;
  // Compare hour
  if ( a->time.wHour < b->time.wHour )
    return 1;
  if ( a->time.wHour != b->time.wHour )
    return -1;
  // Compare minute
  if ( a->time.wMinute < b->time.wMinute )
    return 1;
  if ( a->time.wMinute != b->time.wMinute )
    return -1;
  // Compare second
  if ( a->time.wSecond < b->time.wSecond )
    return 1;
  if ( a->time.wSecond != b->time.wSecond )
    return -1;
  // Compare milliseconds
  if ( a->time.wMilliseconds < b->time.wMilliseconds )
    return 1;
  if ( a->time.wMilliseconds == b->time.wMilliseconds )
    return 0;
  return -1;
}

//----- (00490B90) --------------------------------------------------------

SSaveLoadListBox::SSaveLoadListBox()

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
  this->LastClickTime = 0;
#ifdef HD_BULK_DELETE_SAVES
  this->m_MultiSelected.array = 0;
  this->m_MultiSelected.size = 0;
  this->m_MultiSelected.maxsize = 0;
  this->m_nAnchorSel = -1;
#endif
}

//----- (00490C90) --------------------------------------------------------

SSaveLoadListBox::~SSaveLoadListBox()

{
  bool v4; // cc
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  int *TextLineFrames1; // eax
  int *TextLineFrames2; // eax
  int *TextLineFrames3; // eax
  int *DifficultyTextFrames; // eax
  int *TextLineBackFrames; // eax
  SSaveLoadListBoxItem *array;
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
    this->m_ListBoxItems.array = (SSaveLoadListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 1328 * maxsize);
  this->m_nTopIndex = 0;
  this->Update();
  if ( this->BackFrame >= 0 )
  {
    TextLineFrames1 = this->TextLineFrames1;
    if ( TextLineFrames1 )
    {
      delete[] TextLineFrames1;
      this->TextLineFrames1 = 0;
    }
    TextLineFrames2 = this->TextLineFrames2;
    if ( TextLineFrames2 )
    {
      delete[] TextLineFrames2;
      this->TextLineFrames2 = 0;
    }
    TextLineFrames3 = this->TextLineFrames3;
    if ( TextLineFrames3 )
    {
      delete[] TextLineFrames3;
      this->TextLineFrames3 = 0;
    }
    DifficultyTextFrames = this->DifficultyTextFrames;
    if ( DifficultyTextFrames )
    {
      delete[] DifficultyTextFrames;
      this->DifficultyTextFrames = 0;
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
#ifdef HD_BULK_DELETE_SAVES
  if ( this->m_MultiSelected.array )
  {
    free(this->m_MultiSelected.array);
    this->m_MultiSelected.array = 0;
  }
#endif
  // base destructor called automatically
  // base destructor called automatically
}

//----- (00490EA0) --------------------------------------------------------

const char *SSaveLoadListBox::AddItem(const char *lpszItem1, const char *lpszItem2, const char *lpszItem3, const char *lpszItem4, char *difficultyText, _SYSTEMTIME time, unsigned int dwColor)

{
  int size;
  int v10;
  int v11;
  SSaveLoadListBoxItem *v12; // edi
  int maxsize;
  int v14;
  SSaveLoadListBoxItem *v15; // eax
  int v16;
  const char *lpszItem1a;
  const char *lpszItem1b;
  strncpy(this->m_ListBoxItem.ItemText1, lpszItem1, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText2, lpszItem2, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText3, lpszItem3, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText4, lpszItem4, 0x104u);
  strncpy(this->m_ListBoxItem.DifficultyText, difficultyText, 0x104u);
  size = this->m_ListBoxItems.size;
  this->m_ListBoxItem.time = time;
  this->m_ListBoxItem.Color = dwColor;
  for ( lpszItem1a = (const char *)size; size > this->MaxInnerTextLines; lpszItem1a = (const char *)size )
  {
    if ( size <= 0 )
      Logger.g->Panic("SDArray::operator[]: invalid index (%d)", 0);
    v10 = size - 1;
    lpszItem1b = 0;
    this->m_ListBoxItems.size = size - 1;
    if ( size - 1 > 0 )
    {
      v11 = 0;
      do
      {
        v12 = &this->m_ListBoxItems.array[v11++];
        memcpy(v12, &v12[1], sizeof(SSaveLoadListBoxItem));
        v10 = this->m_ListBoxItems.size;
        ++lpszItem1b;
      }
      while ( (int)lpszItem1b < v10 );
    }
    memset(&this->m_ListBoxItems.array[v10], 0, sizeof(this->m_ListBoxItems.array[v10]));
    size = this->m_ListBoxItems.size;
  }
  maxsize = this->m_ListBoxItems.maxsize;
  if ( size == maxsize )
  {
    if ( maxsize >= 16 )
      v14 = 6 * maxsize / 5;
    else
      v14 = 16;
    v15 = (SSaveLoadListBoxItem *)realloc(this->m_ListBoxItems.array, 1328 * v14);
    v16 = this->m_ListBoxItems.maxsize;
    this->m_ListBoxItems.array = v15;
    memset(&v15[v16], 0, 1328 * (v14 - v16));
    size = this->m_ListBoxItems.size;
    this->m_ListBoxItems.maxsize = v14;
    lpszItem1a = (const char *)size;
  }
  this->m_ListBoxItems.size = size + 1;
  memcpy(&this->m_ListBoxItems.array[size], &this->m_ListBoxItem, sizeof(this->m_ListBoxItems.array[size]));
#ifdef HD_BULK_DELETE_SAVES
  {
    bool f = false;
    this->m_MultiSelected.Add(&f);
  }
#endif
  if ( !this->SelectAble )
    this->EnsureIndexIsVisible(this->m_ListBoxItems.size - 1, 0);
  this->Update();
  return lpszItem1a;
}

//----- (00491150) --------------------------------------------------------

void SSaveLoadListBox::Create(int font, unsigned char NumberOfTextLines, bool bSelectAble)

{
  unsigned int v13; // kr00_4
  int v15;
  int v16;
  float v17;
  int dateTextHeight;
  int width;
  _FILETIME LocalFileTime;
  int visiblerows;
  int v22;
  int v23;
  int dateTextWidth;
  int height;
  _SYSTEMTIME time;
  char strtime[260];
  char strdate[260];
  char strbuf[260];
  Board->GetTextExtent(font, 0, 0, &width, &height, 1.0f);
  visiblerows = NumberOfTextLines;
  this->Resize(this->Width, this->MarginTop + this->MarginBottom + height * NumberOfTextLines);
  this->RowHeight = height;
  SDXWidget::Create(NumberOfTextLines);
  this->SelectAble = bSelectAble;
  this->Font = font;
  this->VisibleTextLines = NumberOfTextLines;
  this->TextLineFrames1 = (int *)operator new[](4 * NumberOfTextLines);
  this->TextLineFrames2 = (int *)operator new[](4 * this->VisibleTextLines);
  this->TextLineFrames3 = (int *)operator new[](4 * this->VisibleTextLines);
  this->DifficultyTextFrames = (int *)operator new[](4 * this->VisibleTextLines);
  this->TextLineBackFrames = (int *)operator new[](4 * this->VisibleTextLines);
  // Build a sample date+time string to measure column width
  LocalFileTime.dwLowDateTime = 2071916032;
  LocalFileTime.dwHighDateTime = 29952358;
  FileTimeToSystemTime(&LocalFileTime, &time);
#ifdef HD_ISO_DATE_SAVES
  sprintf(strdate, "%04d-%02d-%02d", time.wYear, time.wMonth, time.wDay);
#else
  GetDateFormatA(0, 1u, &time, 0, strdate, 260);
#endif
  GetTimeFormatA(0, 0xEu, &time, 0, strtime, 260);
  // Original IDA code relied on strdate/strbuf being adjacent on stack.
  // Build combined "date time" string in strbuf directly.
  strcpy(strbuf, strdate);
  strcat(strbuf, " ");
  strcat(strbuf, strtime);
  v13 = strlen(strbuf);
  v17 = Board->GetTextEffectiveScaleFactor(this->BackFrame);
  Board->GetTextExtent(this->Font, strbuf, v13, &dateTextWidth, &dateTextHeight, v17);
  v15 = 0;
  v16 = 265 - (dateTextWidth + 2);
  v22 = dateTextWidth + 2;
  dateTextWidth += 2;
  this->WidthMiddle = v16 + 13;
  if ( this->VisibleTextLines > 0 )
  {
    v23 = v16 + 109;
    do
    {
      this->TextLineBackFrames[v15] = Board->CreateFrame(
                                        FT_BOX,
                                        this->BackFrame,
                                        this->MarginLeft,
                                        this->MarginTop + v15 * this->RowHeight,
                                        0,
                                        1);
      Board->ResizeFrame(
        this->TextLineBackFrames[v15],
        this->Width - this->MarginLeft - this->MarginRight,
        this->RowHeight);
      this->TextLineFrames1[v15] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v15], 5, 0, 0, 1);
      Board->ResizeFrame(this->TextLineFrames1[v15], 30, this->RowHeight);
      this->TextLineFrames2[v15] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v15], 43, 0, 0, 1);
      Board->ResizeFrame(this->TextLineFrames2[v15], v16, this->RowHeight);
      this->DifficultyTextFrames[v15] = Board->CreateFrame(
                                          FT_FIXTEXT,
                                          this->TextLineBackFrames[v15],
                                          v16 + 51,
                                          0,
                                          0,
                                          1);
      Board->ResizeFrame(this->DifficultyTextFrames[v15], 50, this->RowHeight);
      this->TextLineFrames3[v15] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v15], v23, 0, 0, 1);
      Board->ResizeFrame(this->TextLineFrames3[v15++], v22, this->RowHeight);
    }
    while ( v15 < this->VisibleTextLines );
  }
  this->InsertChild(&this->Slider);
  this->Slider.SetMargin(this->SliderMarginLeft, this->SliderMarginRight, this->SliderMarginTop, this->SliderMarginBottom);
  this->Slider.Create(v16, this->Width, this->Height, visiblerows);
  this->Update();
}

//----- (004915B0) --------------------------------------------------------

int SSaveLoadListBox::DeleteItem(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  if ( nIndex == this->m_nCurSel )
    this->SetCurSel(-1);
  this->m_ListBoxItems.Remove(nIndex);
#ifdef HD_BULK_DELETE_SAVES
  if ( nIndex < this->m_MultiSelected.size )
    this->m_MultiSelected.Remove(nIndex);
#endif
  if ( this->m_nCurSel >= this->m_ListBoxItems.size )
    this->SetCurSel(-1);
  this->Update();
  return nIndex;
}

//----- (00491620) --------------------------------------------------------

int SSaveLoadListBox::EnsureIndexIsVisible(int nIndex, bool bRolling)

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

//----- (004916D0) --------------------------------------------------------

unsigned int SSaveLoadListBox::GetColor(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].Color;
}

//----- (00491700) --------------------------------------------------------

int SSaveLoadListBox::GetCount()

{
  return this->m_ListBoxItems.size;
}

//----- (00491710) --------------------------------------------------------

int SSaveLoadListBox::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (00491720) --------------------------------------------------------

_SYSTEMTIME SSaveLoadListBox::GetItemData(int nIndex)

{
  _SYSTEMTIME time;
  memset(&time, 0, sizeof(time));
  if ( nIndex < this->m_ListBoxItems.size && nIndex >= 0 )
    time = this->m_ListBoxItems.array[nIndex].time;
  return time;
}

//----- (00491760) --------------------------------------------------------

unsigned int SSaveLoadListBox::GetText(int nIndex, char *lpszBuffer1, char *lpszBuffer2, char *lpszBuffer3, char *lpszBuffer4, char *difficultyText, int nSizeOfBuffer)

{
  if ( nIndex >= this->m_ListBoxItems.size
    || nIndex < 0
    || !lpszBuffer1
    || !lpszBuffer2
    || !lpszBuffer3
    || !lpszBuffer4
    || !difficultyText
    || nSizeOfBuffer <= 0 )
  {
    return -1;
  }
  strncpy(lpszBuffer1, this->m_ListBoxItems.array[nIndex].ItemText1, nSizeOfBuffer);
  strncpy(lpszBuffer2, this->m_ListBoxItems.array[nIndex].ItemText2, nSizeOfBuffer);
  strncpy(lpszBuffer3, this->m_ListBoxItems.array[nIndex].ItemText3, nSizeOfBuffer);
  strncpy(lpszBuffer4, this->m_ListBoxItems.array[nIndex].ItemText4, nSizeOfBuffer);
  strncpy(difficultyText, this->m_ListBoxItems.array[nIndex].DifficultyText, nSizeOfBuffer);
  return this->GetTextLen(0, nIndex);
}

//----- (00491860) --------------------------------------------------------

unsigned int SSaveLoadListBox::GetTextLen(int a2, int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return strlen(this->m_ListBoxItems.array[nIndex].ItemText1)
         + strlen(this->m_ListBoxItems.array[nIndex].ItemText2)
         + strlen(this->m_ListBoxItems.array[nIndex].ItemText3)
         + strlen(this->m_ListBoxItems.array[nIndex].DifficultyText)
         + strlen(this->m_ListBoxItems.array[nIndex].ItemText4);
}

//----- (00491910) --------------------------------------------------------

int SSaveLoadListBox::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (00491920) --------------------------------------------------------

bool SSaveLoadListBox::OnAction(SWidget *sender, int action, int param)

{
  switch ( action )
  {
    case 341345:
      this->EnsureIndexIsVisible(this->m_nTopIndex - 1, 1);
LABEL_3:
      this->SendAction(885310498, 0);
      return 1;
    case 341346:
      this->EnsureIndexIsVisible(this->m_nTopIndex + 1, 1);
      this->SendAction(885310498, 0);
      return 1;
    case 341347:
      this->SetTopIndex(param);
      goto LABEL_3;
    default:
      return 0;
  }
}

//----- (004919A0) --------------------------------------------------------

void SSaveLoadListBox::OnMouseDown(int button, int x, int y, int shift)

{
  int v6;
  int MarginTop;
  int v8;
  int TickValue;
  int m_nCurSel;
  int v11;
  if ( this->SelectAble )
  {
    v6 = y;
    if ( button != 1 )
      goto LABEL_10;
    MarginTop = this->MarginTop;
    v8 = this->Height - MarginTop;
    if ( y >= v8 )
      v6 = v8 - 1;
#ifdef HD_BULK_DELETE_SAVES
    {
      int newSel = this->m_nTopIndex + (v6 - MarginTop) / this->RowHeight;
      if ( newSel >= 0 && newSel < this->m_ListBoxItems.size && (shift & (0x0004 | 0x0008)) )
      {
        if ( shift & 0x0008 ) // MK_CONTROL: toggle
        {
          this->m_MultiSelected.array[newSel] = !this->m_MultiSelected.array[newSel];
          this->m_nAnchorSel = newSel;
        }
        else // MK_SHIFT: range from anchor
        {
          int anchor = (this->m_nAnchorSel >= 0 && this->m_nAnchorSel < this->m_ListBoxItems.size)
                       ? this->m_nAnchorSel : newSel;
          int lo = anchor < newSel ? anchor : newSel;
          int hi = anchor < newSel ? newSel : anchor;
          for ( int i = lo; i <= hi; ++i )
            this->m_MultiSelected.array[i] = true;
        }
        this->SetCurSel(newSel);
        this->LastClickTime = 0;
        this->SendAction(885310497, newSel);
        this->Update();
        return;
      }
      if ( newSel >= 0 && newSel < this->m_ListBoxItems.size )
      {
        this->ClearMultiSelection();
        this->m_nAnchorSel = newSel;
      }
    }
#endif
    if ( this->m_nCurSel == this->m_nTopIndex + (v6 - MarginTop) / this->RowHeight
      && (unsigned int)Timer.GetTickValue() - this->LastClickTime <= 0x2EE )
    {
      m_nCurSel = this->m_nCurSel;
      this->LastClickTime = 0;
      this->SendAction(885310499, m_nCurSel);
      return;
    }
    if ( this->SetCurSel(this->m_nTopIndex + (v6 - this->MarginTop) / this->RowHeight) == -2 )
    {
LABEL_10:
      SDXWidget::OnMouseDown(button, x, v6, shift);
    }
    else
    {
      Concert->PlaySound(
        "menu/button_down.wav",
        -12.0f,
        0.0f,
        -1);
      TickValue = (unsigned int)Timer.GetTickValue();

      v11 = this->m_nCurSel;
      this->LastClickTime = TickValue;
      this->SendAction(885310497, v11);
    }
  }
}

//----- (00491AB0) --------------------------------------------------------

void SSaveLoadListBox::OnMouseMove(int x, int y, int shift)

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

//----- (00491B10) --------------------------------------------------------

void SSaveLoadListBox::OnMouseOut()

{
  if ( this->SelectAble )
  {
    this->CurActive = -1;
    this->Update();
  }
  SDXWidget::OnMouseOut();
}

//----- (00491B40) --------------------------------------------------------

void SSaveLoadListBox::OnMouseWheel(int button, int x, int y, int delta)

{
  this->SendAction((button <= 0) + 341345, 0);
}

//----- (00491B60) --------------------------------------------------------

void SSaveLoadListBox::QSort(int InitialRow)

{
  qsort(
    &this->m_ListBoxItems.array[InitialRow],
    this->m_ListBoxItems.size - InitialRow,
    sizeof(SSaveLoadListBoxItem),
    CompareTime);
  this->Update();
}

//----- (00491C20) --------------------------------------------------------

void SSaveLoadListBox::ResetContent()

{
  int m_nTopIndex;
  int VisibleTextLines;
  int maxsize;
  SSaveLoadListBoxItem *array;
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
    this->m_ListBoxItems.array = (SSaveLoadListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 1328 * maxsize);
  this->m_nTopIndex = 0;
#ifdef HD_BULK_DELETE_SAVES
  this->ClearMultiSelection();
  this->m_nAnchorSel = -1;
#endif
  this->Update();
}

//----- (00491CF0) --------------------------------------------------------

unsigned int SSaveLoadListBox::SetColor(int nIndex, unsigned int dwColor)

{
  SSaveLoadListBoxItem *v3; // eax
  SSaveLoadListBoxItem *v4; // edi
  unsigned int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(this->m_ListBoxItem));
  v4 = v3;
  result = this->m_ListBoxItem.Color;
  this->m_ListBoxItem.Color = dwColor;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SSaveLoadListBoxItem));
  return result;
}

//----- (00491D60) --------------------------------------------------------

int SSaveLoadListBox::SetCurSel(int nSelect)

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

//----- (00491DD0) --------------------------------------------------------

int SSaveLoadListBox::SetItemData(int nIndex, _SYSTEMTIME time)

{
  SSaveLoadListBoxItem *v3; // eax
  SSaveLoadListBoxItem *v4; // edi
  int result;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  v3 = &this->m_ListBoxItems.array[nIndex];
  memcpy(&this->m_ListBoxItem, v3, sizeof(this->m_ListBoxItem));
  v4 = v3;
  result = nIndex;
  this->m_ListBoxItem.time = time;
  memcpy(v4, &this->m_ListBoxItem, sizeof(SSaveLoadListBoxItem));
  return result;
}

//----- (00491E40) --------------------------------------------------------

void SSaveLoadListBox::SetMargin(int left, int right, int top, int bottom)

{
  this->MarginLeft = left;
  this->MarginRight = right;
  this->MarginTop = top;
  this->MarginBottom = bottom;
}

//----- (00491E70) --------------------------------------------------------

void SSaveLoadListBox::SetSliderMargin(int left, int right, int top, int bottom)

{
  this->SliderMarginLeft = left;
  this->SliderMarginRight = right;
  this->SliderMarginTop = top;
  this->SliderMarginBottom = bottom;
}

//----- (00491EA0) --------------------------------------------------------

unsigned int SSaveLoadListBox::SetText(int nIndex, char *lpszBuffer1, char *lpszBuffer2, char *lpszBuffer3, char *lpszBuffer4, char *difficultyText)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 || !lpszBuffer1 || !lpszBuffer2 || !lpszBuffer3 || !lpszBuffer4 )
    return -1;
  memcpy(&this->m_ListBoxItem, &this->m_ListBoxItems.array[nIndex], sizeof(this->m_ListBoxItem));
  strncpy(this->m_ListBoxItem.ItemText1, lpszBuffer1, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText2, lpszBuffer2, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText3, lpszBuffer3, 0x104u);
  strncpy(this->m_ListBoxItem.ItemText4, lpszBuffer4, 0x104u);
  strncpy(this->m_ListBoxItem.DifficultyText, difficultyText, 0x104u);
  memcpy(&this->m_ListBoxItems.array[nIndex], &this->m_ListBoxItem, sizeof(this->m_ListBoxItems.array[nIndex]));
  this->Update();
  return this->GetTextLen(0, nIndex);
}

//----- (00491FA0) --------------------------------------------------------

int SSaveLoadListBox::SetTopIndex(int nIndex)

{
  int m_nTopIndex;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (00491FE0) --------------------------------------------------------

void SSaveLoadListBox::Update()

{
  int i;
  int *TextLineBackFrames; // eax
  int v7;
  char emptystr[260] = {0};
  if ( this->BackFrame >= 0 )
  {
    for ( i = 0; i < this->VisibleTextLines; ++i )
    {
      TextLineBackFrames = this->TextLineBackFrames;
      bool isSel = (i + this->m_nTopIndex == this->m_nCurSel);
#ifdef HD_BULK_DELETE_SAVES
      int mi = i + this->m_nTopIndex;
      if ( !isSel && mi >= 0 && mi < this->m_MultiSelected.size && this->m_MultiSelected.array[mi] )
        isSel = true;
#endif
      if ( isSel )
        Board->SetBoxColor(
          TextLineBackFrames[i],
          0x80000000);
      else
        Board->SetBoxColor(
          TextLineBackFrames[i],
          0);
      v7 = i + this->m_nTopIndex;
      if ( v7 >= this->m_ListBoxItems.size )
      {
        Board->SetText(this->TextLineFrames1[i], this->Font, 0, "");
        Board->SetText(this->TextLineFrames2[i], this->Font, 0, "");
        Board->SetText(this->TextLineFrames3[i], this->Font, 0, "");
        Board->SetText(this->DifficultyTextFrames[i], this->Font, 0, "");
      }
      else
      {
        if ( v7 == this->CurActive )
        {
          Board->SetText(this->TextLineFrames1[i], this->Font + 1, 0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText1);
          Board->SetText(
            this->TextLineFrames2[i],
            this->Font + 1,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText2);
          Board->SetText(
            this->TextLineFrames3[i],
            this->Font + 1,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText3);
          Board->SetText(
            this->DifficultyTextFrames[i],
            this->Font + 1,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].DifficultyText);
          Board->SetTextColor(this->TextLineFrames1[i], 0xFFFFFFu);
          Board->SetTextColor(this->TextLineFrames2[i], 0xFFFFFFu);
          Board->SetTextColor(this->TextLineFrames3[i], 0xFFFFFFu);
          Board->SetTextColor(this->DifficultyTextFrames[i], 0xFFFFFFu);
        }
        else
        {
          Board->SetText(this->TextLineFrames1[i], this->Font, 0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText1);
          Board->SetText(
            this->TextLineFrames2[i],
            this->Font,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText2);
          Board->SetText(
            this->TextLineFrames3[i],
            this->Font,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].ItemText3);
          Board->SetText(
            this->DifficultyTextFrames[i],
            this->Font,
            0,
            this->m_ListBoxItems.array[i + this->m_nTopIndex].DifficultyText);
          Board->SetTextColor(this->TextLineFrames1[i], this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
          Board->SetTextColor(this->TextLineFrames2[i], this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
          Board->SetTextColor(this->TextLineFrames3[i], this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
          Board->SetTextColor(
            this->DifficultyTextFrames[i],
            this->m_ListBoxItems.array[i + this->m_nTopIndex].Color);
        }
      }
    }
    this->Slider.SetRows(this->m_ListBoxItems.size, this->m_nTopIndex);
  }
}

#ifdef HD_BULK_DELETE_SAVES
int SSaveLoadListBox::GetMultiSelectCount()
{
  int count = 0;
  for ( int i = 0; i < this->m_MultiSelected.size; ++i )
    if ( this->m_MultiSelected.array[i] ) ++count;
  return count;
}

bool SSaveLoadListBox::IsMultiSelected(int i)
{
  if ( i < 0 || i >= this->m_MultiSelected.size ) return false;
  return this->m_MultiSelected.array[i];
}

void SSaveLoadListBox::ClearMultiSelection()
{
  for ( int i = 0; i < this->m_MultiSelected.size; ++i )
    this->m_MultiSelected.array[i] = false;
}
#endif
