// 3dengine/texture.cpp
// Texture/bitmap loading and management
// Decompiled from: gameSplit/sbitmap.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include "core_common.h"
#include "logger.h"
#include "stream.h"
#include "texture.h"
#include "hdbeefup.h"
#include <dxerr.h>

// Forward declarations for helper functions (defined in gameSplit globals, will be linked later)
extern void BltSameFormat(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int rowBytes, int rows);
extern void Blt_R8G8B8_From_X8R8G8B8(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_R5G6B5_From_DXT1(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_A1R5G5B5_From_DXT1(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_A4R4G4B4_From_DXT5(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_DXT_From_A8R8G8B8(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height, int type);
extern void Blt_A8R8G8B8_From_DXT1(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_A8R8G8B8_From_DXT5(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_DXT_From_SHADOW32(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);
extern void Blt_DXT_From_SHADOW16(unsigned char* dst, int dstPitch, unsigned char* src, int srcPitch, int width, int height);

// stb_image public API
#include "stb_image.h"
// SStream I/O callbacks for stb_image
int stbi_read_stream(void* user, char* data, int size);
void stbi_skip_stream(void* user, int n);
int stbi_eof_stream(void* user);

// Classes: SBitmap, SShadowMask, SSurfaceBitmap, STextureBitmap
// Function count: 27

//----- (00460400) --------------------------------------------------------

SBitmap::SBitmap(SBitmap *src)

{
  *this = *src;
  src->Data = 0;
}

//----- (00460430) --------------------------------------------------------

SBitmap::SBitmap(SBitmap *source, D3DFORMAT format)

{
  int Pixel;
  int Width;
  int Height;
  int v7;
  int v8;
  this->Width = source->Width;
  this->Height = source->Height;
  this->Format = format;
  this->InitPixelFormat();
  Pixel = this->Pixel;
  Width = this->Width;
  Height = this->Height;
  this->Start = 0;
  if ( Pixel <= 4 )
  {
    v7 = Pixel * Width;
    v8 = Height * v7;
  }
  else
  {
    v7 = Pixel * ((Width + 3) >> 2);
    v8 = v7 * ((Height + 3) >> 2);
  }
  this->Pitch = v7;
  this->Size = v8;
  this->Data = new unsigned char[v8];
  this->BitBlt(0, 0, this->Width, this->Height, source, 0, 0);
}

//----- (004604B0) --------------------------------------------------------

SBitmap::SBitmap(int width, int height, D3DFORMAT format, int upsidedown)

{
  this->Width = width;
  this->Height = height;
  this->Format = format;
  this->AllocateData(upsidedown);
}

//----- (004604E0) --------------------------------------------------------

SBitmap::SBitmap()

{
  this->Data = 0;
  this->Size = 0;
}

//----- (00460680) --------------------------------------------------------

SShadowMask::SShadowMask(int width, int height, SBitmap *source)

{
  signed int v5;
  int v6;
  unsigned char *v7; // eax
  int v8;
  unsigned char *v9; // ebx
  int Pitch;
  SBitmap *v11; // edx
  unsigned char *v12; // edi
  char *v13; // eax
  char v14; // dl
  char v15; // cl
  int heighta;
  v5 = (height + 3) & 0xFFFFFFFC;
  this->Width = width;
  v6 = (width + 7) >> 3;
  this->Height = v5;
  this->Pitch = v6;
  this->Size = v5 * v6;
  if ( source->Format != D3DFMT_R5G6B5 || source->Height < v5 || source->Width < 8 * v6 )
    Logger.g->Panic("SShadowMask::SShadowMask: invalid source bitmap");
  v7 = new unsigned char[v5 * v6];
  v8 = this->Height;
  this->Data = v7;
  v9 = &source->Data[source->Start];
  // x64: was `widtha = (int)v7; ... v7 = (unsigned char*)widtha; ... widtha
  // = (int)v7` register-spill round-trip; truncates the freshly-allocated
  // Data[] pointer on x64. v7 is never modified inside the inner loop (v12
  // is the iterator), so the restore is redundant.
  if ( v8 )
  {
    Pitch = this->Pitch;
    v11 = source;
    do
    {
      --v8;
      v12 = v7;
      heighta = v8;
      if ( Pitch )
      {
        v13 = (char *)(v9 + 13);
        do
        {
          v14 = v13[2];
          ++v12;
          v15 = *v13;
          v13 += 16;
          *(v12 - 1) = (*(v13 - 24) & 4)
                     + (((unsigned char)*(v13 - 26) >> 1) & 2)
                     + ((*(v13 - 28) & 4) != 0)
                     + 2
                     * ((*(v13 - 22) & 4)
                      + 2 * ((*(v13 - 20) & 4) + 2 * ((*(v13 - 18) & 4) + 2 * ((v15 & 4) + 2 * (v14 & 0xFC)))));
          --Pitch;
        }
        while ( Pitch );
        v8 = heighta;
        v11 = source;
        Pitch = this->Pitch;
      }
      v9 += v11->Pitch;
      v7 += Pitch;
    }
    while ( v8 );
  }
}

//----- (004607B0) --------------------------------------------------------

SShadowMask::SShadowMask(SStream *is)

{
  int Int;
  unsigned char *v4; // eax
  this->Width = is->ReadInt();
  this->Height = is->ReadInt();
  this->Pitch = is->ReadInt();
  Int = is->ReadInt();
  this->Size = Int;
  v4 = new unsigned char[Int];
  this->Data = v4;
  is->Read(v4, this->Size);
}

//----- (00460800) --------------------------------------------------------

SSurfaceBitmap::SSurfaceBitmap(IDirect3DSurface9 *_lpSurface)

{
  HRESULT v4;
  D3DFORMAT Format;
  unsigned char *pBits;
  unsigned int Width;
  const char *v8;
  D3DLOCKED_RECT lockr;
  D3DSURFACE_DESC desc;
  char atmstr[200];
  this->Data = 0;
  this->Size = 0;
  this->lpSurface = _lpSurface;
  v4 = _lpSurface->LockRect(&lockr, 0, 0);
  if ( v4 )
  {
    v8 = DXGetErrorStringA(v4);
    sprintf(atmstr, "%s: %s", "SSurfaceBitmap::SSurfaceBitmap: LockRect", v8);
    Logger.g->Panic(atmstr);
  }
  this->lpSurface->GetDesc(&desc);
  Format = desc.Format;
  this->Height = desc.Height;
  this->Pitch = lockr.Pitch;
  pBits = (unsigned char *)lockr.pBits;
  this->Format = Format;
  Width = desc.Width;
  this->Data = pBits;
  this->Width = Width;
  this->Start = 0;
  this->InitPixelFormat();
}

//----- (00460910) --------------------------------------------------------

STextureBitmap::STextureBitmap(IDirect3DTexture9 *_lpTexture, int level)

{
  HRESULT v5;
  D3DFORMAT Format;
  unsigned char *pBits;
  unsigned int Width;
  const char *v9;
  D3DLOCKED_RECT lockr;
  D3DSURFACE_DESC desc;
  char atmstr[200];
  this->Data = 0;
  this->Size = 0;
  this->lpTexture = _lpTexture;
  this->Level = level;
  v5 = _lpTexture->LockRect(level, &lockr, 0, 0);
  if ( v5 )
  {
    v9 = DXGetErrorStringA(v5);
    sprintf(atmstr, "%s: %s", "STextureBitmap::STextureBitmap: LockRect", v9);
    Logger.g->Panic(atmstr);
  }
  this->lpTexture->GetLevelDesc(this->Level, &desc);
  Format = desc.Format;
  this->Height = desc.Height;
  this->Pitch = lockr.Pitch;
  pBits = (unsigned char *)lockr.pBits;
  this->Format = Format;
  Width = desc.Width;
  this->Data = pBits;
  this->Width = Width;
  this->Start = 0;
  this->InitPixelFormat();
}

//----- (00460A70) --------------------------------------------------------

SBitmap::~SBitmap()

{
  unsigned char *Data; // eax
  Data = this->Data;
  if ( Data )
  {
    delete[] Data;
    this->Data = 0;
  }
}

//----- (00460AB0) --------------------------------------------------------

SShadowMask::~SShadowMask()

{
  unsigned char *Data; // eax
  Data = this->Data;
  if ( Data )
  {
    delete[] Data;
    this->Data = 0;
  }
}

//----- (00460AD0) --------------------------------------------------------

SSurfaceBitmap::~SSurfaceBitmap()

{
  // IDA-style ports invoke this explicitly AND let it run at scope exit,
  // so the dtor must be idempotent - clear lpSurface after UnlockRect
  // so the second call skips a dangling COM method dispatch.
  if ( this->lpSurface )
  {
    this->lpSurface->UnlockRect();
    this->lpSurface = 0;
  }
  this->Data = 0;
}

//----- (00460B20) --------------------------------------------------------

STextureBitmap::~STextureBitmap()

{
  // Same idempotency contract as SSurfaceBitmap above.
  if ( this->lpTexture )
  {
    this->lpTexture->UnlockRect(this->Level);
    this->lpTexture = 0;
  }
  this->Data = 0;
}

//----- (00460B70) --------------------------------------------------------

SBitmap& SBitmap::operator=(SBitmap *src)

{
  unsigned char *Data; // eax
  Data = this->Data;
  if ( Data )
  {
    delete[] Data;
    this->Data = 0;
  }
  memcpy(this, src, sizeof(SBitmap));
  src->Data = 0;
  return *this;
}

//----- (00460BB0) --------------------------------------------------------

void SBitmap::AllocateData(int upsidedown)

{
  int Pixel;
  int Width;
  int Height;
  int v6;
  int v7;
  int v8;
  this->InitPixelFormat();
  Pixel = this->Pixel;
  Width = this->Width;
  Height = this->Height;
  this->Start = 0;
  if ( Pixel <= 4 )
  {
    v6 = Pixel * Width;
    v7 = Height * v6;
  }
  else
  {
    v6 = Pixel * ((Width + 3) >> 2);
    v7 = v6 * ((Height + 3) >> 2);
  }
  this->Pitch = v6;
  this->Size = v7;
  this->Data = new unsigned char[v7];
  if ( upsidedown )
  {
    v8 = this->Size - this->Pitch;
    this->Pitch = -this->Pitch;
    this->Start = v8;
  }
}

//----- (00460C20) --------------------------------------------------------

void SBitmap::BitBlt(int x, int y, int width, int height, SBitmap *source, int src_x, int src_y)

{
  int Pixel;
  int v10;
  int v11;
  int v12;
  int v13;
  int v14;
  int Pitch;
  int v16;
  int v17;
  int v18;
  int v19;
  int v20;
  unsigned char *v21; // edi
  int v22;
  int v23;
  D3DFORMAT Format;
  int v25;
  unsigned char *v26; // eax
  unsigned char *v27; // edx
  int v28;
  unsigned char v29; // cl
  unsigned char *v30; // eax
  unsigned char *v31; // ebx
  unsigned char *v32; // edx
  int v33;
  unsigned char v34; // al
  short v35; // cx
  unsigned char *v36; // eax
  unsigned char *v37; // ebx
  unsigned char *v38; // edx
  int v39;
  unsigned char v40; // al
  short v41; // cx
  unsigned char *v42; // eax
  unsigned char *v43; // ebx
  unsigned char *v44; // edx
  int v45;
  unsigned char v46; // al
  short v47; // cx
  int v48;
  int xa;
  // x64: ya/src_x{a,b,c,d}/src_yc hold byte-addresses derived from heap
  // pointers (`(int)(v21 + 2)` / `(int)src_yb`); on x64 a 4-byte int
  // truncates the upper half of textures allocated above 4 GiB. Use
  // intptr_t so the +Pitch/+source->Pitch row-stride arithmetic
  // preserves the full 8-byte address.
  intptr_t ya;
  int widtha;
  int heighta;
  intptr_t src_xa;
  intptr_t src_xb;
  intptr_t src_xc;
  intptr_t src_xd;
  int src_ya;
  unsigned char *src_yb;
  intptr_t src_yc;
  int src_yd;
  int src_ye;
  int src_yf;
  Pixel = this->Pixel;
  if ( Pixel > 4 && (((unsigned char)y | (unsigned char)x) & 3) != 0 )
    Logger.g->Panic("SBitmap::BitBlt: Invalid alignment for compressed format");
  v48 = source->Pixel;
  if ( v48 > 4 && (((unsigned char)src_y | (unsigned char)src_x) & 3) != 0 )
    Logger.g->Panic("SBitmap::BitBlt: Invalid alignment for compressed format");
  v10 = this->Width - x;
  v11 = width;
  if ( width > v10 )
    v11 = this->Width - x;
  v12 = height;
  if ( height > this->Height - y )
    v12 = this->Height - y;
  v13 = source->Width - src_x;
  if ( v11 <= v13 )
    v13 = v11;
  v14 = source->Height - src_y;
  heighta = v13;
  if ( v12 <= v14 )
    v14 = v12;
  if ( v13 > 0 && v14 > 0 )
  {
    Pitch = this->Pitch;
    v16 = y >> 2;
    if ( Pixel <= 4 )
      v16 = y;
    v17 = x >> 2;
    v18 = Pitch * v16;
    widtha = Pitch;
    if ( Pixel <= 4 )
      v17 = x;
    v19 = Pixel * v17;
    v20 = source->Pitch;
    xa = v20;
    v21 = &this->Data[v19 + v18 + this->Start];
    v22 = src_y >> 2;
    ya = (intptr_t)v21;
    if ( v48 <= 4 )
      v22 = src_y;
    src_ya = v20 * v22;
    v23 = src_x >> 2;
    if ( v48 <= 4 )
      v23 = src_x;
    src_yb = &source->Data[this->Pixel * v23 + src_ya + source->Start];
    Format = source->Format;
    v25 = heighta;
    if ( this->Format == Format )
    {
      if ( this->Pixel > 4 )
      {
        BltSameFormat(v21, Pitch, src_yb, v20, this->Pixel * ((heighta + 3) >> 2), (v14 + 3) >> 2);
        return;
      }
LABEL_78:
      BltSameFormat(v21, Pitch, src_yb, v20, this->Pixel * v25, v14);
      return;
    }
    switch ( this->Format )
    {
      case D3DFMT_X8R8G8B8:
        if ( Format == D3DFMT_A8R8G8B8 )
        {
          BltSameFormat(v21, Pitch, src_yb, v20, this->Pixel * heighta, v14);
          return;
        }
LABEL_85:
        if ( Format == D3DFMT_DXT1 )
          goto LABEL_89;
LABEL_98:
        Logger.g->Panic("SBitmap::BitBlt: Unsupported conversion");
      case D3DFMT_A8R8G8B8:
        if ( Format == D3DFMT_R8G8B8 )
        {
          v26 = v21 + 2;
          v27 = src_yb + 2;
          src_xa = (intptr_t)(v21 + 2);
          src_yc = (intptr_t)(src_yb + 2);
          do
          {
            --v14;
            v28 = v25;
            do
            {
              v29 = *(v27 - 2);
              v27 += 3;
              *(v26 - 2) = v29;
              v26 += 4;
              *(v26 - 5) = *(v27 - 4);
              *(v26 - 4) = *(v27 - 3);
              *(v26 - 3) = -1;
              --v28;
            }
            while ( v28 );
            v27 = (unsigned char *)(v20 + src_yc);
            v26 = (unsigned char *)(widtha + src_xa);
            v25 = heighta;
            src_yc += v20;
            src_xa += widtha;
          }
          while ( v14 );
          return;
        }
        goto LABEL_88;
      case D3DFMT_R8G8B8:
        if ( Format == D3DFMT_X8R8G8B8 || Format == D3DFMT_A8R8G8B8 )
        {
          Blt_R8G8B8_From_X8R8G8B8(v21, Pitch, src_yb, v20, heighta, v14);
          return;
        }
        goto LABEL_98;
    }
    v25 = heighta;
    if ( this->Format == D3DFMT_R5G6B5 )
    {
      if ( Format == D3DFMT_A8R8G8B8 )
      {
        v30 = src_yb + 1;
        src_xb = (intptr_t)(src_yb + 1);
        do
        {
          v31 = v21;
          src_yd = v14 - 1;
          v32 = v30;
          v33 = v25;
          do
          {
            v34 = *v32;
            v31 += 2;
            v35 = v32[1];
            v32 += 4;
            *((_WORD *)v31 - 1) = (*(v32 - 5) >> 3) + 8 * ((v34 & 0xFC) + 32 * (v35 & 0xFFF8));
            --v33;
          }
          while ( v33 );
          v30 = (unsigned char *)(xa + src_xb);
          v21 = (unsigned char *)(widtha + ya);
          v14 = src_yd;
          v25 = heighta;
          src_xb += xa;
          ya += widtha;
        }
        while ( src_yd );
        return;
      }
LABEL_77:
      if ( Format == 257 )
        goto LABEL_78;
      if ( Format == D3DFMT_DXT1 )
      {
        Blt_R5G6B5_From_DXT1(v21, Pitch, src_yb, v20, v25, v14);
        return;
      }
      goto LABEL_98;
    }
    v25 = heighta;
    if ( this->Format == D3DFMT_A1R5G5B5 )
    {
      if ( Format == D3DFMT_A8R8G8B8 )
      {
        v36 = src_yb + 2;
        src_xc = (intptr_t)(src_yb + 2);
        do
        {
          v37 = v21;
          src_ye = v14 - 1;
          v38 = v36;
          v39 = v25;
          do
          {
            v40 = *v38;
            v37 += 2;
            v41 = v38[1];
            v38 += 4;
            *((_WORD *)v37 - 1) = (*(v38 - 6) >> 3)
                                + 4 * ((*(v38 - 5) & 0xF8) + 32 * ((v40 & 0xF8) + 2 * (v41 & 0xFF80)));
            --v39;
          }
          while ( v39 );
          v36 = (unsigned char *)(xa + src_xc);
          v21 = (unsigned char *)(widtha + ya);
          v14 = src_ye;
          v25 = heighta;
          src_xc += xa;
          ya += widtha;
        }
        while ( src_ye );
        return;
      }
LABEL_82:
      if ( Format == D3DFMT_DXT1 )
      {
        Blt_A1R5G5B5_From_DXT1(v21, Pitch, src_yb, v20, v25, v14);
        return;
      }
      goto LABEL_98;
    }
    v25 = heighta;
    if ( this->Format == D3DFMT_A4R4G4B4 )
    {
      if ( Format == D3DFMT_A8R8G8B8 )
      {
        v42 = src_yb + 2;
        src_xd = (intptr_t)(src_yb + 2);
        do
        {
          v43 = v21;
          src_yf = v14 - 1;
          v44 = v42;
          v45 = v25;
          do
          {
            v46 = *v44;
            v43 += 2;
            v47 = v44[1];
            v44 += 4;
            *((_WORD *)v43 - 1) = (*(v44 - 6) >> 4) + (*(v44 - 5) & 0xF0) + 16 * ((v46 & 0xF0) + 16 * (v47 & 0xFFF0));
            --v45;
          }
          while ( v45 );
          v42 = (unsigned char *)(xa + src_xd);
          v21 = (unsigned char *)(widtha + ya);
          v14 = src_yf;
          v25 = heighta;
          src_xd += xa;
          ya += widtha;
        }
        while ( src_yf );
        return;
      }
LABEL_91:
      if ( Format == D3DFMT_DXT5 )
      {
        Blt_A4R4G4B4_From_DXT5(v21, Pitch, src_yb, v20, heighta, v14);
        return;
      }
      goto LABEL_98;
    }
    if ( this->Format == D3DFMT_DXT1 )
    {
      if ( Format == D3DFMT_A8R8G8B8 )
      {
        Blt_DXT_From_A8R8G8B8(v21, Pitch, src_yb, v20, heighta, v14, 1);
        return;
      }
    }
    else
    {
      if ( this->Format == D3DFMT_DXT3 )
      {
        if ( Format == D3DFMT_A8R8G8B8 )
        {
          Blt_DXT_From_A8R8G8B8(v21, Pitch, src_yb, v20, heighta, v14, 3);
          return;
        }
        goto LABEL_98;
      }
      if ( this->Format == D3DFMT_DXT5 )
      {
        if ( Format == D3DFMT_A8R8G8B8 )
        {
          Blt_DXT_From_A8R8G8B8(v21, Pitch, src_yb, v20, heighta, v14, 5);
          return;
        }
        goto LABEL_98;
      }
      if ( this->Format != D3DFMT_DXT1 )
      {
        switch ( this->Format )
        {
          case D3DFMT_R5G6B5:
            goto LABEL_77;
          case D3DFMT_A1R5G5B5:
            goto LABEL_82;
          case D3DFMT_X8R8G8B8:
            goto LABEL_85;
          case D3DFMT_A8R8G8B8:
LABEL_88:
            if ( Format == D3DFMT_DXT1 )
            {
LABEL_89:
              Blt_A8R8G8B8_From_DXT1(v21, Pitch, src_yb, v20, v25, v14);
              return;
            }
LABEL_94:
            if ( Format == D3DFMT_DXT5 )
            {
              Blt_A8R8G8B8_From_DXT5(v21, Pitch, src_yb, v20, v25, v14);
              return;
            }
            goto LABEL_98;
        }
        if ( this->Format != D3DFMT_A4R4G4B4 )
        {
          if ( this->Format != D3DFMT_A8R8G8B8 )
            goto LABEL_98;
          goto LABEL_94;
        }
        goto LABEL_91;
      }
    }
    if ( Format == 256 )
    {
      Blt_DXT_From_SHADOW32(v21, Pitch, src_yb, v20, heighta, v14);
      return;
    }
    if ( Format == 257 )
    {
      Blt_DXT_From_SHADOW16(v21, Pitch, src_yb, v20, heighta, v14);
      return;
    }
    goto LABEL_98;
  }
}

//----- (00464E10) --------------------------------------------------------

HBITMAP SBitmap::CreateWinBitmap()

{
  if ( this->Format != D3DFMT_A8R8G8B8 )
    Logger.g->Panic("SBitmap::CreateWinBitmap: Only 32 bit bitmap can be converted");
  return CreateBitmap(this->Width, this->Height, 1u, 0x20u, this->Data);
}

//----- (00464E40) --------------------------------------------------------

int SBitmap::GetLogicalSize()

{
  int Pixel;
  int Width;
  int Height;
  Pixel = this->Pixel;
  Width = this->Width;
  Height = this->Height;
  if ( Pixel <= 4 )
    return Height * Pixel * Width;
  else
    return Pixel * ((Height + 3) >> 2) * ((Width + 3) >> 2);
}

//----- (00464E70) --------------------------------------------------------

void SBitmap::InitPixelFormat()

{
  D3DFORMAT Format;
  Format = this->Format;
  if ( Format > D3DFMT_DXT2 )
  {
    if ( Format != D3DFMT_DXT3 && Format != D3DFMT_DXT4 && Format != D3DFMT_DXT5 )
      goto LABEL_16;
    goto LABEL_15;
  }
  if ( Format == D3DFMT_DXT2 )
  {
LABEL_15:
    this->Pixel = 16;
    return;
  }
  if ( Format > D3DFMT_A4R4G4B4 )
  {
    if ( Format != D3DFMT_DXT1 )
      goto LABEL_16;
    this->Pixel = 8;
  }
  else if ( Format >= D3DFMT_R5G6B5 )
  {
    this->Pixel = 2;
  }
  else
  {
    if ( Format != D3DFMT_R8G8B8 )
    {
      if ( (unsigned int)(Format - 21) <= 1 )
      {
        this->Pixel = 4;
        return;
      }
LABEL_16:
      Logger.g->Panic("SBitmap::InitPixelFormat: Bitmap format unsupported");
    }
    this->Pixel = 3;
  }
}

//----- (00465130) --------------------------------------------------------

char SBitmap::LoadPNG(const char *filename, const char *panicstr)

{
  unsigned char *v7;
  int Pixel;
  int v9;
  int v10;
  int Height;
  int v12;
  unsigned char *Data;
  unsigned char v14;
  stbi_io_callbacks io_callbacks;
  const char *v16;
  const char *v17;
  v17 = panicstr;
  v16 = filename;
#ifdef HDB_MISSING_ASSET_FALLBACK
  // Pass nullptr so OpenRead never panics; we substitute a procedural
  // placeholder bitmap below if the file is missing.
  SStream *v5 = (SStream *)FileSystem.OpenRead(filename, nullptr);
  if ( !v5 ) {
    Logger.g->Log(0, "MISSING IMAGE: %s -> procedural placeholder", filename);
    this->FillMissingPlaceholder(32, 32);
    return 1;
  }
#else
  SStream *v5 = (SStream *)FileSystem.OpenRead(filename, panicstr);
  if ( !v5 )
    return 0;
#endif
  io_callbacks.read = stbi_read_stream;
  io_callbacks.skip = stbi_skip_stream;
  io_callbacks.eof = stbi_eof_stream;
  v7 = stbi_load_from_callbacks(&io_callbacks, v5, &this->Width, &this->Height, &this->Pixel, 0);
  Pixel = this->Pixel;
  this->Data = v7;
  if ( Pixel >= 3 )
  {
    this->Format = (D3DFORMAT)((Pixel != 3) + 20);
    v9 = this->Pixel * this->Width;
    v10 = 0;
    Height = this->Height;
    this->Start = 0;
    v12 = v9 * Height;
    this->Pitch = v9;
    this->Size = v12;
    if ( v12 > 0 )
    {
      do
      {
        Data = this->Data;
        v14 = Data[v10];
        Data[v10] = Data[v10 + 2];
        this->Data[v10 + 2] = v14;
        v10 += this->Pixel;
      }
      while ( v10 < this->Size );
    }
    v5->Release();
    return 1;
  }
  else
  {
    if ( v17 )
      Logger.g->Panic("%s: Error loading %s: Unsupported PNG format", v17, v16);
    if ( v7 )
    {
      delete[] v7;
      this->Data = 0;
    }
    v5->Release();
    return 0;
  }
}

//----- (004652A0) --------------------------------------------------------

char SBitmap::LoadTGA(char *filename, char *panicstr)

{
  int v7;
  int v8;
  unsigned char v9;
  const char *filenamea;
  const char *panicstra;
  unsigned char head[20];
  filenamea = filename;
  panicstra = panicstr;
#ifdef HDB_MISSING_ASSET_FALLBACK
  // Pass nullptr so OpenRead never panics; we substitute a procedural
  // placeholder bitmap below if the file is missing.
  SStream *v5 = (SStream *)FileSystem.OpenRead(filename, nullptr);
  if ( !v5 ) {
    Logger.g->Log(0, "MISSING IMAGE: %s -> procedural placeholder", filename);
    this->FillMissingPlaceholder(32, 32);
    return 1;
  }
#else
  SStream *v5 = (SStream *)FileSystem.OpenRead(filename, panicstr);
  if ( !v5 )
    return 0;
#endif
  v5->Read(head, 18);
  if ( head[0]
    || head[1]
    || head[2] != 2
    || head[3]
    || head[4]
    || head[5]
    || head[6]
    || head[8]
    || head[9]
    || head[10]
    || head[11] )
  {
    throw "Not a TGA file";
  }
  v7 = head[14];
  this->Width = head[12] + (head[13] << 8);
  v8 = v7 + (head[15] << 8);
  v9 = head[16];
  this->Height = v8;
  switch ( v9 )
  {
    case 0x10u:
      if ( (head[17] & 0x1F) != 1 )
      {
        throw "Unsupported format";
      }
      this->Format = D3DFMT_X1R5G5B5;
      break;
    case 0x18u:
      this->Format = D3DFMT_R8G8B8;
      break;
    case 0x20u:
      this->Format = D3DFMT_A8R8G8B8;
      break;
    default:
      throw "Unsupported bit depth";
  }
  this->AllocateData((head[17] & 0x20) == 0);
  v5->Read(this->Data, this->Size);
  v5->Release();
  return 1;
}

#ifdef HDB_MISSING_ASSET_FALLBACK
// Synthesize a magenta-and-black checker bitmap for missing .png / .tga
// loads. 32-bit BGRA matches the most common A8R8G8B8 path; cell size =
// max(1, min(width,height) / 4) so even 16x16 atlases get a pattern.
void SBitmap::FillMissingPlaceholder(int width, int height)
{
  if (this->Data) {
    delete[] this->Data;
    this->Data = 0;
  }
  this->Width = width;
  this->Height = height;
  this->Format = D3DFMT_A8R8G8B8;
  this->Pixel = 4;
  this->Pitch = width * 4;
  this->Size = this->Pitch * height;
  this->Start = 0;
  this->Data = new unsigned char[this->Size];
  int cell = width < height ? width : height;
  cell /= 4;
  if (cell < 1) cell = 1;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      bool magenta = (((x / cell) ^ (y / cell)) & 1) == 0;
      unsigned char *p = this->Data + y * this->Pitch + x * 4;
      // BGRA byte order for D3DFMT_A8R8G8B8
      p[0] = magenta ? 0xFF : 0x00; // B
      p[1] = 0x00;                  // G
      p[2] = magenta ? 0xFF : 0x00; // R
      p[3] = 0xFF;                  // A
    }
  }
}
#endif // HDB_MISSING_ASSET_FALLBACK

//----- (00465710) --------------------------------------------------------

void SShadowMask::MakeBitmap(SBitmap *destination)

{
  SBitmap *v2; // ebx
  SShadowMask *v3; // edi
  int Height;
  int Width;
  D3DFORMAT Format;
  unsigned char *v7; // esi
  unsigned char *Data; // edx
  int v9;
  unsigned char *v10; // edx
  int Pitch;
  unsigned char v12; // bl
  unsigned char v13; // di
  unsigned char *v14; // eax
  unsigned char v15; // si
  int v16;
  unsigned char *v17; // ecx
  unsigned int v18;
  int v19;
  unsigned char *v20; // ecx
  unsigned int v21;
  unsigned char *v22; // ecx
  int v23;
  int j;
  int v25;
  unsigned char *v26; // eax
  int v27;
  unsigned char *v28; // ecx
  unsigned char *v29; // eax
  int v30;
  unsigned char *v31; // edx
  unsigned char *v32; // ebx
  char v33; // cl
  int v34;
  unsigned char *v35; // ecx
  unsigned char *v36; // edx
  int v37;
  unsigned char *v38; // eax
  unsigned char *v39; // ebx
  char v40; // dl
  unsigned char *v41;
  unsigned char *v42;
  unsigned char *v43;
  unsigned char *v44;
  int v45;
  int k;
  int v47;
  int i;
  unsigned int *v50;
  unsigned char *v51;
  unsigned char *v52;
  unsigned char *v53;
  v2 = destination;
  v3 = this;
  Height = destination->Height;
  if ( Height < this->Height )
    goto LABEL_42;
  Width = destination->Width;
  if ( destination->Width < 8 * v3->Pitch )
    goto LABEL_42;
  Format = destination->Format;
  if ( Format == D3DFMT_DXT1 )
  {
    v7 = &destination->Data[destination->Start];
    Data = v3->Data;
    v44 = Data;
    v41 = v7;
    for ( i = 0; i < v3->Height; i += 4 )
    {
      v9 = 0;
      v51 = Data;
      v10 = v7;
      v47 = 0;
      if ( v3->Width > 0 )
      {
        do
        {
          *(_DWORD *)v10 = 0xFFFF;
          Pitch = v3->Pitch;
          v50 = (unsigned int *)(v10 + 4);
          v12 = *v51;
          v13 = v51[Pitch];
          v14 = &v51[Pitch + Pitch];
          v15 = *v14;
          v16 = v14[Pitch];
          v17 = &v14[Pitch + -3 * Pitch];
          *v50 = (~((2 * ((v12 & 1) + 2 * ((v12 & 2) + 2 * ((v12 & 4) + 2 * (v12 & 8))))) | (((v13 & 1)
                                                                                            + 2
                                                                                            * ((v13 & 2)
                                                                                             + 2
                                                                                             * ((v13 & 4) + 2 * (v13 & 8)))) << 9) | (((v15 & 1) + 2 * ((v15 & 2) + 2 * ((v15 & 4) + 2 * (v15 & 8)))) << 17) | (((v16 & 1) + 2 * ((v16 & 2) + 2 * ((v16 & 4) + 2 * (v16 & 0xFFFFFFF8)))) << 25)) >> 1) & 0x55555555;
          v50[1] = 0xFFFF;
          v18 = *v17;
          v19 = this->Pitch;
          v20 = &v17[v19];
          v21 = *v20;
          v22 = &v20[v19];
          v23 = (((v21 & 0x80) + ((v21 >> 3) & 2) + ((v21 >> 2) & 8) + ((v21 >> 1) & 0x20)) << 8) | (((v22[v19] & 0x80) + ((v22[v19] >> 3) & 2) + ((v22[v19] >> 2) & 8) + ((v22[v19] >> 1) & 0x20)) << 24);
          v51 = &v22[v19 + 1 - 3 * v19];
          v9 = v47 + 8;
          v50[2] = (~(((v18 & 0x80) + ((v18 >> 3) & 2) + ((v18 >> 2) & 8) + ((v18 >> 1) & 0x20)) | (((*v22 & 0x80) + ((*v22 >> 3) & 2) + ((*v22 >> 2) & 8) + ((*v22 >> 1) & 0x20)) << 16) | v23) >> 1) & 0x55555555;
          v10 = (unsigned char *)(v50 + 3);
          v3 = this;
          v47 = v9;
        }
        while ( v9 < this->Width );
        v2 = destination;
        v7 = v41;
      }
      for ( Width = v2->Width; v9 < v2->Width; Width = v2->Width )
      {
        *(_DWORD *)v10 = 0xFFFF;
        v10 += 8;
        *((_DWORD *)v10 - 1) = 1431655765;
        v9 += 4;
      }
      v7 += v2->Pitch;
      v41 = v7;
      Data = &v44[4 * v3->Pitch];
      v44 = Data;
    }
    for ( j = i; j < v2->Height; j += 4 )
    {
      v25 = 0;
      v26 = v7;
      if ( Width > 0 )
      {
        do
        {
          *(_DWORD *)v26 = 0xFFFF;
          v26 += 8;
          *((_DWORD *)v26 - 1) = 1431655765;
          v25 += 4;
          Width = v2->Width;
        }
        while ( v25 < v2->Width );
      }
      v7 += v2->Pitch;
    }
    return;
  }
  if ( Format == D3DFMT_R5G6B5 )
  {
    v27 = 0;
    v28 = &destination->Data[destination->Start];
    v29 = v3->Data;
    v42 = v29;
    v52 = v28;
    v45 = 0;
    if ( v3->Height > 0 )
    {
      do
      {
        v30 = 0;
        v31 = v28;
        if ( v3->Width > 0 )
        {
          v32 = v29;
          do
          {
            v33 = *v32++;
            *(_WORD *)v31 = -((v33 & 1) != 0);
            *((_WORD *)v31 + 1) = -((v33 & 2) != 0);
            *((_WORD *)v31 + 2) = -((v33 & 4) != 0);
            *((_WORD *)v31 + 3) = -((v33 & 8) != 0);
            *((_WORD *)v31 + 4) = -((v33 & 0x10) != 0);
            *((_WORD *)v31 + 5) = -((v33 & 0x20) != 0);
            v30 += 8;
            *((_WORD *)v31 + 6) = -((v33 & 0x40) != 0);
            *((_WORD *)v31 + 7) = v33 >> 15;
            v31 += 16;
          }
          while ( v30 < v3->Width );
          v2 = destination;
          v28 = v52;
          v29 = v42;
        }
        if ( v30 < v2->Width )
        {
          do
          {
            v31 += 2;
            *((_WORD *)v31 - 1) = 0;
            ++v30;
          }
          while ( v30 < v2->Width );
          v3 = this;
        }
        v29 += v3->Pitch;
        v27 = v45 + 1;
        v28 += v2->Pitch;
        v42 = v29;
        v52 = v28;
        v45 = v27;
      }
      while ( v27 < v3->Height );
      Height = v2->Height;
    }
    if ( v27 < Height )
    {
      do
      {
        memset(v28, 0, 2 * v2->Width);
        v27 += 4;
        v28 = &v52[v2->Pitch];
        v52 = v28;
      }
      while ( v27 < v2->Height );
    }
    return;
  }
  if ( Format != D3DFMT_A8R8G8B8 )
LABEL_42:
    Logger.g->Panic("SShadowMask::MakeBitmap: invalid destination bitmap");
  v34 = 0;
  v35 = &destination->Data[destination->Start];
  v36 = v3->Data;
  v43 = v36;
  v53 = v35;
  for ( k = 0; v34 < v3->Height; k = v34 )
  {
    v37 = 0;
    v38 = v35;
    if ( v3->Width > 0 )
    {
      v39 = v36;
      do
      {
        v40 = *v39++;
        *(_DWORD *)v38 = -((v40 & 1) != 0);
        *((_DWORD *)v38 + 1) = -((v40 & 2) != 0);
        *((_DWORD *)v38 + 2) = -((v40 & 4) != 0);
        *((_DWORD *)v38 + 3) = -((v40 & 8) != 0);
        *((_DWORD *)v38 + 4) = -((v40 & 0x10) != 0);
        *((_DWORD *)v38 + 5) = -((v40 & 0x20) != 0);
        v37 += 8;
        *((_DWORD *)v38 + 6) = -((v40 & 0x40) != 0);
        *((_DWORD *)v38 + 7) = v40 >> 31;
        v38 += 32;
      }
      while ( v37 < v3->Width );
      v2 = destination;
      v35 = v53;
      v36 = v43;
    }
    for ( ; v37 < v2->Width; ++v37 )
    {
      *(_DWORD *)v38 = 0;
      v38 += 4;
    }
    v36 += v3->Pitch;
    v34 = k + 1;
    v35 += v2->Pitch;
    v43 = v36;
    v53 = v35;
  }
  for ( ; v34 < v2->Height; v53 = v35 )
  {
    memset(v35, 0, 4 * v2->Width);
    v34 += 4;
    v35 = &v53[v2->Pitch];
  }
}

//----- (00465C70) --------------------------------------------------------

void SBitmap::MakeInverseOpaque()

{
  int v2;
  int Width;
  int v4;
  unsigned char *v5; // eax
  int v6;
  if ( this->Format == D3DFMT_A8R8G8B8 )
  {
    v2 = 0;
    if ( this->Height > 0 )
    {
      Width = this->Width;
      do
      {
        v4 = 0;
        v5 = &this->Data[v2 * this->Pitch + this->Start];
        if ( Width > 0 )
        {
          do
          {
            v6 = *(_DWORD *)v5;
            v5 += 4;
            ++v4;
            *((_DWORD *)v5 - 1) = ~(v6 & 0xFFFFFF);
            Width = this->Width;
          }
          while ( v4 < this->Width );
        }
        ++v2;
      }
      while ( v2 < this->Height );
    }
  }
}

//----- (00465CC0) --------------------------------------------------------

void SBitmap::MakeOpaque()

{
  int v2;
  int Width;
  unsigned char *v4; // ecx
  int v5;
  if ( this->Format == D3DFMT_A8R8G8B8 )
  {
    v2 = 0;
    if ( this->Height > 0 )
    {
      Width = this->Width;
      do
      {
        v4 = &this->Data[v2 * this->Pitch + 3 + this->Start];
        v5 = 0;
        if ( Width > 0 )
        {
          do
          {
            *v4 = -1;
            v4 += 4;
            Width = this->Width;
            ++v5;
          }
          while ( v5 < this->Width );
        }
        ++v2;
      }
      while ( v2 < this->Height );
    }
  }
}

//----- (00465FF0) --------------------------------------------------------

void SBitmap::NextMipLevel()

{
  int Width;
  int Height;
  bool v4;
  int v5; // kr0C_4
  unsigned char *v6; // eax
  unsigned char *v7; // ecx
  int v8;
  int Pitch;
  int v10;
  unsigned char *v11; // edi
  unsigned char *v12; // eax
  unsigned char *v13; // ecx
  unsigned char *v14; // edi
  int v15;
  int v16;
  int v17;
  unsigned char *v18; // edx
  int v19;
  unsigned char *v20; // eax
  unsigned char *Data; // edx
  unsigned char *v22; // edi
  int v23;
  unsigned char *v24; // eax
  int v25;
  unsigned char *v26; // eax
  int v27;
  int v28;
  unsigned char *v29;
  unsigned char *v30;
  unsigned char *v31;
  unsigned char *v32;
  unsigned char *v33;
  if ( this->Format != D3DFMT_A8R8G8B8 )
    Logger.g->Panic("SBitmap::NextMipLevel: Bitmap should be 32 bit ARGB");
  Width = this->Width;
  if ( this->Width != 1 && (Width & 1) != 0 || (Height = this->Height, Height != 1) && (Height & 1) != 0 )
    Logger.g->Panic(
      "SBitmap::NextMipLevel: Bitmap size is unsupported (%d x %d)",
      Width,
      this->Height);
  v4 = Width == 1;
  if ( Width > 1 )
  {
    if ( Height > 1 )
    {
      v5 = this->Width;
      this->Height = Height / 2;
      this->Width = v5 / 2;
      v6 = new unsigned char[4 * v5 / 2 * (Height / 2)];
      v7 = &this->Data[this->Start];
      v8 = this->Height;
      v32 = v6;
      v33 = v6;
      v29 = v7;
      if ( v8 )
      {
        Pitch = this->Pitch;
        do
        {
          v10 = this->Width;
          --v8;
          v11 = v7;
          if ( this->Width )
          {
            do
            {
              v11 += 8;
              *v33 = (*(v11 - 8) + v11[this->Pitch - 4] + *(v11 - 4) + 2 + (unsigned int)v11[this->Pitch - 8]) >> 2;
              v33[1] = (v11[this->Pitch - 7] + v11[this->Pitch - 3] + *(v11 - 7) + 2 + (unsigned int)*(v11 - 3)) >> 2;
              v33[2] = (v11[this->Pitch - 2] + *(v11 - 2) + *(v11 - 6) + 2 + (unsigned int)v11[this->Pitch - 6]) >> 2;
              v33[3] = (v11[this->Pitch - 1] + *(v11 - 1) + *(v11 - 5) + (unsigned int)v11[this->Pitch - 5] + 2) >> 2;
              v33 += 4;
              --v10;
            }
            while ( v10 );
            Pitch = this->Pitch;
            v7 = v29;
          }
          v7 += 2 * Pitch;
          v29 = v7;
        }
        while ( v8 );
      }
LABEL_32:
      Data = this->Data;
      goto LABEL_33;
    }
    v4 = Width == 1;
  }
  if ( v4 )
  {
    if ( Height != 1 )
    {
      this->Height = Height / 2;
      v12 = new unsigned char[4 * (Height / 2)];
      v13 = &this->Data[this->Start];
      v14 = v12;
      v15 = this->Height;
      v32 = v12;
      v30 = v13;
      if ( v15 )
      {
        v16 = this->Pitch;
        do
        {
          v17 = this->Width;
          v28 = v15 - 1;
          v18 = v13;
          if ( this->Width )
          {
            do
            {
              v18 += 4;
              *v14 = (v18[this->Pitch - 4] + 1 + (unsigned int)*(v18 - 4)) >> 1;
              v14[1] = (v18[this->Pitch - 3] + 1 + (unsigned int)*(v18 - 3)) >> 1;
              v14[2] = (v18[this->Pitch - 2] + 1 + (unsigned int)*(v18 - 2)) >> 1;
              v14[3] = (v18[this->Pitch - 1] + (unsigned int)*(v18 - 1) + 1) >> 1;
              v14 += 4;
              --v17;
            }
            while ( v17 );
            v16 = this->Pitch;
            v13 = v30;
          }
          v15 = v28;
          v13 += 2 * v16;
          v30 = v13;
        }
        while ( v28 );
      }
      goto LABEL_32;
    }
LABEL_34:
    Logger.g->Panic("SBitmap::NextMipLevel: 1x1 Bitmap cannot be shrinked");
  }
  if ( Height != 1 )
    goto LABEL_34;
  v19 = Width / 2;
  this->Width = v19;
  v20 = new unsigned char[4 * v19];
  Data = this->Data;
  v22 = v20;
  v23 = this->Height;
  v32 = v20;
  v24 = &Data[this->Start];
  v31 = v24;
  if ( v23 )
  {
    do
    {
      v25 = this->Width;
      --v23;
      if ( this->Width )
      {
        v26 = v24 + 5;
        do
        {
          v27 = *(v26 - 5);
          v26 += 8;
          *v22 = (v27 + 1 + (unsigned int)*(v26 - 9)) >> 1;
          v22[1] = (*(v26 - 12) + 1 + (unsigned int)*(v26 - 8)) >> 1;
          v22[2] = (*(v26 - 11) + 1 + (unsigned int)*(v26 - 7)) >> 1;
          v22[3] = (*(v26 - 10) + (unsigned int)*(v26 - 6) + 1) >> 1;
          v22 += 4;
          --v25;
        }
        while ( v25 );
        v24 = v31;
      }
      v24 += this->Pitch;
      v31 = v24;
    }
    while ( v23 );
    goto LABEL_32;
  }
LABEL_33:
  this->Pitch = 4 * this->Width;
  delete[] Data;
  this->Data = v32;
  this->Start = 0;
}

//----- (00466310) --------------------------------------------------------

void SBitmap::Rotate(int angle)

{
  D3DFORMAT Format;
  int Height;
  unsigned char *v5; // edi
  int v6;
  int v7;
  int Pitch;
  int v9;
  int v10;
  unsigned char *i; // ecx
  int v12;
  int v13;
  int v14;
  unsigned char *j; // ecx
  int v16;
  int v17;
  int v18;
  unsigned char *k; // ecx
  unsigned char *Data;
  unsigned char *v21;
  int anglea;
  int angleb;
  int anglec;
  Format = this->Format;
  if ( Format != D3DFMT_A8R8G8B8 && Format != D3DFMT_X8R8G8B8 )
    Logger.g->Panic("SBitmap::Rotate: Bitmap should be 32 bit");
  Height = this->Height;
  if ( this->Width != Height )
    Logger.g->Panic("SBitmap::Rotate: Bitmap must be square");
  if ( angle )
  {
    v21 = new unsigned char[4 * Height * this->Width];
    v5 = v21;
    switch ( angle )
    {
      case 1:
        v6 = this->Height;
        v7 = 0;
        anglea = 0;
        if ( v6 > 0 )
        {
          Pitch = this->Pitch;
          do
          {
            v9 = Pitch * (v6 - 1) + 4 * v7;
            v10 = 0;
            for ( i = &this->Data[v9 + this->Start]; v10 < this->Width; i -= Pitch )
            {
              ++v10;
              *(_DWORD *)v5 = *(_DWORD *)i;
              v5 += 4;
              Pitch = this->Pitch;
            }
            v6 = this->Height;
            v7 = anglea + 1;
            anglea = v7;
          }
          while ( v7 < v6 );
        }
        break;
      case 2:
        v12 = this->Height;
        v13 = 0;
        for ( angleb = 0; v13 < v12; angleb = v13 )
        {
          v14 = 0;
          for ( j = &this->Data[4 * this->Width - 4 + this->Pitch * (v12 - v13 - 1) + this->Start];
                v14 < this->Width;
                v5 += 4 )
          {
            v16 = *(_DWORD *)j;
            j -= 4;
            *(_DWORD *)v5 = v16;
            ++v14;
          }
          v12 = this->Height;
          v13 = angleb + 1;
        }
        break;
      case 3:
        v17 = 0;
        for ( anglec = 0; v17 < this->Height; anglec = v17 )
        {
          v18 = 0;
          for ( k = &this->Data[4 * (this->Width - v17) - 4 + this->Start]; v18 < this->Width; k += this->Pitch )
          {
            ++v18;
            *(_DWORD *)v5 = *(_DWORD *)k;
            v5 += 4;
          }
          v17 = anglec + 1;
        }
        break;
    }
    Data = this->Data;
    this->Pitch = 4 * this->Width;
    delete[] Data;
    this->Data = v21;
    this->Start = 0;
  }
}

//----- (004664E0) --------------------------------------------------------

void SShadowMask::Save(SStream *is)

{
  is->WriteInt(this->Width);
  is->WriteInt(this->Height);
  is->WriteInt(this->Pitch);
  is->WriteInt(this->Size);
  is->Write(this->Data, this->Size);
}

//----- (00466530) --------------------------------------------------------

char SBitmap::SaveTGA(char *filename, char *panicstr)

{
  D3DFORMAT Format;
  unsigned char Width;
  unsigned char *v9;
  int i;
  const char *filenamea;
  const char *panicstra;
  int v14;
  SStream *is;
  unsigned char v16;
  unsigned char head[20];
  Format = this->Format;
  filenamea = filename;
  panicstra = panicstr;
  switch ( Format )
  {
    case D3DFMT_R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_R5G6B5:
      v16 = 32;
      break;
    case D3DFMT_A8R8G8B8:
      v16 = 40;
      break;
    default:
      Logger.g->Panic("SBitmap::SaveTGA: Bitmap format unsupported");
  }
  is = (SStream *)FileSystem.OpenWrite(filename, panicstr);
  if ( !is )
    return 0;
  Width = this->Width;
  memset(head, 0, 18);
  head[12] = Width;
  head[13] = BYTE1(this->Width);
  head[14] = this->Height;
  head[15] = BYTE1(this->Height);
  head[16] = 8 * LOBYTE(this->Pixel);
  head[2] = 2;
  head[17] = v16;
  is->Write(head, 18);
  v9 = &this->Data[this->Start];
  for ( i = 0; ; i = v14 + 1 )
  {
    v14 = i;
    if ( i >= this->Height )
      break;
    is->Write(v9, this->Width * this->Pixel);
    v9 += this->Pitch;
  }
  is->Release();
  return 1;
}

