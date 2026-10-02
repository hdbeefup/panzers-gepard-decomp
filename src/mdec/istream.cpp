// mdec/istream.cpp
// Bitstream input/output for MP3 decoder
// Decompiled from: gameSplit/mp3_decoder.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <corecrt_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "decode.h"

// Classes: SMpegAudioDecoder
// Function count: 13

//----- (0046F7B0) --------------------------------------------------------

void SMpegAudioDecoder::in_close()

{

  this->in_file->Release();

}



//----- (0046F7C0) --------------------------------------------------------

BOOL SMpegAudioDecoder::in_fillbuf(int minsize)

{

  int in_end; // eax

  int in_p; // ecx

  signed int v5; // eax

  int v7; // esi

  SStream *in_file; // ecx

  int v9; // eax

  int v10; // ecx



  in_end = this->in_end;

  in_p = this->in_p;

  v5 = in_end - in_p;

  if ( v5 >= minsize )

    return 1;

  memmove(this->in_buf, &this->in_buf[in_p], v5);

  this->in_end -= this->in_p;

  v7 = this->in_end;

  in_file = this->in_file;

  this->in_p = 0;

  v9 = in_file->ReadMax(&this->in_buf[v7], ((((0x4000 - v7) >> 31) & 0xFFF) + 0x4000 - v7) & 0xFFFFF000);

  v10 = this->in_end;

  if ( v9 > 0 )

  {

    v10 += v9;

    this->in_end = v10;

  }

  return v10 - this->in_p >= minsize;

}



//----- (0046F870) --------------------------------------------------------

unsigned int SMpegAudioDecoder::in_getbits(int n)

{

  int v2; // ebx

  int in_bits; // esi

  unsigned int in_bitbuf; // eax

  int in_p; // ebx

  int v6; // esi



  v2 = n;

  in_bits = this->in_bits;

  in_bitbuf = this->in_bitbuf;

  if ( in_bits < n )

  {

    in_p = this->in_p;

    do

    {

      in_bits += 8;

      in_bitbuf = (in_bitbuf << 8) | this->in_buf[in_p++];

      this->in_bitbuf = in_bitbuf;

      this->in_p = in_p;

      this->in_bits = in_bits;

    }

    while ( in_bits < n );

    v2 = n;

  }

  v6 = in_bits - v2;

  this->in_bits = v6;

  return ((1 << v2) - 1) & (in_bitbuf >> v6);

}



//----- (0046F8E0) --------------------------------------------------------

void SMpegAudioDecoder::in_open(SStream *is)

{

  this->in_file = is;

  this->in_bits = 0;

  this->in_p = 0;

  this->in_end = 0;

}



//----- (0046F910) --------------------------------------------------------

int SMpegAudioDecoder::in_seeksync()

{

  int v2; // ecx

  unsigned char *in_buf; // ebx

  int v4; // edi

  int in_p; // edx

  signed int v6; // eax

  int v7; // ebx

  SStream *in_file; // ecx

  int v9; // eax

  int in_end; // ecx

  unsigned char v11; // bl

  int v12; // eax

  int v14; // [esp+Ch] [ebp-Ch]

  int v15; // [esp+14h] [ebp-4h]



  v2 = 0;

  v15 = 0;

  in_buf = this->in_buf;

  while ( 1 )

  {

    v4 = v2;

    in_p = this->in_p;

    v6 = this->in_end - in_p;

    v14 = v2;

    if ( v6 < 2 )

    {

      memmove(this->in_buf, &this->in_buf[in_p], v6);

      v7 = this->in_end - this->in_p;

      in_file = this->in_file;

      this->in_end = v7;

      this->in_p = 0;

      v9 = in_file->ReadMax(&this->in_buf[v7], ((((0x4000 - v7) >> 31) & 0xFFF) + 0x4000 - v7) & 0xFFFFF000);

      in_end = this->in_end;

      if ( v9 > 0 )

      {

        in_end += v9;

        this->in_end = in_end;

      }

      in_p = this->in_p;

      if ( in_end - in_p < 2 )

        return 0;

      in_buf = this->in_buf;

      v2 = v15;

      v4 = v14;

    }

    if ( in_buf[in_p] == 0xFF )

    {

      v11 = this->in_buf[in_p + 1];

      if ( (v11 & 0xF0) == 0xF0 )

      {

        this->in_bits = 4;

        this->in_bitbuf = v11 & 0xF;

        this->in_p = in_p + 2;

        in_fillbuf(1728);

        return 1;

      }

      in_buf = this->in_buf;

    }

    v12 = v2++;

    v15 = v2;

    if ( !v12 )

    {

      printf("Syncing...\n");

      v2 = v15;

    }

    if ( v4 >= 1024 )

      break;

    ++this->in_p;

  }

  printf("Sync lost.\n");

  return 0;

}



//----- (0046FA60) --------------------------------------------------------

void SMpegAudioDecoder::s2_dropbits(int n)

