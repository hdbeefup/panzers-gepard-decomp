// 3dengine/animation.cpp
// SAnimation — frame animation loaded from .ANI files
// Decompiled from: gameSplit/sanimation.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <string.h>
#include "animation.h"
#include "logger.h"
#include "stream.h"

extern SFileSystem FileSystem;
extern "C" int ZSTD_decompress(void *dst, int dstCapacity, const void *src, int srcSize);

// Classes: SAnimation
// Function count: 4

//----- (004603D0) --------------------------------------------------------

SAnimation::SAnimation()

{
  this->Data = 0;
  this->Size = 0;
  this->Buffer = 0;
}

//----- (00460A30) --------------------------------------------------------

SAnimation::~SAnimation()

{
  if ( this->Buffer )
  {
    delete[] this->Buffer;
    this->Buffer = 0;
  }
  if ( this->Data )
  {
    delete[] this->Data;
    this->Data = 0;
  }
}

//----- (00464EF0) --------------------------------------------------------

void SAnimation::LoadANI(int mode, const char *filename, const char *panicstr)

{
  SStream *is = FileSystem.OpenRead(filename, panicstr);
  if ( !is )
    return;

  is->ReadSignature();
  int chunkId = is->ReadChunkHeader();

  // 0x46494E41 = "ANIF" (old format), 0x32494E41 = "ANI2" (ZSTD compressed)
  if ( chunkId == 0x46494E41 )
  {
    this->Anim2Format = false;
  }
  else if ( chunkId == 0x32494E41 )
  {
    this->Anim2Format = true;
  }
  else
  {
    throw "Not an ANI file";
  }

  this->Width = is->ReadInt();
  this->Height = is->ReadInt();
  this->Frames = is->ReadInt();

  this->Format = D3DFMT_DXT1;
  this->InitPixelFormat();

  int Pixel = this->Pixel;
  int Width = this->Width;
  this->Start = 0;

  int pitch, height;
  if ( Pixel <= 4 )
  {
    pitch = Pixel * Width;
    height = this->Height;
  }
  else
  {
    pitch = Pixel * ((Width + 3) >> 2);
    height = (this->Height + 3) >> 2;
  }
  this->Pitch = pitch;
  this->Size = pitch * height;

  unsigned char *data = new unsigned char[pitch * height];
  this->Data = data;
  memset(&data[this->Start], 0, 8 * ((this->Width + 3) / 4) * ((this->Height + 3) / 4));

  int chunkRemain = is->ReadChunkRemain();
  this->BufferLen = chunkRemain;
  unsigned char *buf = new unsigned char[chunkRemain];
  this->Buffer = buf;
  is->Read(buf, this->BufferLen);

  this->BufferPtr = 0;
  this->CurrentFrame = 0;

  is->ReadChunkValidate(mode);
  is->Release();

  if ( this->Anim2Format )
  {
    int decompSize = 8 * this->Frames * ((this->Width + 3) / 4) * ((this->Height + 3) / 4);
    unsigned char *decompBuf = new unsigned char[decompSize];
    if ( ZSTD_decompress(decompBuf, decompSize, this->Buffer, this->BufferLen) != decompSize )
      Logger.g->Panic("SAnimation::LoadANI: Data corruption");
    delete[] this->Buffer;
    this->Buffer = decompBuf;
    this->BufferLen = decompSize;
  }
}

//----- (00465D10) --------------------------------------------------------

bool SAnimation::NextFrame(bool looping)

