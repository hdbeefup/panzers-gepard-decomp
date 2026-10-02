// window/editbox.cpp
// Text edit box
// Decompiled from: gameSplit/sdxwidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <string.h>
#include <wchar.h>

extern unsigned char g_MenuRace;
#include "editbox.h"

// Helper struct for UTF-8 writing
struct Utf8WriteIterator {
  unsigned char *p;
  unsigned char *end;

  void AppendCodepoint(unsigned int cp) {
    if (cp < 0x80) {
      if (p + 1 <= end) *p++ = (unsigned char)(cp & 0x7F);
    } else if (cp < 0x800) {
      if (p + 2 <= end) {
        *p = (unsigned char)(((cp >> 6) & 0x1F) | 0xC0);
        p[1] = (unsigned char)((cp & 0x3F) | 0x80);
        p += 2;
      }
    } else if (cp < 0x10000) {
      if (p + 3 <= end) {
        *p = (unsigned char)(((cp >> 12) & 0xF) | 0xE0);
        p[1] = (unsigned char)(((cp >> 6) & 0x3F) | 0x80);
        p[2] = (unsigned char)((cp & 0x3F) | 0x80);
        p += 3;
      }
    } else {
      if (p + 4 <= end) {
        *p = (unsigned char)((cp >> 18) | 0xF0);
        p[1] = (unsigned char)(((cp >> 12) & 0x3F) | 0x80);
        p[2] = (unsigned char)(((cp >> 6) & 0x3F) | 0x80);
        p[3] = (unsigned char)((cp & 0x3F) | 0x80);
        p += 4;
      }
    }
  }
};

// Classes: SEditBox
// Function count: 10

//----- (004879A0) --------------------------------------------------------

SEditBox::SEditBox()

{
  this->CtrlPressed = 0;
  this->TextFrame = -1;
  this->CaretFrame = -1;
  this->Text[0] = 0;
  this->CaretPos = 0;
}

//----- (00487AF0) --------------------------------------------------------

void SEditBox::Create(int font, bool alpha, int background)

{
  char v4; // bl
  int v6;
  int Width;
  int v8;
  int v9;
  int v10;
  int v11;
  int FontWidth;
  v4 = background;
  if ( (_BYTE)background )
  {
    if ( g_MenuRace )
      this->SetBackgroundColor(0x90282828);
    else
      this->SetBackgroundColor(0x40282828u);
  }
  SDXWidget::Create(0);
  this->Font = font;
  Board->GetTextExtent(font, 0, 0, &FontWidth, &background, 1.0f);
  v6 = Board->CreateFrame(FT_FIXTEXT, this->BackFrame, 5, 1, 0, 1);
  v11 = background;
  Width = this->Width;
  this->TextFrame = v6;
  Board->ResizeFrame(v6, Width - 10, v11);
  if ( v4 )
  {
    v8 = Board->CreateFrame(FT_BOX, this->BackFrame, 2, 2, 0, 0);
    v9 = v8;
    if ( alpha )
      Board->SetBoxColor(v8, 1411917864u);
    else
      Board->SetBoxColor(v8, (unsigned int)-268435456);
    Board->ResizeFrame(v9, this->Width - 4, this->Height - 4);
  }
  v10 = Board->CreateFrame(FT_TEXT, this->TextFrame, 0, 0, 0, 1);
  this->CaretFrame = v10;
  Board->SetText(v10, font, 0, "l");
  this->CaretVisible = 1;
  this->CaretTimer = this->SetTimer(0x1F4u);
  this->SetFocus();
  this->Update();
}

//----- (00487C50) --------------------------------------------------------

char *SEditBox::GetText()

{
  return this->Utf8Text;
}

//----- (00487C60) --------------------------------------------------------

bool SEditBox::OnChar(int code)