{

  int s2_bits; // eax

  unsigned int s2_bitbuf; // esi

  int s2_p; // edi

  int v6; // ecx



  s2_bits = this->s2_bits;

  if ( s2_bits < n )

  {

    s2_bitbuf = this->s2_bitbuf;

    s2_p = this->s2_p;

    do

    {

      v6 = this->s2_buf[s2_p];

      s2_bits += 8;

      ++s2_p;

      s2_bitbuf = v6 | (s2_bitbuf << 8);

      this->s2_p = s2_p;

      this->s2_bitbuf = s2_bitbuf;

      this->s2_bits = s2_bits;

    }

    while ( s2_bits < n );

  }

  this->s2_bits = s2_bits - n;

}



//----- (0046FAC0) --------------------------------------------------------

int SMpegAudioDecoder::s2_fillbuf(int size)

{

  int s2_bits; // edi

  int s2_p; // eax

  int v5; // ecx

  int v6; // edx

  int result; // eax



  s2_bits = this->s2_bits;

  s2_p = this->s2_p;

  if ( s2_bits <= 8 )

  {

    v6 = this->s2_p;

  }

  else

  {

    do

    {

      v5 = s2_p;

      if ( s2_p )

        v6 = s2_p - 1;

      else

        v6 = 0;

      --s2_p;

      if ( !v5 )

        s2_p = 0;

      s2_bits -= 8;

    }

    while ( s2_bits > 8 );

    this->s2_bits = s2_bits;

    this->s2_p = s2_p;

  }

  if ( this->s2_end - v6 + size > 4096 )

    return 0;

  memmove(this->s2_buf, &this->s2_buf[v6], this->s2_end - v6);

  this->s2_end -= this->s2_p;

  this->s2_p = 0;

  if ( !in_fillbuf(size) )

    return 0;

  memmove(&this->s2_buf[this->s2_end], &this->in_buf[this->in_p], size);

  result = size + this->s2_end;

  this->s2_bits = 0;

  this->in_p += size;

  this->s2_end = result;

  return result;

}



//----- (0046FBB0) --------------------------------------------------------

void SMpegAudioDecoder::s2_flushbits()

{

  int s2_bits; // edx

  int s2_p; // eax

  int v4; // ecx



  s2_bits = this->s2_bits;

  if ( s2_bits > 8 )

  {

    s2_p = this->s2_p;

    do

    {

      v4 = s2_p;

      if ( s2_p-- == 0 )

        s2_p = v4;

      s2_bits -= 8;

    }

    while ( s2_bits > 8 );

    this->s2_bits = s2_bits;

    this->s2_p = s2_p;

  }

}



//----- (0046FBF0) --------------------------------------------------------

unsigned int SMpegAudioDecoder::s2_getbits(int n)

{

  int v2; // ebx

  int s2_bits; // esi

  unsigned int s2_bitbuf; // eax

  int s2_p; // ebx

  int v6; // esi



  v2 = n;

  s2_bits = this->s2_bits;

  s2_bitbuf = this->s2_bitbuf;

  if ( s2_bits < n )

  {

    s2_p = this->s2_p;

    do

    {

      s2_bits += 8;

      s2_bitbuf = (s2_bitbuf << 8) | this->s2_buf[s2_p++];

      this->s2_bitbuf = s2_bitbuf;

      this->s2_p = s2_p;

      this->s2_bits = s2_bits;

    }

    while ( s2_bits < n );

    v2 = n;

  }

  v6 = s2_bits - v2;

  this->s2_bits = v6;

  return ((1 << v2) - 1) & (s2_bitbuf >> v6);

}



//----- (0046FC60) --------------------------------------------------------

int SMpegAudioDecoder::s2_getpos()

{

  return 8 * this->s2_p - this->s2_bits;

}



//----- (0046FC70) --------------------------------------------------------

void SMpegAudioDecoder::s2_init()

{

  this->s2_bits = 0;

  this->s2_p = 0;

  this->s2_end = 0;

}



//----- (0046FC90) --------------------------------------------------------

unsigned int SMpegAudioDecoder::s2_prebits(int n)

{

  char v2; // bl

  int s2_bits; // esi

  unsigned int s2_bitbuf; // eax

  int s2_p; // ebx



  v2 = n;

  s2_bits = this->s2_bits;

  s2_bitbuf = this->s2_bitbuf;

  if ( s2_bits < n )

  {

    s2_p = this->s2_p;

    do

    {

      s2_bits += 8;

      s2_bitbuf = (s2_bitbuf << 8) | this->s2_buf[s2_p++];

      this->s2_bitbuf = s2_bitbuf;

      this->s2_p = s2_p;

      this->s2_bits = s2_bits;

    }

    while ( s2_bits < n );

    v2 = n;

  }

  return ((1 << v2) - 1) & (s2_bitbuf >> (s2_bits - v2));

}



//----- (0046FD00) --------------------------------------------------------

void SMpegAudioDecoder::s2_setpos(int pos)

{

  int v2; // esi



  v2 = (pos + 7) >> 3;

  this->s2_p = v2;

  this->s2_bits = 8 * v2 - pos;

  if ( 8 * v2 != pos )

    this->s2_bitbuf = *((unsigned char *)&this->in_bitbuf + v2 + 3);

}
