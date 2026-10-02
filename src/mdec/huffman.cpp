// mdec/huffman.cpp
// Huffman decoding for MP3 decoder
// Decompiled from: gameSplit/mp3_decoder.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <corecrt_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "decode.h"

// Classes: SMpegAudioDecoder
// Function count: 2

//----- (0046DF50) --------------------------------------------------------

void SMpegAudioDecoder::HuffmanDecode(int h, int *x, int *y, int *v, int *w)

{

  int v7; // esi

  unsigned int i; // eax

  int *v9; // ecx

  unsigned int v10; // eax

  unsigned int v11; // [esp+Ch] [ebp-4h]



  v7 = huf_tabs[h].tabstart;

  if ( v7 == huf_tabs[h].tabend )

  {

    *w = 0;

    *v = 0;

    *y = 0;

    *x = 0;

  }

  else

  {

    for ( i = s2_prebits(20); i < huf_codes[v7]; ++v7 )

      ;

    v11 = huf_values[v7];

    s2_dropbits(huf_bits[v7]);

    if ( h >= 32 )

    {

      *v = (v11 >> 3) & 1;

      *w = (v11 >> 2) & 1;

      *x = (v11 >> 1) & 1;

      *y = v11 & 1;

      if ( *v && s2_getbits(1) )

        *v = -*v;

      if ( *w && s2_getbits(1) )

        *w = -*w;

      if ( *x && s2_getbits(1) )

        *x = -*x;

    }

    else

    {

      v9 = x;

      *x = v11 >> 4;

      *y = v11 & 0xF;

      if ( huf_tabs[h].linbits && huf_tabs[h].maxval == *x )

      {

        v10 = s2_getbits(huf_tabs[h].linbits);

        v9 = x;

        *x += v10;

      }

      if ( *v9 && s2_getbits(1) )

        *x = -*x;

      if ( huf_tabs[h].linbits && huf_tabs[h].maxval == *y )

        *y += s2_getbits(huf_tabs[h].linbits);

    }

    if ( *y )

    {

      if ( s2_getbits(1) )

        *y = -*y;

    }

  }

}

//----- (004712A0) --------------------------------------------------------

void SMpegAudioDecoder::L3_HuffmanDecode(int gr, int ch, int block_end)

{

  int v6; // ecx

  int v7; // edx

  int v8; // ecx

  int v9; // edx

  int v10; // eax

  int v11; // eax

  int v12; // esi

  int v13; // eax

  int *v14; // ecx

  unsigned int v15; // eax

  int v16; // eax

  int v17; // edi

  int *v18; // edi

  int v19; // eax

  int w; // [esp+Ch] [ebp-14h] BYREF

  int v; // [esp+10h] [ebp-10h] BYREF

  int v22; // [esp+14h] [ebp-Ch]

  int *v23; // [esp+18h] [ebp-8h]

  int y; // [esp+1Ch] [ebp-4h] BYREF



  // IDA expressed this as `&Frame.version + 40*ch + 18*gr` (int-stride pointer
  // arithmetic). That relied on Callback being 4 bytes (x86) so the byte offsets
  // between Frame, Callback, sfreq and si happened to round to ints. On x64
  // Callback is 8 bytes and the stride drifts one int per pointer-sized field,
  // landing v5[N] in the wrong part of si.ch[ch].gr[gr]. Use symbolic access.
  SGrInfo &grInfo = this->si.ch[ch].gr[gr];

  if ( grInfo.window_switching_flag && grInfo.block_type == 2 )

  {

    v6 = 36;

    v7 = 576;

    ch = 36;

  }

  else

  {

    v8 = 37 * this->sfreq;

    v9 = (int)grInfo.region0_count;

    ch = dword_58C55C[v8 + v9];

    v10 = v8 + (int)grInfo.region1_count;

    v6 = ch;

    v7 = dword_58C560[v9 + v10];

  }

  v11 = (int)grInfo.big_values;

  v12 = 0;

  v22 = v7;

  if ( 2 * v11 )

  {

    v23 = &this->is[1];

    do

    {

      if ( v12 >= v6 )

      {

        if ( v12 >= v7 )

          v13 = (int)grInfo.table_select[2];

        else

          v13 = (int)grInfo.table_select[1];

      }

      else

      {

        v13 = (int)grInfo.table_select[0];

      }

      HuffmanDecode(v13, &gr, &y, &v, &w);

      v14 = v23;

      v12 += 2;

      v7 = v22;

      *(v23 - 1) = gr;

      *v14 = y;

      v15 = 2 * grInfo.big_values;

      v23 = v14 + 2;

      v6 = ch;

    }

    while ( (unsigned int)v12 < v15 );

  }

  ch = (int)grInfo.count1table_select + 32;

  v16 = s2_getpos();

  v17 = block_end;

  if ( v16 < block_end )

  {

    v18 = &this->is[v12 + 1];

    do

    {

      if ( v12 >= 576 )

        break;

      HuffmanDecode(ch, &gr, &y, &v, &w);

      *(v18 - 1) = v;

      v12 += 4;

      *v18 = w;

      v18[1] = gr;

      v18[2] = y;

      v18 += 4;

      v19 = s2_getpos();

    }

    while ( v19 < block_end );

    v17 = block_end;

  }

  if ( s2_getpos() > v17 )

    v12 -= 4;

  s2_setpos(v17);

  if ( v12 < 576 )

    memset(&this->is[v12], 0, 4 * (576 - v12));

}