{
  wchar_t *Text; // esi
  int v7;
  int textheight;
  int textwidth;
  float scaleFactor;
  if ( code >= 1 && code <= 12 || code >= 14 && code <= 26 )
    return 0;
  Text = this->Text;
  while ( *Text++ )
    ;
  v7 = (int)(Text - &this->Text[1]);
  scaleFactor = Board->GetTextEffectiveScaleFactor(this->Font);
  Board->GetTextExtent(this->Font,
    this->Utf8Text,
    (int)strlen(this->Utf8Text),
    &textwidth,
    &textheight,
    scaleFactor);
  if ( textwidth < this->Width - 18 && (unsigned int)code >= 0x20 && v7 < 255 )
  {
    memmove(&this->Text[this->CaretPos + 1], &this->Text[this->CaretPos], 2 * (v7 - this->CaretPos) + 2);
    this->Text[this->CaretPos] = code;
    ++this->CaretPos;
    this->Update();
    this->SendAction(283681, 0);
  }
  return 1;
}

//----- (00487D80) --------------------------------------------------------

bool SEditBox::OnKeyDown(int keycode, bool repeat)

{
  int v4;
  wchar_t *v5; // ecx
  int v7;
  wchar_t *Text; // ecx
  int v12;
  wchar_t *v13; // ecx
  wchar_t *v14; // edi
  int CaretPos;
  int v17;
  char *v18; // edx
  char *v19; // ecx
  short v20; // ax
  switch ( keycode )
  {
    case 8:
    {
      // x64 fix: original used `(char *)this + 2 * v17 + 1180` byte
      // arithmetic where 1180 was x86 byte offset of `Text[]` from
      // `this`. On x64 SDXWidget grew (SWidget has pointer fields), so
      // 1180 lands inside Utf8Text[] instead of Text[] — the memmove
      // target was wrong, so backspace moved the caret but didn't
      // actually delete a wchar. Use typed Text[] access; matches the
      // DELETE key handling at case 46 below.
      if ( this->CaretPos <= 0 )
        return 1;
      --this->CaretPos;
      wchar_t *end = &this->Text[this->CaretPos + 1];
      while ( *end )
        ++end;
      // Move Text[CaretPos+1 .. end] (including the null terminator)
      // back one slot to overwrite Text[CaretPos].
      memmove(&this->Text[this->CaretPos],
              &this->Text[this->CaretPos + 1],
              ((end - &this->Text[this->CaretPos + 1]) + 1) * sizeof(wchar_t));
      this->Update();
      this->SendAction(283681, 0);
      return 1;
    }
    case 9:
    case 13:
    case 16:
    case 27:
    case 33:
    case 34:
    case 38:
    case 40:
    case 122:
      return 0;
    case 17:
      this->CtrlPressed = 1;
      return 0;
    // HD v1.7 backport — Ctrl+V paste / Ctrl+C copy.
    case 'V':
      if ( this->CtrlPressed )
      {
        this->GetClipboardText();
        return 1;
      }
      return 1;
    case 'C':
      if ( this->CtrlPressed )
      {
        this->SetClipboardText();
        return 1;
      }
      return 1;
    case 35:
      if ( this->CtrlPressed )
        return 0;
      Text = this->Text;
      while ( *Text++ )
        ;
      this->CaretPos = (int)(Text - &this->Text[1]);
      this->Update();
      return 1;
    case 36:
      if ( this->CtrlPressed )
        return 0;
      this->CaretPos = 0;
      this->Update();
      return 1;
    case 37:
      v4 = this->CaretPos;
      if ( v4 <= 0 )
        return 1;
      this->CaretPos = v4 - 1;
      this->Update();
      return 1;
    case 39:
      v5 = this->Text;
      while ( *v5++ )
        ;
      v7 = this->CaretPos;
      if ( v7 < (int)(v5 - &this->Text[1]) )
      {
        this->CaretPos = v7 + 1;
        this->Update();
      }
      return 1;
    case 46:
      if ( this->CtrlPressed )
        return 0;
      v12 = this->CaretPos;
      v13 = &this->Text[v12];
      if ( !this->Text[v12] )
        return 1;
      v14 = v13 + 1;
      while ( *v13++ )
        ;
      memmove(&this->Text[v12], &this->Text[v12 + 1], 2 * (v13 - v14));
      this->Update();
      this->SendAction(283681, 0);
      return 1;
    default:
      return 1;
  }
}