{
  int frame = this->CurrentFrame;

  if ( frame >= this->Frames )
  {
    if ( !looping )
      return false;
    memset(&this->Data[this->Start], 0, 8 * ((this->Width + 3) / 4) * ((this->Height + 3) / 4));
    this->BufferPtr = 0;
    this->CurrentFrame = 0;
    frame = 0;
  }

  int bufPtr = this->BufferPtr;
  int blockW = (this->Width + 3) / 4;
  int blockH = (this->Height + 3) / 4;
  int numBlocks = blockW * blockH;
  unsigned char *src = &this->Buffer[bufPtr];

  if ( this->Anim2Format )
  {
    // ANI2 format: direct DXT1 block copy — interleave color+alpha
    unsigned char *colorSrc = src;
    unsigned char *alphaSrc = &src[4 * numBlocks];
    unsigned char *dst = &this->Data[this->Start];

    if ( numBlocks <= 0 )
    {
      this->BufferPtr = bufPtr + 8 * numBlocks;
      this->CurrentFrame = frame + 1;
      return true;
    }

    for (int i = 0; i < numBlocks; i++)
    {
      *(int *)dst = *(int *)colorSrc;
      colorSrc += 4;
      *((int *)dst + 1) = *(int *)alphaSrc;
      alphaSrc += 4;
      dst += 8;
    }

    this->BufferPtr += 8 * numBlocks;
    this->CurrentFrame = frame + 1;
    return true;
  }
  else
  {
    // ANIA format: RLE-coded delta compression
    // Decode control bytes
    char *codes = new char[numBlocks];
    int codePos = 0;
    int decodedTotal = 0;

    if ( numBlocks > 0 )
    {
      do
      {
        unsigned int byte = *src++;
        unsigned int runLen = (byte >> 3) + 1;
        int code = byte & 7;
        // Fill codes with this code for runLen entries
        for (unsigned int r = 0; r < (runLen >> 2); r++)
          *(int *)&codes[decodedTotal + r * 4] = code * 0x01010101;
        for (unsigned int r = 0; r < (runLen & 3); r++)
          codes[decodedTotal + 4 * (runLen >> 2) + r] = (char)code;
        decodedTotal += runLen;
      }
      while ( decodedTotal < numBlocks );
    }

    if ( decodedTotal != numBlocks )
      Logger.g->Panic("SAnimation::NextFrame: Code buffer overrun");

    // Apply delta operations
    unsigned char *dst = &this->Data[this->Start];
    unsigned char *prev = dst - 8;

    for (int i = 0; i < numBlocks; i++)
    {
      switch ( codes[i] )
      {
      case 0: // Full new block (8 bytes)
        dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
        dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
        dst += 8;
        src += 8;
        break;
      case 1: // Skip (keep previous frame data)
        dst += 8;
        break;
      case 2: // Zero alpha half
        *(int *)(dst + 4) = 0;
        dst += 8;
        break;
      case 3: // New alpha half (4 bytes)
        dst[4] = src[0]; dst[5] = src[1]; dst[6] = src[2]; dst[7] = src[3];
        dst += 8;
        src += 4;
        break;
      case 4: // Copy both halves from previous block
        *(int *)dst = *(int *)prev;
        *((int *)dst + 1) = *((int *)prev + 1);
        dst += 8;
        break;
      case 5: // Copy color from previous, zero alpha
        *(int *)dst = *(int *)prev;
        *(int *)(dst + 4) = 0;
        dst += 8;
        break;
      case 6: // Copy color from previous, new alpha (4 bytes)
        *(int *)dst = *(int *)prev;
        dst[4] = src[0]; dst[5] = src[1]; dst[6] = src[2]; dst[7] = src[3];
        dst += 8;
        src += 4;
        break;
      case 7: // RLE color (2 bytes repeated), zero alpha
        dst[0] = src[0]; dst[1] = src[1];
        dst[2] = src[0]; dst[3] = src[1];
        src += 2;
        *(int *)(dst + 4) = 0;
        dst += 8;
        break;
      default:
        Logger.g->Panic("SAnimation::NextFrame: Invalid code");
      }
      prev += 8;
      if ( src > &this->Buffer[this->BufferLen] )
        Logger.g->Panic("SAnimation::NextFrame: Data buffer overrun");
    }

    delete[] codes;
    this->BufferPtr = (int)(src - this->Buffer);
    this->CurrentFrame = frame + 1;
    return true;
  }
}