//----- (00487FF0) --------------------------------------------------------

bool SEditBox::OnKeyUp(int keycode)

{
  bool result; // al
  result = 0;
  if ( keycode == 17 )
    this->CtrlPressed = 0;
  return result;
}

//----- (00488010) --------------------------------------------------------

void SEditBox::OnMouseDown(int button, int x, int y, int shift)

{
  if ( button == 1 && x >= 0 && x < this->Width && y >= 0 && y < this->Height )
    this->SetFocus();
}

//----- (00488040) --------------------------------------------------------

void SEditBox::OnTimer(int id, unsigned int time)

{
  this->CaretVisible = !this->CaretVisible;
  this->Update();
}

//----- (00488060) --------------------------------------------------------

void SEditBox::SetText(const char *text)

{
  const char *v2; // edx
  const char *v3; // ebx
  wchar_t *v5; // ecx
  wchar_t *v6; // edi
  const char *v7; // esi
  unsigned int v8;
  unsigned int v9;
  char v10; // al
  const char *v11; // esi
  char v12; // bl
  char v13; // bh
  const char *v14; // esi
  char v15; // bl
  char v16; // bh
  int v17;
  int v18;
  unsigned int v19;
  wchar_t *v20; // edx
  wchar_t *v23;
  const char *v24;
  unsigned int v25;
  char text_3;
  v2 = text;
  v3 = &text[strlen(text)];
  v24 = v3;
  v5 = this->Text;
  v23 = v5;
  v25 = (unsigned int)(v5 + 255);
  v6 = v5;
  if ( text < v3 )
  {
    while ( 1 )
    {
      v7 = v2;
      if ( v2 < v3 )
      {
        v9 = *(unsigned char *)v2++;
        if ( v9 - 193 > 0x1E )
        {
          if ( v9 - 225 > 0xE )
          {
            if ( v9 - 241 > 6
              || (v14 = v7 + 4, v14 > v3)
              || (v15 = *v2, *v2 < 0x80u)
              || (unsigned char)v15 > 0xBFu
              || (v16 = v2[1], (unsigned char)(v16 + 0x80) > 0x3Fu)
              || (text_3 = v2[2], (unsigned char)(text_3 + 0x80) > 0x3Fu) )
            {
LABEL_22:
              v8 = v9;
              goto LABEL_23;
            }
            v2 = v14;
            v8 = (text_3 & 0x3F) + (((v16 & 0x3F) + ((((v9 & 7) << 6) + (v15 & 0x3F)) << 6)) << 6);
          }
          else
          {
            v11 = v7 + 3;
            if ( v11 > v3 )
              goto LABEL_22;
            v12 = *v2;
            if ( *v2 < 0x80u )
              goto LABEL_22;
            if ( (unsigned char)v12 > 0xBFu )
              goto LABEL_22;
            v13 = v2[1];
            if ( (unsigned char)(v13 + 0x80) > 0x3Fu )
              goto LABEL_22;
            v2 = v11;
            v8 = (v13 & 0x3F) + ((((v9 & 0xF) << 6) + (v12 & 0x3F)) << 6);
          }
        }
        else
        {
          if ( v2 + 1 > v3 )
            goto LABEL_22;
          v10 = *v2;
          if ( *v2 < 0x80u || (unsigned char)v10 > 0xBFu )
            goto LABEL_22;
          v2 = v7 + 2;
          v8 = ((v9 & 0x1F) << 6) + (v10 & 0x3F);
        }
      }
      else
      {
        v8 = 0;
      }
LABEL_23:
      v17 = 2 - (v8 < 0x10000);
      if ( (unsigned int)&v6[v17] <= v25 )
      {
        v18 = v17 - 1;
        if ( v18 )
        {
          if ( v18 == 1 )
          {
            v19 = v8 - 0x10000;
            *v6 = ((v19 >> 10) & 0x3FF) - 10240;
            v6[1] = (v19 & 0x3FF) - 9216;
            v6 += 2;
          }
        }
        else
        {
          *v6++ = (wchar_t)v8;
        }
      }
      v3 = v24;
      if ( v2 >= v24 )
      {
        v5 = v23;
        break;
      }
    }
  }
  if ( v6 + 1 <= v5 + 255 )
    *v6 = 0;
  v20 = v5 + 1;
  this->Text[255] = 0;
  while ( *v5++ )
    ;
  this->CaretPos = (int)(v5 - v20);
  this->Update();
}

//----- (00488250) --------------------------------------------------------

void SEditBox::Update()

{
  bool v4; // sf
  wchar_t *Text; // edi
  wchar_t *v6; // edx
  wchar_t *v8; // esi
  unsigned char *Utf8Text; // ebx
  unsigned int v10;
  unsigned int v11;
  wchar_t v12; // ax
  unsigned int v13;
  int v14;
  wchar_t *v16; // ebx
  int v17;
  unsigned int v18;
  unsigned int v20;
  int v21;
  int v22;
  SEditBox *v23; // edi
  Utf8WriteIterator wi;
  int height;
  float scaleFactor;
  int width;
  SEditBox *v31;
  wchar_t *v32;
  v4 = this->BackFrame < 0;
  v31 = this;
  if ( !v4 )
  {
    Text = this->Text;
    v6 = this->Text;
    while ( *v6++ )
      ;
    v8 = this->Text;
    Utf8Text = (unsigned char *)this->Utf8Text;
    wi.p = (unsigned char *)this->Utf8Text;
    v10 = (unsigned int)&Text[v6 - &this->Text[1]];
    wi.end = (unsigned char *)&this->Utf8Text[765];
    if ( (unsigned int)Text < v10 )
    {
      do
      {
        v11 = *v8++;
        if ( v11 - 55297 <= 0x3FE )
        {
          v32 = v8 + 1;
          if ( (unsigned int)(v8 + 1) <= v10 )
          {
            v12 = *v8;
            v8 = v32;
            v11 = (v12 & 0x3FF) + (((v11 & 0x3FF) + 64) << 10);
          }
        }
        v13 = v11;
        if ( v11 >= 0x80 )
        {
          if ( v11 >= 0x800 )
            v14 = 4 - (v11 < 0x10000);
          else
            v14 = 2;
        }
        else
        {
          v14 = 1;
        }
        if ( &Utf8Text[v14] <= wi.end )
        {
          switch ( v14 )
          {
            case 1:
              *Utf8Text++ = v13 & 0x7F;
              break;
            case 2:
              *Utf8Text = ((v13 >> 6) & 0x1F) | 0xC0;
              Utf8Text[1] = (v13 & 0x3F) | 0x80;
              Utf8Text += 2;
              break;
            case 3:
              *Utf8Text = ((v13 >> 12) & 0xF) | 0xE0;
              Utf8Text[1] = ((v13 >> 6) & 0x3F) | 0x80;
              Utf8Text[2] = (v13 & 0x3F) | 0x80;
              Utf8Text += 3;
              break;
            case 4:
              *Utf8Text = (v13 >> 18) | 0xF0;
              Utf8Text[1] = ((v13 >> 12) & 0x3F) | 0x80;
              Utf8Text[2] = ((v13 >> 6) & 0x3F) | 0x80;
              Utf8Text[3] = (v13 & 0x3F) | 0x80;
              Utf8Text += 4;
              break;
          }
          wi.p = Utf8Text;
        }
      }
      while ( (unsigned int)v8 < v10 );
    }
    wi.AppendCodepoint(0);
    v31->Utf8Text[765] = 0;
    Board->SetText(v31->TextFrame,
      v31->Font,
      0,
      v31->Utf8Text);
    v16 = 0;
    v32 = 0;
    v17 = 0;
    v18 = (unsigned int)&Text[wcslen(Text)];
    if ( (unsigned int)Text < v18 )
    {
      int caretPos = v31->CaretPos;
      do
      {
        if ( v17 >= caretPos )
          break;
        if ( (unsigned int)Text >= v18 )
          goto LABEL_33;
        v20 = *Text++;
        if ( v20 - 55297 <= 0x3FE )
        {
          if ( (unsigned int)(Text + 1) <= v18 )
          {
            v21 = *Text++ & 0x3FF;
            v20 = v21 + (((v20 & 0x3FF) + 64) << 10);
          }
          v16 = v32;
        }
        if ( v20 < 0x80 )
LABEL_33:
          v22 = 1;
        else
          v22 = v20 >= 0x800 ? 4 - (v20 < 0x10000) : 2;
        v16 = (wchar_t *)((char *)v16 + v22);
        ++v17;
        v32 = v16;
      }
      while ( (unsigned int)Text < v18 );
    }
    v23 = v31;
    scaleFactor = Board->GetTextEffectiveScaleFactor(v31->Font);
    Board->GetTextExtent(v23->Font, v23->Utf8Text, (int)(size_t)v16, &width, &height, scaleFactor);
    if ( width > v23->Width - 8 )
    {
      v23->CaretPos = 0;
      width = 1;
    }
    Board->MoveFrame(v23->CaretFrame, width, 0);
    Board->ShowFrame(v23->CaretFrame, v23->CaretVisible);
  }
}

SEditBox::~SEditBox()
{
  // No dynamic members to free — base class handles cleanup
}

// HD v1.7 backport — clipboard support.
// Win32 CF_UNICODETEXT round-trips cleanly with the wchar_t Text[] buffer
// the editbox already uses internally. Newlines are stripped because
// the edit boxes are single-line.

void SEditBox::GetClipboardText()
{
  if ( !OpenClipboard(0) )
    return;
  HANDLE h = GetClipboardData(CF_UNICODETEXT);
  if ( !h )
  {
    CloseClipboard();
    return;
  }
  const wchar_t *src = (const wchar_t *)GlobalLock(h);
  if ( !src )
  {
    CloseClipboard();
    return;
  }

  // Length of current text
  wchar_t *cur = this->Text;
  while ( *cur ) ++cur;
  int curLen = (int)(cur - this->Text);

  for ( const wchar_t *p = src; *p; ++p )
  {
    wchar_t ch = *p;
    if ( ch == L'\r' || ch == L'\n' || ch == L'\t' )
      continue;
    if ( ch < 0x20 )
      continue;
    if ( curLen >= 255 )
      break;
    int caret = this->CaretPos;
    int tail = curLen - caret;
    memmove(&this->Text[caret + 1], &this->Text[caret], (size_t)(tail + 1) * sizeof(wchar_t));
    this->Text[caret] = ch;
    ++this->CaretPos;
    ++curLen;
  }

  GlobalUnlock(h);
  CloseClipboard();
  this->Update();
  this->SendAction(283681, 0);
}

void SEditBox::SetClipboardText()
{
  if ( !OpenClipboard(0) )
    return;
  EmptyClipboard();

  wchar_t *end = this->Text;
  while ( *end ) ++end;
  size_t bytes = ((size_t)(end - this->Text) + 1) * sizeof(wchar_t);

  HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if ( h )
  {
    void *dst = GlobalLock(h);
    if ( dst )
    {
      memcpy(dst, this->Text, bytes);
      GlobalUnlock(h);
      SetClipboardData(CF_UNICODETEXT, h);
    }
    else
    {
      GlobalFree(h);
    }
  }

  CloseClipboard();
}
