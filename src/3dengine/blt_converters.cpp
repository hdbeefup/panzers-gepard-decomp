// 3dengine/blt_converters.cpp — Pixel format conversion (DXT, RGB, shadow maps)
#include <windows.h>
#include <string.h>
#include <emmintrin.h>
#include "core_common.h"
#include "logger.h"

// Override Windows read-only LOBYTE/LOWORD/HIBYTE with lvalue-compatible versions (IDA pattern)
#undef LOBYTE
#undef HIBYTE
#undef LOWORD
#undef HIWORD
#define LOBYTE(x) (*((_BYTE*)&(x)))
#define HIBYTE(x) (*(((_BYTE*)&(x)) + sizeof(x) - 1))
#define LOWORD(x) (*((_WORD*)&(x)))
#define HIWORD(x) (*(((_WORD*)&(x)) + 1))

//----- (004611F0) --------------------------------------------------------

void __cdecl BltSameFormat(

        unsigned char *dest_line,

        int dest_pitch,

        unsigned char *src_line,

        int src_pitch,

        int linewidth,

        int height)

{

  int i; // esi



  for ( i = height; i; --i )

  {

    memcpy(dest_line, src_line, linewidth);

    src_line += src_pitch;

    dest_line += dest_pitch;

  }

}



//----- (00461220) --------------------------------------------------------

void __cdecl Blt_A1R5G5B5_From_DXT1(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // ecx

  int v7; // eax

  unsigned __int8 *v8; // ebx

  unsigned __int8 *v9; // edx

  int v10; // esi

  int v11; // edi

  unsigned __int16 v12; // cx

  unsigned __int16 v13; // ax

  unsigned __int16 v14; // si

  int v15; // ebx

  int v16; // ebx

  int v17; // edi

  unsigned int v18; // ecx

  __int16 v19; // dx

  unsigned int v20; // edx

  int i; // ecx

  char v22; // al

  unsigned __int8 *v23; // ecx

  int v24; // esi

  int v25; // eax

  unsigned __int8 *v26; // edx

  int v27; // ecx

  unsigned __int8 *v28; // [esp+10h] [ebp-5Ch]

  int v29; // [esp+1Ch] [ebp-50h]

  _DWORD *v30; // [esp+20h] [ebp-4Ch]

  unsigned __int8 *v31; // [esp+20h] [ebp-4Ch]

  int v32; // [esp+24h] [ebp-48h]

  unsigned __int8 *v33; // [esp+28h] [ebp-44h]

  int v34; // [esp+2Ch] [ebp-40h]

  unsigned __int16 v35; // [esp+30h] [ebp-3Ch]

  int v36; // [esp+34h] [ebp-38h]

  int v37; // [esp+38h] [ebp-34h]

  int v38; // [esp+3Ch] [ebp-30h]

  unsigned __int8 *v39; // [esp+3Ch] [ebp-30h]

  unsigned __int16 block[16]; // [esp+40h] [ebp-2Ch]

  unsigned __int16 c[4]; // [esp+60h] [ebp-Ch]



  v6 = height;

  v7 = 0;

  v29 = 0;

  v8 = dest_line;

  v28 = dest_line;

  if ( height > 0 )

  {

    v9 = src_line;

    v10 = 4 * dest_pitch;

    v11 = width;

    do

    {

      v33 = v8;

      v32 = 0;

      if ( v11 > 0 )

      {

        do

        {

          v12 = *(_WORD *)v9;

          v35 = *(_WORD *)v9;

          v13 = *((_WORD *)v9 + 1);

          v30 = (_DWORD *)(v9 + 4);

          v14 = v13;

          v15 = v32;

          v34 = v13 & 0xF800;

          v38 = *(_WORD *)v9 & 0xF800;

          if ( *(_WORD *)v9 <= v13 )

          {

            v19 = *((_WORD *)v9 + 1) & 0x1F;

            c[3] = ((unsigned int)(v38 + v34) >> 2) & 0x7C00 | ((unsigned __int16)((v19 + (v12 & 0x1F)) | (((v12 & 0x7E0) + (v13 & 0x7E0u)) >> 1) & 0x7C0) >> 1);

            c[2] = c[3] | 0x8000;

          }

          else

          {

            v37 = *(_WORD *)v9 & 0x1F;

            v36 = *((_WORD *)v9 + 1) & 0x1F;

            v16 = *(_WORD *)v9 & 0x7E0;

            v17 = *((_WORD *)v9 + 1) & 0x7E0;

            c[2] = ((v36 + 2 * v37 + 1) / 3u) & 0x1F | ((v17 + 2 * (v16 + 16)) / 6u) & 0x3E0 | ((v34

                                                                                               + 2048

                                                                                               + 2 * (v12 & 0xF800u))

                                                                                              / 6) & 0xFC00 | 0x8000;

            v18 = v16 + 2 * (v17 + 16);

            v15 = v32;

            v19 = v13 & 0x1F;

            c[3] = ((v37 + 2 * v36 + 1) / 3u) & 0x1F | (v18 / 6) & 0x3E0 | ((v38 + 2 * v34 + 2048) / 6u) & 0xFC00 | 0x8000;

            v14 = v13;

          }

          v11 = width;

          c[0] = v35 & 0x1F | (v35 >> 1) & 0xFFE0 | 0x8000;

          c[1] = v19 | (v14 >> 1) & 0xFFE0 | 0x8000;

          v20 = *v30;

          v31 = (unsigned __int8 *)(v30 + 1);

          for ( i = 0; i < 16; ++i )

          {

            v22 = v20;

            v20 >>= 2;

            block[i] = c[v22 & 3];

          }

          v23 = v33;

          v24 = 0;

          v39 = v33;

          do

          {

            if ( v24 + v29 >= height )

              break;

            v25 = 0;

            v26 = v23;

            do

            {

              if ( v25 + v15 >= width )

                break;

              v27 = v25 + 4 * v24;

              ++v25;

              *(_WORD *)v26 = block[v27];

              v26 += 2;

            }

            while ( v25 < 4 );

            ++v24;

            v23 = &v39[dest_pitch];

            v39 += dest_pitch;

          }

          while ( v24 < 4 );

          v33 += 8;

          v9 = v31;

          v32 = v15 + 4;

        }

        while ( v15 + 4 < width );

        v7 = v29;

        v6 = height;

        v10 = 4 * dest_pitch;

      }

      v7 += 4;

      v9 = &src_line[src_pitch];

      v8 = &v28[v10];

      src_line += src_pitch;

      v28 += v10;

      v29 = v7;

    }

    while ( v7 < v6 );

  }

}



//----- (004614F0) --------------------------------------------------------

void __cdecl Blt_A4R4G4B4_From_DXT5(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // ecx

  int v7; // eax

  unsigned __int8 *v8; // ebx

  unsigned __int8 *v9; // esi

  int v10; // edx

  int v11; // edi

  unsigned int v12; // ecx

  unsigned __int16 *v13; // esi

  unsigned int v14; // edx

  unsigned int v15; // edx

  int v16; // eax

  unsigned int v17; // ecx

  int v18; // esi

  int v19; // eax

  int v20; // eax

  int v21; // eax

  int v22; // eax

  int v23; // eax

  int v24; // ecx

  unsigned int v25; // esi

  char v26; // al

  unsigned int v27; // eax

  _DWORD *v28; // esi

  int v29; // ebx

  int v30; // ebx

  int v31; // edi

  unsigned int v32; // kr04_4

  unsigned int v33; // ecx

  unsigned int v34; // ecx

  int i; // edx

  char v36; // al

  unsigned int v37; // ecx

  int v38; // eax

  int v39; // eax

  int v40; // eax

  unsigned __int8 *v41; // ecx

  int v42; // esi

  int v43; // eax

  unsigned __int8 *v44; // edx

  int v45; // ecx

  unsigned __int8 *v46; // [esp+10h] [ebp-70h]

  int v47; // [esp+14h] [ebp-6Ch]

  int v48; // [esp+18h] [ebp-68h]

  int v49; // [esp+1Ch] [ebp-64h]

  unsigned __int8 *v50; // [esp+20h] [ebp-60h]

  int v51; // [esp+24h] [ebp-5Ch]

  unsigned __int8 *v52; // [esp+28h] [ebp-58h]

  unsigned __int16 *v53; // [esp+2Ch] [ebp-54h]

  _DWORD *v54; // [esp+2Ch] [ebp-54h]

  unsigned __int8 *v55; // [esp+2Ch] [ebp-54h]

  unsigned int v56; // [esp+30h] [ebp-50h]

  int v57; // [esp+30h] [ebp-50h]

  int v58; // [esp+34h] [ebp-4Ch]

  int v59; // [esp+34h] [ebp-4Ch]

  int v60; // [esp+38h] [ebp-48h]

  unsigned __int16 v61; // [esp+38h] [ebp-48h]

  int v62; // [esp+3Ch] [ebp-44h]

  int v63; // [esp+3Ch] [ebp-44h]

  int v64; // [esp+44h] [ebp-3Ch]

  unsigned __int8 *v65; // [esp+44h] [ebp-3Ch]

  int v66; // [esp+48h] [ebp-38h]

  unsigned __int16 block[16]; // [esp+4Ch] [ebp-34h]

  unsigned __int16 c[4]; // [esp+6Ch] [ebp-14h]

  unsigned __int8 a[8]; // [esp+74h] [ebp-Ch]



  v6 = height;

  v7 = 0;

  v49 = 0;

  v8 = dest_line;

  v50 = dest_line;

  v9 = src_line;

  v46 = src_line;

  if ( height > 0 )

  {

    v10 = 4 * dest_pitch;

    v11 = width;

    do

    {

      v52 = v8;

      v51 = 0;

      if ( v11 > 0 )

      {

        do

        {

          v12 = *(unsigned __int16 *)v9;

          v13 = (unsigned __int16 *)(v9 + 2);

          *(_WORD *)a = v12;

          v53 = v13;

          v58 = 4 * (unsigned __int8)v12;

          v60 = 3 * (unsigned __int8)v12;

          v48 = 2 * (unsigned __int8)v12;

          v14 = v12 >> 8;

          v56 = v12 >> 8;

          v62 = 2 * (v12 >> 8);

          v47 = 3 * (v12 >> 8);

          v64 = 4 * (v12 >> 8);

          if ( (unsigned __int8)v12 <= BYTE1(v12) )

          {

            *(_WORD *)&a[6] = -256;

            a[2] = (v14 + v58 + 2) / 5;

            a[3] = (v62 + 2 + v60) / 5u;

            a[4] = (v48 + 2 + v47) / 5u;

            a[5] = (v64 + 2 + (unsigned int)(unsigned __int8)v12) / 5;

          }

          else

          {

            a[2] = (v14 + 6 * (unsigned __int8)v12 + 3) / 7;

            a[3] = (v62 + 4 * (unsigned __int8)v12 + (unsigned int)(unsigned __int8)v12 + 3) / 7;

            a[4] = (v58 + 3 + v47) / 7u;

            a[5] = (v60 + 3 + v64) / 7u;

            a[6] = (v48 + 4 * v56 + v56 + 3) / 7;

            a[7] = ((unsigned __int8)v12 + 6 * v56 + 3) / 7;

          }

          v15 = v13[1];

          v16 = *v13 & 7;

          v17 = (unsigned int)(*v13 + ((unsigned __int8)v15 << 16)) >> 3;

          v18 = v13[2] << 8;

          block[0] = (a[v16] & 0xF0) << 8;

          v19 = v17 & 7;

          v17 >>= 3;

          block[1] = (a[v19] & 0xF0) << 8;

          v20 = v17 & 7;

          v17 >>= 3;

          block[2] = (a[v20] & 0xF0) << 8;

          v21 = v17 & 7;

          v17 >>= 3;

          block[3] = (a[v21] & 0xF0) << 8;

          v22 = v17 & 7;

          v17 >>= 3;

          block[4] = (a[v22] & 0xF0) << 8;

          v23 = v17 & 7;

          v17 >>= 3;

          block[5] = (a[v23] & 0xF0) << 8;

          block[6] = (a[v17 & 7] & 0xF0) << 8;

          v23 = a[v17 >> 3];

          v24 = 8;

          v25 = (v15 >> 8) + v18;

          block[7] = (v23 & 0xFFF0) << 8;

          do

          {

            v26 = v25;

            v25 >>= 3;

            block[v24++] = (a[v26 & 7] & 0xF0) << 8;

          }

          while ( v24 < 16 );

          v61 = v53[3];

          v27 = v53[4];

          v28 = (_DWORD *)(v53 + 5);

          v54 = (_DWORD *)(v53 + 5);

          v59 = v27 & 0xF800;

          v57 = v61 & 0xF800;

          v29 = v51;

          if ( v61 <= (unsigned __int16)v27 )

          {

            c[2] = ((unsigned int)(v57 + v59) >> 5) & 0xF00 | ((unsigned __int16)(((v61 & 0x1F) + (v27 & 0x1F)) | (((v61 & 0x7E0) + (v27 & 0x7E0)) >> 2) & 0x3C0) >> 2);

            c[3] = c[2];

          }

          else

          {

            v66 = v61 & 0x1F;

            v30 = v61 & 0x7E0;

            v63 = v27 & 0x1F;

            v31 = v27 & 0x7E0;

            v32 = v31 + 2 * (v30 + 16);

            v33 = v30 + 2 * (v31 + 16);

            v29 = v51;

            v11 = width;

            c[2] = ((v63 + 2 * v66 + 1) / 6u) & 0xF | ((v59 + 2048 + 2 * v57) / 0x30u) & 0xF00 | (v32 / 0x18) & 0xF0;

            c[3] = ((v66 + 2 * v63 + 1) / 6u) & 0xF | ((v57 + 2 * v59 + 2048) / 0x30u) & 0xF00 | (v33 / 0x18) & 0xF0;

            v28 = v54;

          }

          c[0] = (unsigned __int16)(v61 & 0x1E | ((unsigned __int16)(v61 & 0x780 | (v61 >> 1) & 0x7800) >> 2)) >> 1;

          c[1] = (unsigned __int16)(v27 & 0x1E | ((unsigned __int16)(v27 & 0x780 | (v27 >> 1) & 0x7800) >> 2)) >> 1;

          v34 = *v28;

          v55 = (unsigned __int8 *)(v28 + 1);

          for ( i = 0; i < 16; i += 4 )

          {

            v36 = v34;

            v37 = v34 >> 2;

            block[i] |= c[v36 & 3];

            v38 = v37 & 3;

            v37 >>= 2;

            block[i + 1] |= c[v38];

            v39 = v37 & 3;

            v37 >>= 2;

            block[i + 2] |= c[v39];

            v40 = v37 & 3;

            v34 = v37 >> 2;

            block[i + 3] |= c[v40];

          }

          v41 = v52;

          v42 = 0;

          v65 = v52;

          do

          {

            if ( v42 + v49 >= height )

              break;

            v43 = 0;

            v44 = v41;

            do

            {

              if ( v43 + v29 >= v11 )

                break;

              v45 = v43 + 4 * v42;

              ++v43;

              *(_WORD *)v44 = block[v45];

              v44 += 2;

            }

            while ( v43 < 4 );

            ++v42;

            v41 = &v65[dest_pitch];

            v65 += dest_pitch;

          }

          while ( v42 < 4 );

          v52 += 8;

          v9 = v55;

          v51 = v29 + 4;

        }

        while ( v29 + 4 < v11 );

        v7 = v49;

        v6 = height;

        v10 = 4 * dest_pitch;

      }

      v7 += 4;

      v9 = &v46[src_pitch];

      v8 = &v50[v10];

      v46 += src_pitch;

      v50 += v10;

      v49 = v7;

    }

    while ( v7 < v6 );

  }

}



//----- (00461A70) --------------------------------------------------------

void __cdecl Blt_A8R8G8B8_From_DXT1(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // ecx

  int v7; // eax

  unsigned __int8 *v8; // esi

  int v9; // ebx

  unsigned __int8 *v10; // edx

  int v11; // edi

  unsigned __int16 v12; // bx

  unsigned int v13; // edx

  int i; // ecx

  char v15; // al

  unsigned __int8 *v16; // ecx

  int v17; // esi

  int v18; // eax

  unsigned __int8 *v19; // edx

  int v20; // ecx

  unsigned __int8 *v21; // [esp+18h] [ebp-78h]

  unsigned int v22; // [esp+1Ch] [ebp-74h]

  int v23; // [esp+20h] [ebp-70h]

  _DWORD *v24; // [esp+24h] [ebp-6Ch]

  unsigned __int8 *v25; // [esp+24h] [ebp-6Ch]

  unsigned __int8 *v26; // [esp+28h] [ebp-68h]

  unsigned __int8 *v27; // [esp+38h] [ebp-58h]

  unsigned int block[16]; // [esp+3Ch] [ebp-54h]

  struct { union { struct { unsigned char r, g, b, a; }; unsigned int val; }; } c[4]; // unnamed color type



  v6 = height;

  v7 = 0;

  v23 = 0;

  v8 = dest_line;

  v21 = dest_line;

  if ( height > 0 )

  {

    v9 = width;

    v10 = src_line;

    do

    {

      v11 = 0;

      v26 = v8;

      if ( v9 > 0 )

      {

        c[0].a = -1;

        c[1].a = -1;

        c[2].a = -1;

        do

        {

          v12 = *(_WORD *)v10;

          v22 = *((unsigned __int16 *)v10 + 1);

          v24 = (_DWORD *)(v10 + 4);

          c[0].g = ((*(unsigned __int16 *)v10 >> 3) & 0xFC) + 2;

          c[1].b = (BYTE1(v22) & 0xF8) + 4;

          c[0].r = 8 * v12 + 4;

          c[0].b = (HIBYTE(v12) & 0xF8) + 4;

          c[1].r = 8 * v22 + 4;

          c[1].g = ((v22 >> 3) & 0xFC) + 2;

          if ( v12 <= (unsigned __int16)v22 )

          {

            c[2].r = (c[0].r + (unsigned int)c[1].r) >> 1;

            c[2].g = (c[1].g + (unsigned int)c[0].g) >> 1;

            c[2].b = (c[0].b + (unsigned int)c[1].b) >> 1;

            c[3] = c[2];

            c[3].a = 0;

          }

          else

          {

            c[3].a = -1;

            c[2].r = (c[1].r + 2 * (unsigned int)c[0].r) / 3;

            c[2].g = (c[1].g + 2 * (unsigned int)c[0].g) / 3;

            c[2].b = (c[1].b + 2 * (unsigned int)c[0].b) / 3;

            c[3].r = (c[0].r + 2 * (unsigned int)c[1].r) / 3;

            c[3].g = (c[0].g + 2 * (unsigned int)c[1].g) / 3;

            c[3].b = (c[0].b + 2 * (unsigned int)c[1].b) / 3;

          }

          v13 = *v24;

          v25 = (unsigned __int8 *)(v24 + 1);

          for ( i = 0; i < 16; ++i )

          {

            v15 = v13;

            v13 >>= 2;

            block[i] = c[v15 & 3].val;

          }

          v16 = v26;

          v17 = 0;

          v9 = width;

          v27 = v26;

          do

          {

            if ( v17 + v23 >= height )

              break;

            v18 = 0;

            v19 = v16;

            do

            {

              if ( v18 + v11 >= width )

                break;

              v20 = v18 + 4 * v17;

              ++v18;

              *(_DWORD *)v19 = block[v20];

              v19 += 4;

            }

            while ( v18 < 4 );

            ++v17;

            v16 = &v27[dest_pitch];

            v27 += dest_pitch;

          }

          while ( v17 < 4 );

          v26 += 16;

          v11 += 4;

          v10 = v25;

        }

        while ( v11 < width );

        v7 = v23;

        v6 = height;

        v8 = v21;

      }

      v7 += 4;

      v10 = &src_line[src_pitch];

      v8 += 4 * dest_pitch;

      src_line += src_pitch;

      v21 = v8;

      v23 = v7;

    }

    while ( v7 < v6 );

  }

}



//----- (00461CC0) --------------------------------------------------------

void __cdecl Blt_A8R8G8B8_From_DXT5(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  unsigned __int8 *v6; // edx

  int v7; // ecx

  unsigned __int8 *v8; // esi

  int v9; // ebx

  int v10; // edi

  unsigned __int8 *v11; // ebx

  int v12; // esi

  unsigned int v13; // edx

  int v14; // esi

  unsigned int v15; // ecx

  int v16; // eax

  int v17; // eax

  int v18; // eax

  int v19; // eax

  int v20; // eax

  int v21; // eax

  int v22; // ecx

  unsigned int v23; // esi

  char v24; // al

  unsigned int v25; // ecx

  int i; // edx

  char v27; // al

  unsigned int v28; // ecx

  int v29; // eax

  int v30; // eax

  int v31; // eax

  unsigned __int8 *v32; // ecx

  int v33; // esi

  int v34; // eax

  unsigned __int8 *v35; // edx

  int v36; // ecx

  unsigned int v37; // [esp+14h] [ebp-88h]

  unsigned __int8 *v38; // [esp+18h] [ebp-84h]

  unsigned __int8 *v39; // [esp+1Ch] [ebp-80h]

  int v40; // [esp+20h] [ebp-7Ch]

  unsigned __int8 *v41; // [esp+24h] [ebp-78h]

  unsigned __int8 *v42; // [esp+2Ch] [ebp-70h]

  int v43; // [esp+30h] [ebp-6Ch]

  unsigned int v44; // [esp+30h] [ebp-6Ch]

  int v45; // [esp+34h] [ebp-68h]

  int v46; // [esp+38h] [ebp-64h]

  unsigned __int8 *v47; // [esp+3Ch] [ebp-60h]

  unsigned int *v48; // [esp+3Ch] [ebp-60h]

  unsigned int block[16]; // [esp+40h] [ebp-5Ch]

  struct { union { struct { unsigned char r, g, b, a; }; unsigned int val; }; } c[4]; // unnamed color type

  unsigned __int8 a[8]; // [esp+90h] [ebp-Ch]



  v6 = dest_line;

  v7 = 0;

  v8 = src_line;

  v38 = dest_line;

  v39 = src_line;

  v40 = 0;

  if ( height > 0 )

  {

    v9 = width;

    do

    {

      v10 = 0;

      v47 = v8;

      v41 = v6;

      if ( v9 > 0 )

      {

        c[0].a = 0;

        c[1].a = 0;

        c[2].a = 0;

        do

        {

          v11 = v47;

          v12 = HIBYTE(*(unsigned __int16 *)v47);

          *(_WORD *)a = *(_WORD *)v47;

          v45 = 4 * a[0];

          v46 = 3 * a[0];

          v43 = 2 * a[0];

          if ( a[0] <= a[1] )

          {

            *(_WORD *)&a[6] = -256;

            a[2] = (v12 + v45 + 2) / 5u;

            a[3] = (2 * v12 + v46 + 2) / 5u;

            a[4] = (3 * v12 + v43 + 2) / 5u;

            a[5] = ((unsigned int)a[0] + 4 * v12 + 2) / 5;

          }

          else

          {

            a[2] = (v12 + 6 * (unsigned int)a[0] + 3) / 7;

            a[3] = (2 * v12 + 4 * a[0] + (unsigned int)a[0] + 3) / 7;

            a[4] = (3 * v12 + v45 + 3) / 7u;

            a[5] = (v46 + 4 * v12 + 3) / 7u;

            a[6] = (v12 + v43 + 4 * v12 + 3) / 7u;

            a[7] = ((unsigned int)a[0] + 6 * v12 + 3) / 7;

          }

          v13 = *((unsigned __int16 *)v47 + 2);

          v14 = *((unsigned __int16 *)v47 + 3) << 8;

          v15 = (unsigned int)(*((unsigned __int16 *)v47 + 1) + ((unsigned __int8)v13 << 16)) >> 3;

          block[0] = a[*((_WORD *)v47 + 1) & 7] << 24;

          v16 = v15 & 7;

          v15 >>= 3;

          block[1] = a[v16] << 24;

          v17 = v15 & 7;

          v15 >>= 3;

          block[2] = a[v17] << 24;

          v18 = v15 & 7;

          v15 >>= 3;

          block[3] = a[v18] << 24;

          v19 = v15 & 7;

          v15 >>= 3;

          block[4] = a[v19] << 24;

          v20 = v15 & 7;

          v15 >>= 3;

          block[5] = a[v20] << 24;

          block[6] = a[v15 & 7] << 24;

          v21 = a[v15 >> 3];

          v22 = 8;

          v23 = (v13 >> 8) + v14;

          block[7] = v21 << 24;

          do

          {

            v24 = v23;

            v23 >>= 3;

            block[v22++] = a[v24 & 7] << 24;

          }

          while ( v22 < 16 );

          v37 = *((unsigned __int16 *)v47 + 4);

          v48 = (unsigned int *)(v47 + 12);

          v44 = *((unsigned __int16 *)v11 + 5);

          c[0].g = ((v37 >> 3) & 0xFC) + 2;

          c[1].b = (BYTE1(v44) & 0xF8) + 4;

          c[0].r = 8 * v37 + 4;

          c[0].b = (BYTE1(v37) & 0xF8) + 4;

          c[1].r = 8 * v44 + 4;

          c[1].g = ((v44 >> 3) & 0xFC) + 2;

          if ( (unsigned __int16)v37 <= (unsigned __int16)v44 )

          {

            c[2].r = (c[0].r + (unsigned int)c[1].r) >> 1;

            c[2].g = (c[1].g + (unsigned int)c[0].g) >> 1;

            c[2].b = (c[0].b + (unsigned int)c[1].b) >> 1;

            c[3] = c[2];

          }

          else

          {

            c[2].r = (c[1].r + 2 * (unsigned int)c[0].r) / 3;

            c[2].g = (c[1].g + 2 * (unsigned int)c[0].g) / 3;

            c[2].b = (c[1].b + 2 * (unsigned int)c[0].b) / 3;

            c[3].r = (c[0].r + 2 * (unsigned int)c[1].r) / 3;

            c[3].g = (c[0].g + 2 * (unsigned int)c[1].g) / 3;

            *(_WORD *)&c[3].b = (unsigned __int8)((c[0].b + 2 * (unsigned int)c[1].b) / 3);

          }

          v25 = *v48;

          v47 = v11 + 16;

          for ( i = 0; i < 16; i += 4 )

          {

            v27 = v25;

            v28 = v25 >> 2;

            block[i] |= c[v27 & 3].val;

            v29 = v28 & 3;

            v28 >>= 2;

            block[i + 1] |= c[v29].val;

            v30 = v28 & 3;

            v28 >>= 2;

            block[i + 2] |= c[v30].val;

            v31 = v28 & 3;

            v25 = v28 >> 2;

            block[i + 3] |= c[v31].val;

          }

          v32 = v41;

          v33 = 0;

          v9 = width;

          v42 = v41;

          do

          {

            if ( v33 + v40 >= height )

              break;

            v34 = 0;

            v35 = v32;

            do

            {

              if ( v34 + v10 >= width )

                break;

              v36 = v34 + 4 * v33;

              ++v34;

              *(_DWORD *)v35 = block[v36];

              v35 += 4;

            }

            while ( v34 < 4 );

            ++v33;

            v32 = &v42[dest_pitch];

            v42 += dest_pitch;

          }

          while ( v33 < 4 );

          v41 += 16;

          v10 += 4;

        }

        while ( v10 < width );

        v7 = v40;

        v6 = v38;

        v8 = v39;

      }

      v8 += src_pitch;

      v7 += 4;

      v6 += 4 * dest_pitch;

      v39 = v8;

      v38 = v6;

      v40 = v7;

    }

    while ( v7 < height );

  }

}

// Alpha encoding lookup table for DXT5
static unsigned char alphas[8] = {0, 1, 2, 3, 4, 5, 6, 7};

//----- (00462180) --------------------------------------------------------

void __cdecl Blt_DXT_From_A8R8G8B8(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height,

        int dxt)

{

  unsigned __int8 *v7; // esi

  int v8; // ecx









  int v17; // edx

  int v18; // ecx

  unsigned __int8 *v19; // eax

  unsigned __int8 *v20; // ebx

  int v21; // esi

  int v22; // ecx

  int v23; // ecx

  int v24; // edx

  int v25; // eax

  int v26; // esi

  bool v27; // cc

  unsigned __int8 *v28; // ecx

  int v29; // edi

  unsigned __int8 v30; // al

  unsigned __int8 *v31; // ebx

  unsigned __int8 v32; // al

  unsigned __int8 *v33; // ebx

  unsigned __int8 v34; // al

  unsigned __int8 *v35; // ebx

  unsigned __int8 *v36; // edi

  unsigned __int8 v37; // al

  char v38; // cl

  __int16 v39; // dx

  unsigned __int8 v40; // al

  char v41; // cl

  __int16 v42; // dx

  unsigned __int8 v43; // al

  char v44; // cl

  unsigned int v45; // ecx

  unsigned int v46; // esi

  int v47; // edi

  int v48; // ebx

  int v49; // ecx

  int v50; // esi

  int v51; // eax

  int v52; // eax

  unsigned int v53; // esi

  int v54; // esi

  int v55; // ecx

  int v56; // edx

  float v57;

  float v58; // xmm5_4

  float v59; // xmm4_4

  float v60; // xmm7_4

  float v61; // xmm1_4

  float v62; // xmm6_4

  float v63; // xmm7_4

  float v64; // xmm3_4

  float v65; // xmm4_4

  float v66;

  float v67; // xmm5_4

  float v68;

  float v69;

  int v70; // edx

  int v71; // ecx

  int v72; // eax

  unsigned __int16 v73; // si

  int v74; // edx

  int v75; // ecx

  int v76; // eax

  float v77;

  float v78;

  char v79; // al

  char v80; // dl

  char v81; // cl

  char v82; // al

  char v83; // al

  char v84; // dl

  char v85; // al

  char v86; // cl

  char v87; // al

  char v88; // dl

  char v89; // al

  char v90; // cl

  char v91; // al

  char v92; // dl

  char v93; // al

  char v94; // cl

  char v95; // dl

  float v96;

  float v97;

  float v98;

  float v99;

  char v100; // cl

  float v101;

  float v102;

  char v103; // al

  float v104;

  char v105; // al

  char v106; // al

  char v107; // cl

  float v108;

  char v109; // al

  float v110;

  float v111;

  char v112; // dl

  float v113;

  char v114; // dl

  char v115; // al

  char v116; // dl

  float v117;

  char v118; // cl

  float v119;

  float v120;

  char v121; // al

  float v122;

  char v123; // al

  char v124; // al

  char v125; // dl

  float v126;

  char v127; // cl

  float v128;

  float v129;

  char v130; // al

  float v131;

  char v132; // al

  int v133; // eax

  __int16 v134; // dx

  bool v135; // cf


  int v137; // [esp+Ch] [ebp-16Ch]

  int v138; // [esp+Ch] [ebp-16Ch]

  char v139; // [esp+Ch] [ebp-16Ch]

  char v140; // [esp+Ch] [ebp-16Ch]

  char v141; // [esp+Ch] [ebp-16Ch]

  char v142; // [esp+Ch] [ebp-16Ch]

  char v143; // [esp+Ch] [ebp-16Ch]

  char v144; // [esp+Ch] [ebp-16Ch]

  char v145; // [esp+Ch] [ebp-16Ch]

  char v146; // [esp+Ch] [ebp-16Ch]

  unsigned int v147; // [esp+10h] [ebp-168h]

  double v148; // [esp+10h] [ebp-168h]

  float v149; // [esp+10h] [ebp-168h]

  unsigned __int8 *v150; // [esp+18h] [ebp-160h]

  int v151; // [esp+18h] [ebp-160h]

  int v152; // [esp+1Ch] [ebp-15Ch]

  float v153; // [esp+1Ch] [ebp-15Ch]

  int v154; // [esp+20h] [ebp-158h]

  float v155; // [esp+20h] [ebp-158h]

  int v156; // [esp+24h] [ebp-154h]

  float v157; // [esp+24h] [ebp-154h]

  __int16 v158; // [esp+28h] [ebp-150h]

  int v159; // [esp+28h] [ebp-150h]

  float v160; // [esp+28h] [ebp-150h]

  int v161; // [esp+2Ch] [ebp-14Ch]

  float v162; // [esp+2Ch] [ebp-14Ch]

  int v163; // [esp+30h] [ebp-148h]

  float v164; // [esp+30h] [ebp-148h]

  int v165; // [esp+34h] [ebp-144h]

  float v166; // [esp+34h] [ebp-144h]

  int v167; // [esp+38h] [ebp-140h]

  float v168; // [esp+38h] [ebp-140h]

  int v169; // [esp+3Ch] [ebp-13Ch]

  float v170; // [esp+3Ch] [ebp-13Ch]

  int v171; // [esp+40h] [ebp-138h]

  int v172; // [esp+44h] [ebp-134h]

  unsigned __int16 v173; // [esp+44h] [ebp-134h]

  int v174; // [esp+48h] [ebp-130h]

  float v175; // [esp+48h] [ebp-130h]

  float v176; // [esp+4Ch] [ebp-12Ch]

  int v177; // [esp+50h] [ebp-128h]

  float v178; // [esp+50h] [ebp-128h]

  int v179; // [esp+54h] [ebp-124h]

  float v180; // [esp+54h] [ebp-124h]

  int v181; // [esp+58h] [ebp-120h]

  float v182; // [esp+58h] [ebp-120h]

  int v183; // [esp+5Ch] [ebp-11Ch]

  float v184; // [esp+5Ch] [ebp-11Ch]

  float v185; // [esp+60h] [ebp-118h]

  int v186; // [esp+64h] [ebp-114h]

  unsigned __int8 *v187; // [esp+68h] [ebp-110h]

  int v188; // [esp+68h] [ebp-110h]

  int v189; // [esp+6Ch] [ebp-10Ch]

  int v190; // [esp+70h] [ebp-108h]

  int v191; // [esp+74h] [ebp-104h]

  int v192; // [esp+78h] [ebp-100h]

  int v193; // [esp+7Ch] [ebp-FCh]

  int v194; // [esp+80h] [ebp-F8h]

  int v195; // [esp+84h] [ebp-F4h]

  int v196; // [esp+88h] [ebp-F0h]

  int v197; // [esp+8Ch] [ebp-ECh]

  int v198; // [esp+90h] [ebp-E8h]

  int v199; // [esp+94h] [ebp-E4h]

  int v200; // [esp+98h] [ebp-E0h]

  int v201; // [esp+9Ch] [ebp-DCh]

  int v202; // [esp+A0h] [ebp-D8h]

  int v203; // [esp+A4h] [ebp-D4h]

  int v204; // [esp+A8h] [ebp-D0h]

  int v205; // [esp+ACh] [ebp-CCh]

  int v206; // [esp+B0h] [ebp-C8h]

  int v207; // [esp+B4h] [ebp-C4h]

  int v208; // [esp+B8h] [ebp-C0h]

  int v209; // [esp+BCh] [ebp-BCh]

  int v210; // [esp+C0h] [ebp-B8h]

  int v211; // [esp+C4h] [ebp-B4h]

  int v212; // [esp+C8h] [ebp-B0h]

  int v213; // [esp+CCh] [ebp-ACh]

  int v214; // [esp+D0h] [ebp-A8h]

  int v215; // [esp+D4h] [ebp-A4h]

  int v216; // [esp+D8h] [ebp-A0h]

  int v217; // [esp+DCh] [ebp-9Ch]

  int v218; // [esp+E0h] [ebp-98h]

  int v219; // [esp+E4h] [ebp-94h]

  unsigned __int8 bits[4]; // [esp+E8h] [ebp-90h]

  int v221; // [esp+ECh] [ebp-8Ch]

  int v222; // [esp+100h] [ebp-78h]

  int v223; // [esp+104h] [ebp-74h]

  int v224; // [esp+108h] [ebp-70h]

  int v225; // [esp+10Ch] [ebp-6Ch]

  unsigned __int8 *v226; // [esp+110h] [ebp-68h]

  int v227; // [esp+114h] [ebp-64h]

  double v228; // [esp+118h] [ebp-60h]

  double v229; // [esp+118h] [ebp-60h]

  double v230; // [esp+118h] [ebp-60h]

  double v231; // [esp+120h] [ebp-58h]

  unsigned __int8 block[4][16]; // [esp+130h] [ebp-48h] BYREF



  v7 = dest_line;

  v8 = 0;

  v226 = dest_line;

  static int __isa_available = 0; // force scalar path
  static int _sqr[256];
  if ( __isa_available < 2 )

  {

    do

    {

      _sqr[v8] = v8 * v8;

      ++v8;

    }

    while ( v8 < 256 );

  }

  else

  {
    // SSE2 path (equivalent, using intrinsics) - stubbed, scalar path used
    for (int i = 0; i < 256; ++i)
      _sqr[i] = i * i;
  }

  v17 = 0;

  v227 = 0;

  if ( height <= 0 )

    return;

  v18 = width;

  v19 = src_line;

  do

  {

    v150 = v7;

    v20 = v19;

    v21 = 0;

    v225 = 0;

    if ( v18 <= 0 )

      goto LABEL_371;

    v22 = 0;

    if ( v17 + 1 < height )

      v22 = src_pitch;

    v223 = v22;

    v23 = 2 * src_pitch;

    if ( v17 + 2 >= height )

      v23 = 0;

    v224 = v23;

    v18 = width;

    do

    {

      v137 = 4;

      v24 = 0;

      if ( v21 + 1 < v18 )

        v24 = 4;

      v25 = v21 + 2;

      v26 = 0;

      v27 = v25 < v18;

      v28 = &block[0][2];

      if ( v27 )

        v26 = 8;

      v29 = v26 + v24;

      do

      {

        v28 += 16;

        *(v28 - 18) = *v20;

        *(v28 - 17) = v20[v24];

        *(v28 - 16) = v20[v26];

        v30 = v20[v29];

        v31 = &v20[v223];

        *(v28 - 15) = v30;

        *(v28 - 14) = *v31;

        *(v28 - 13) = v31[v24];

        *(v28 - 12) = v31[v26];

        v32 = v31[v29];

        v33 = &v31[v224];

        *(v28 - 11) = v32;

        *(v28 - 6) = *v33;

        *(v28 - 5) = v33[v24];

        *(v28 - 4) = v33[v26];

        v34 = v33[v29];

        v35 = &v33[-v223];

        *(v28 - 3) = v34;

        *(v28 - 10) = *v35;

        *(v28 - 9) = v35[v24];

        *(v28 - 8) = v35[v26];

        *(v28 - 7) = v35[v29];

        v20 = &v35[1 - v224];

        --v137;

      }

      while ( v137 );

      v36 = v150;

      v187 = v20;

      if ( dxt == 3 )

      {

        v37 = block[3][6];

        v38 = block[3][5] & 0xF0;

        *(_WORD *)v150 = (block[3][0] >> 4) | block[3][1] & 0xF0 | (16

                                                                  * (block[3][2] & 0xF0 | (16 * (block[3][3] & 0xF0))));

        v39 = (unsigned __int8)((block[3][4] >> 4) | v38) | (unsigned __int16)(16

                                                                             * (v37 & 0xF0 | (16 * (block[3][7] & 0xF0))));

        v40 = block[3][10];

        v41 = block[3][9] & 0xF0;

        *((_WORD *)v150 + 1) = v39;

        v42 = (unsigned __int8)((block[3][8] >> 4) | v41) | (unsigned __int16)(16

                                                                             * (v40 & 0xF0 | (16 * (block[3][11] & 0xF0))));

        v43 = block[3][14];

        v44 = block[3][13] & 0xF0;

        *((_WORD *)v150 + 2) = v42;

        *((_WORD *)v150 + 3) = (unsigned __int8)((block[3][12] >> 4) | v44) | (unsigned __int16)(16

                                                                                               * (v43 & 0xF0 | (16 * (block[3][15] & 0xF0))));

LABEL_101:

        v36 += 8;

        goto LABEL_102;

      }

      if ( dxt == 5 )

      {

        v45 = block[3][0];

        v46 = block[3][0];

        v147 = block[3][0];

        if ( block[3][1] >= (unsigned int)block[3][0] )

        {

          if ( block[3][1] > (unsigned int)block[3][0] )

            v46 = block[3][1];

          v147 = v46;

        }

        else

        {

          v45 = block[3][1];

        }

        if ( block[3][2] >= v45 )

        {

          if ( block[3][2] > v46 )

            v46 = block[3][2];

          v147 = v46;

        }

        else

        {

          v45 = block[3][2];

        }

        if ( block[3][3] >= v45 )

        {

          if ( block[3][3] > v46 )

            v46 = block[3][3];

          v147 = v46;

        }

        else

        {

          v45 = block[3][3];

        }

        if ( block[3][4] >= v45 )

        {

          if ( block[3][4] > v46 )

            v46 = block[3][4];

          v147 = v46;

        }

        else

        {

          v45 = block[3][4];

        }

        if ( block[3][5] >= v45 )

        {

          if ( block[3][5] > v46 )

            v46 = block[3][5];

          v147 = v46;

        }

        else

        {

          v45 = block[3][5];

        }

        if ( block[3][6] >= v45 )

        {

          if ( block[3][6] > v46 )

            v46 = block[3][6];

          v147 = v46;

        }

        else

        {

          v45 = block[3][6];

        }

        if ( block[3][7] >= v45 )

        {

          if ( block[3][7] > v46 )

            v46 = block[3][7];

          v147 = v46;

        }

        else

        {

          v45 = block[3][7];

        }

        if ( block[3][8] >= v45 )

        {

          if ( block[3][8] > v46 )

            v46 = block[3][8];

          v147 = v46;

        }

        else

        {

          v45 = block[3][8];

        }

        if ( block[3][9] >= v45 )

        {

          if ( block[3][9] > v46 )

            v46 = block[3][9];

          v147 = v46;

        }

        else

        {

          v45 = block[3][9];

        }

        if ( block[3][10] >= v45 )

        {

          if ( block[3][10] > v46 )

            v46 = block[3][10];

          v147 = v46;

        }

        else

        {

          v45 = block[3][10];

        }

        if ( block[3][11] >= v45 )

        {

          if ( block[3][11] > v46 )

            v46 = block[3][11];

          v147 = v46;

        }

        else

        {

          v45 = block[3][11];

        }

        if ( block[3][12] >= v45 )

        {

          if ( block[3][12] > v46 )

            v46 = block[3][12];

          v147 = v46;

        }

        else

        {

          v45 = block[3][12];

        }

        if ( block[3][13] >= v45 )

        {

          if ( block[3][13] > v46 )

            v46 = block[3][13];

          v147 = v46;

        }

        else

        {

          v45 = block[3][13];

        }

        if ( block[3][14] >= v45 )

        {

          if ( block[3][14] > v46 )

            v46 = block[3][14];

          v147 = v46;

        }

        else

        {

          v45 = block[3][14];

        }

        if ( block[3][15] >= v45 )

        {

          if ( block[3][15] > v46 )

            v46 = block[3][15];

          v147 = v46;

        }

        else

        {

          v45 = block[3][15];

        }

        v158 = v46 | ((_WORD)v45 << 8);

        if ( v45 == v46 )

        {

          *(_WORD *)v150 = v46 | ((_WORD)v45 << 8);

          *(_DWORD *)(v150 + 2) = 0;

          *((_WORD *)v150 + 3) = 0;

        }

        else

        {

          v47 = 15 * v45;

          v48 = 2 * (v46 - v45);

          v49 = alphas[(int)(v46 + 14 * block[3][0] - 15 * v45) / v48] | (8

                                                                        * (alphas[(int)(v46 + 14 * block[3][1]

                                                                                            - 15 * v45)

                                                                                / v48] | (8

                                                                                        * (alphas[(int)(v46 + 14 * block[3][2] - 15 * v45)

                                                                                                / v48] | (8 * (alphas[(int)(v46 + 14 * block[3][3] - 15 * v45) / v48] | (8 * (alphas[(int)(v46 + 14 * block[3][4] - 15 * v45) / v48] | (8 * (alphas[(int)(v46 + 14 * block[3][5] - 15 * v45) / v48] | (8 * (alphas[(int)(v46 + 14 * block[3][6] - 15 * v45) / v48] | (8 * alphas[(int)(v46 + 14 * block[3][7] - 15 * v45) / v48])))))))))))));

          v50 = 8

              * (alphas[(int)(v147 + 14 * block[3][9] - v47) / v48] | (8

                                                                     * (alphas[(int)(v147 + 14 * block[3][10] - v47)

                                                                             / v48] | (8

                                                                                     * (alphas[(int)(v147 + 14 * block[3][11] - v47)

                                                                                             / v48] | (8 * (alphas[(int)(v147 + 14 * block[3][12] - v47) / v48] | (8 * (alphas[(int)(v147 + 14 * block[3][13] - v47) / v48] | (8 * (alphas[(int)(v147 + 14 * block[3][14] - v47) / v48] | (8 * alphas[(int)(v46 + 14 * block[3][15] - v47) / v48]))))))))))));

          v51 = 14 * block[3][8] - v47;

          v36 = v150;

          v52 = (int)(v147 + v51) / v48;

          v20 = v187;

          v53 = alphas[v52] | v50;

          *(_WORD *)v150 = v158;

          *((_WORD *)v150 + 1) = v49;

          *((_WORD *)v150 + 2) = ((_WORD)v53 << 8) | BYTE2(v49);

          *((_WORD *)v150 + 3) = v53 >> 8;

        }

        goto LABEL_101;

      }

LABEL_102:

      v194 = block[0][3];

      v197 = block[0][4];

      v200 = block[0][5];

      v203 = block[0][6];

      v219 = block[0][7];

      v217 = block[0][8];

      v215 = block[0][9];

      v210 = block[0][10];

      v206 = block[0][11];

      v188 = block[0][12];

      v189 = block[0][13];

      v191 = block[0][14];

      v167 = block[0][0];

      v163 = block[0][2];

      v171 = block[0][15];

      v174 = block[0][1];

      v154 = block[0][0]

           + block[0][1]

           + block[0][2]

           + block[0][3]

           + block[0][4]

           + block[0][5]

           + block[0][6]

           + block[0][7]

           + block[0][8]

           + block[0][9]

           + block[0][10]

           + block[0][11]

           + block[0][12]

           + block[0][13]

           + block[0][15]

           + block[0][14];

      v192 = block[1][2];

      v193 = block[1][3];

      v196 = block[1][4];

      v199 = block[1][5];

      v202 = block[1][6];

      v204 = block[1][7];

      v218 = block[1][8];

      v152 = block[1][9];

      v211 = block[1][10];

      v207 = block[1][11];

      v205 = block[1][12];

      v186 = block[1][13];

      v190 = block[1][14];

      v165 = block[1][1];

      v54 = 16

          * (_sqr[block[0][0]]

           + _sqr[block[0][1]]

           + _sqr[block[0][2]]

           + _sqr[block[0][3]]

           + _sqr[block[0][4]]

           + _sqr[block[0][5]]

           + _sqr[block[0][6]]

           + _sqr[block[0][7]]

           + _sqr[block[0][8]]

           + _sqr[block[0][9]]

           + _sqr[block[0][10]]

           + _sqr[block[0][11]]

           + _sqr[block[0][12]]

           + _sqr[block[0][13]]

           + _sqr[block[0][14]]

           + _sqr[block[0][15]])

          - v154 * v154;

      v212 = block[1][0];

      v172 = block[1][15];

      v169 = block[1][0]

           + block[1][2]

           + block[1][3]

           + block[1][4]

           + block[1][5]

           + block[1][6]

           + block[1][7]

           + block[1][8]

           + block[1][9]

           + block[1][10]

           + block[1][11]

           + block[1][12]

           + block[1][13]

           + block[1][15]

           + block[1][14]

           + block[1][1];

      v213 = block[2][1];

      v179 = block[2][2];

      v195 = block[2][3];

      v198 = block[2][4];

      v201 = block[2][5];

      v177 = block[2][6];

      v183 = block[2][7];

      v216 = block[2][8];

      v214 = block[2][9];

      v208 = block[2][10];

      v209 = block[2][11];

      v159 = block[2][12];

      v156 = block[2][13];

      v161 = block[2][14];

      v55 = 16

          * (_sqr[block[1][0]]

           + _sqr[block[1][1]]

           + _sqr[block[1][2]]

           + _sqr[block[1][3]]

           + _sqr[block[1][4]]

           + _sqr[block[1][5]]

           + _sqr[block[1][6]]

           + _sqr[block[1][7]]

           + _sqr[block[1][8]]

           + _sqr[block[1][9]]

           + _sqr[block[1][10]]

           + _sqr[block[1][11]]

           + _sqr[block[1][12]]

           + _sqr[block[1][13]]

           + _sqr[block[1][14]]

           + _sqr[block[1][15]])

          - v169 * v169;

      v181 = block[2][0];

      v138 = block[2][15];

      v151 = block[2][0]

           + block[2][1]

           + block[2][2]

           + block[2][3]

           + block[2][4]

           + block[2][5]

           + block[2][6]

           + block[2][7]

           + block[2][8]

           + block[2][9]

           + block[2][10]

           + block[2][11]

           + block[2][12]

           + block[2][13]

           + block[2][15]

           + block[2][14];

      v56 = 16

          * (_sqr[block[2][0]]

           + _sqr[block[2][1]]

           + _sqr[block[2][2]]

           + _sqr[block[2][3]]

           + _sqr[block[2][4]]

           + _sqr[block[2][5]]

           + _sqr[block[2][6]]

           + _sqr[block[2][7]]

           + _sqr[block[2][8]]

           + _sqr[block[2][9]]

           + _sqr[block[2][10]]

           + _sqr[block[2][11]]

           + _sqr[block[2][12]]

           + _sqr[block[2][13]]

           + _sqr[block[2][14]]

           + _sqr[block[2][15]])

          - v151 * v151;

      if ( v55 < v54 || v55 < v56 )

      {

        if ( v54 < v56 )

        {

          v54 = 16

              * (block[0][0] * block[2][0]

               + block[0][1] * block[2][1]

               + block[0][2] * block[2][2]

               + block[0][3] * block[2][3]

               + block[0][4] * block[2][4]

               + block[0][5] * block[2][5]

               + block[0][6] * block[2][6]

               + block[0][7] * block[2][7]

               + block[0][8] * block[2][8]

               + block[0][9] * block[2][9]

               + block[0][10] * block[2][10]

               + block[0][11] * block[2][11]

               + block[0][12] * block[2][12]

               + block[0][13] * block[2][13]

               + block[0][14] * block[2][14]

               + block[0][15] * block[2][15])

              - v154 * v151;

          v55 = 16

              * (block[1][0] * block[2][0]

               + block[1][1] * block[2][1]

               + block[1][2] * block[2][2]

               + block[1][3] * block[2][3]

               + block[1][4] * block[2][4]

               + block[1][5] * block[2][5]

               + block[1][6] * block[2][6]

               + block[1][7] * block[2][7]

               + block[1][8] * block[2][8]

               + block[1][9] * block[2][9]

               + block[1][10] * block[2][10]

               + block[1][11] * block[2][11]

               + block[1][12] * block[2][12]

               + block[1][13] * block[2][13]

               + block[1][14] * block[2][14]

               + block[1][15] * block[2][15])

              - v169 * v151;

        }

        else

        {

          v55 = 16

              * (block[0][0] * block[1][0]

               + block[0][1] * block[1][1]

               + block[0][2] * block[1][2]

               + block[0][3] * block[1][3]

               + block[0][4] * block[1][4]

               + block[0][5] * block[1][5]

               + block[0][6] * block[1][6]

               + block[0][7] * block[1][7]

               + block[0][8] * block[1][8]

               + block[0][9] * block[1][9]

               + block[0][10] * block[1][10]

               + block[0][11] * block[1][11]

               + block[0][12] * block[1][12]

               + block[0][13] * block[1][13]

               + block[0][14] * block[1][14]

               + block[0][15] * block[1][15])

              - v154 * v169;

          v56 = 16

              * (block[0][0] * block[2][0]

               + block[0][1] * block[2][1]

               + block[0][2] * block[2][2]

               + block[0][3] * block[2][3]

               + block[0][4] * block[2][4]

               + block[0][5] * block[2][5]

               + block[0][6] * block[2][6]

               + block[0][7] * block[2][7]

               + block[0][8] * block[2][8]

               + block[0][9] * block[2][9]

               + block[0][10] * block[2][10]

               + block[0][11] * block[2][11]

               + block[0][12] * block[2][12]

               + block[0][13] * block[2][13]

               + block[0][14] * block[2][14]

               + block[0][15] * block[2][15])

              - v154 * v151;

        }

      }

      else

      {

        v54 = 16

            * (block[0][0] * block[1][0]

             + block[0][1] * block[1][1]

             + block[0][2] * block[1][2]

             + block[0][3] * block[1][3]

             + block[0][4] * block[1][4]

             + block[0][5] * block[1][5]

             + block[0][6] * block[1][6]

             + block[0][7] * block[1][7]

             + block[0][8] * block[1][8]

             + block[0][9] * block[1][9]

             + block[0][10] * block[1][10]

             + block[0][11] * block[1][11]

             + block[0][12] * block[1][12]

             + block[0][13] * block[1][13]

             + block[0][14] * block[1][14]

             + block[0][15] * block[1][15])

            - v154 * v169;

        v56 = 16

            * (block[1][0] * block[2][0]

             + block[1][1] * block[2][1]

             + block[1][2] * block[2][2]

             + block[1][3] * block[2][3]

             + block[1][4] * block[2][4]

             + block[1][5] * block[2][5]

             + block[1][6] * block[2][6]

             + block[1][7] * block[2][7]

             + block[1][8] * block[2][8]

             + block[1][9] * block[2][9]

             + block[1][10] * block[2][10]

             + block[1][11] * block[2][11]

             + block[1][12] * block[2][12]

             + block[1][13] * block[2][13]

             + block[1][14] * block[2][14]

             + block[1][15] * block[2][15])

            - v169 * v151;

      }

      v228 = (double)v55;

      v148 = (double)v56;

      if ( (double)v54 * (double)v54 + (double)v55 * (double)v55 + (double)v56 * (double)v56 < 0.0001 )

      {

        v134 = ((v154 + 8) >> 7) + 8 * ((((v169 + 8) >> 4) & 0xFC) + 32 * (((v151 + 8) >> 4) & 0xFFF8));

        if ( dxt == 1 )

        {

          v135 = block[3][3] < 0x80u;

          *(_WORD *)v36 = v134;

          *((_WORD *)v36 + 1) = v134;

          LOBYTE(v222) = (block[3][1] < 0x80u ? 0xC : 0) | (block[3][0] < 0x80u ? 3 : 0) | (block[3][2] < 0x80u

                                                                                          ? 0x30

                                                                                          : 0) | (v135 ? 0xC0 : 0);

          BYTE1(v222) = (block[3][5] < 0x80u ? 0xC : 0) | (block[3][4] < 0x80u ? 3 : 0) | (block[3][6] < 0x80u ? 0x30 : 0) | (block[3][7] < 0x80u ? 0xC0 : 0);

          BYTE2(v222) = (block[3][9] < 0x80u ? 0xC : 0) | (block[3][8] < 0x80u ? 3 : 0) | (block[3][10] < 0x80u

                                                                                         ? 0x30

                                                                                         : 0) | (block[3][11] < 0x80u

                                                                                               ? 0xC0

                                                                                               : 0);

          HIBYTE(v222) = (block[3][13] < 0x80u ? 0xC : 0) | (block[3][12] < 0x80u ? 3 : 0) | (block[3][14] < 0x80u

                                                                                            ? 0x30

                                                                                            : 0) | (block[3][15] < 0x80u

                                                                                                  ? 0xC0

                                                                                                  : 0);

          *((_DWORD *)v36 + 1) = v222;

          goto LABEL_369;

        }

        *(_WORD *)v36 = v134;

        goto LABEL_368;

      }

      v57 = (float)(sqrt((double)((double)v54 * (double)v54 + (double)v55 * (double)v55 + (double)v56 * (double)v56)));


      v58 = (float)((double)v54 * (1.0f / v57));


      v185 = v58;

      v59 = (float)(v228 * (1.0f / v57));


      v60 = (float)(v148 * (1.0f / v57));


      v231 = (double)v154 * 0.0625;

      v229 = (double)v169;

      v176 = v59;

      v149 = v60;

      v61 = (float)((double)v169 * 0.0625f * v59 + v58 * v231 + (double)v151 * 0.0625f * v60);


      v182 = (float)((float)((float)((float)v212 * v59) + (float)((float)v167 * v58)) + (float)((float)v181 * v60))

           - v61;

      v175 = (float)((float)((float)((float)v165 * v59) + (float)((float)v174 * v58)) + (float)((float)v213 * v60))

           - v61;

      v62 = (float)((float)((float)((float)v192 * v59) + (float)((float)v163 * v58)) + (float)((float)v179 * v60)) - v61;

      v180 = v62;

      v170 = (float)((float)((float)((float)v193 * v59) + (float)((float)v194 * v58)) + (float)((float)v195 * v60))

           - v61;

      v164 = (float)((float)((float)((float)v196 * v59) + (float)((float)v197 * v58)) + (float)((float)v198 * v60))

           - v61;

      v63 = (float)((float)((float)((float)v199 * v59) + (float)((float)v200 * v58)) + (float)((float)v201 * v60)) - v61;

      v64 = (float)((float)((float)((float)v202 * v59) + (float)((float)v203 * v58)) + (float)((float)v177 * v149))

          - v61;

      v178 = v64;

      v65 = (float)((float)((float)((float)v204 * v59) + (float)((float)v219 * v58)) + (float)((float)v183 * v149))

          - v61;

      v184 = v65;

      v166 = (float)((float)((float)((float)v218 * v176) + (float)((float)v217 * v58)) + (float)((float)v216 * v149))

           - v61;

      v153 = (float)((float)((float)((float)v152 * v176) + (float)((float)v215 * v58)) + (float)((float)v214 * v149))

           - v61;

      v168 = (float)((float)((float)((float)v211 * v176) + (float)((float)v210 * v58)) + (float)((float)v208 * v149))

           - v61;

      v155 = (float)((float)((float)((float)v207 * v176) + (float)((float)v206 * v58)) + (float)((float)v209 * v149))

           - v61;

      v160 = (float)((float)((float)((float)v205 * v176) + (float)((float)v188 * v58)) + (float)((float)v159 * v149))

           - v61;

      v157 = (float)((float)((float)((float)v186 * v176) + (float)((float)v189 * v58)) + (float)((float)v156 * v149))

           - v61;

      v162 = (float)((float)((float)((float)v190 * v176) + (float)((float)v191 * v58)) + (float)((float)v161 * v149))

           - v61;

      v66 = v182;

      v67 = (float)((float)((float)((float)v172 * v176) + (float)((float)v171 * v58)) + (float)((float)v138 * v149))

          - v61;

      v68 = v182;

      if ( v182 <= (double)v175 )

        v68 = (float)(fmax(v175, v66));


      else

        v66 = v175;

      if ( v66 <= v62 )

        v68 = (float)(fmax(v62, v68));


      else

        v66 = v62;

      if ( v66 <= v170 )

        v68 = (float)(fmax(v170, v68));


      else

        v66 = v170;

      if ( v66 <= v164 )

        v68 = (float)(fmax(v164, v68));


      else

        v66 = v164;

      if ( v66 <= v63 )

        v68 = (float)(fmax(v63, v68));


      else

        v66 = v63;

      if ( v66 <= v64 )

        v68 = (float)(fmax(v64, v68));


      else

        v66 = v64;

      if ( v66 <= v65 )

        v68 = (float)(fmax(v65, v68));


      else

        v66 = v65;

      if ( v66 <= v166 )

        v68 = (float)(fmax(v166, v68));


      else

        v66 = v166;

      if ( v66 <= v153 )

        v68 = (float)(fmax(v153, v68));


      else

        v66 = v153;

      if ( v66 <= v168 )

        v68 = (float)(fmax(v168, v68));


      else

        v66 = v168;

      if ( v66 <= v155 )

        v68 = (float)(fmax(v155, v68));


      else

        v66 = v155;

      if ( v66 <= v160 )

        v68 = (float)(fmax(v160, v68));


      else

        v66 = v160;

      if ( v66 <= v157 )

        v68 = (float)(fmax(v157, v68));


      else

        v66 = v157;

      if ( v66 <= v162 )

        v68 = (float)(fmax(v162, v68));


      else

        v66 = v162;

      if ( v66 <= v67 )

        v68 = (float)(fmax(v67, v68));


      else

        v66 = v67;

      v230 = v229 * 0.0625;

      v69 = (float)((double)v151 * 0.0625f);


      v70 = (int)(v185 * v66 + v231 + 0.5);

      v71 = (int)(v176 * v66 + v230 + 0.5);

      v72 = (int)(v149 * v66 + v69 + 0.5);

      if ( v70 <= 255 )

      {

        if ( v70 < 0 )

          v70 = 0;

      }

      else

      {

        v70 = 255;

      }

      if ( v71 <= 255 )

      {

        if ( v71 < 0 )

          LOBYTE(v71) = 0;

      }

      else

      {

        LOBYTE(v71) = -1;

      }

      if ( v72 <= 255 )

      {

        if ( v72 < 0 )

          LOWORD(v72) = 0;

      }

      else

      {

        LOWORD(v72) = 255;

      }

      v73 = (v70 >> 3) + 8 * ((v71 & 0xFC) + 32 * (v72 & 0xFFF8));

      v74 = (int)(v185 * v68 + v231 + 0.5);

      v75 = (int)(v176 * v68 + v230 + 0.5);

      v76 = (int)(v149 * v68 + v69 + 0.5);

      if ( v74 <= 255 )

      {

        if ( v74 < 0 )

          v74 = 0;

      }

      else

      {

        v74 = 255;

      }

      if ( v75 <= 255 )

      {

        if ( v75 < 0 )

          LOBYTE(v75) = 0;

      }

      else

      {

        LOBYTE(v75) = -1;

      }

      if ( v76 <= 255 )

      {

        if ( v76 < 0 )

          LOWORD(v76) = 0;

      }

      else

      {

        LOWORD(v76) = 255;

      }

      v173 = (v74 >> 3) + 8 * ((v75 & 0xFC) + 32 * (v76 & 0xFFF8));

      if ( (block[3][3] >= 0x80u)

         + (block[3][2] >= 0x80u)

         + (block[3][1] >= 0x80u)

         + (block[3][0] >= 0x80u)

         + (block[3][7] >= 0x80u)

         + (block[3][6] >= 0x80u)

         + (block[3][5] >= 0x80u)

         + (block[3][4] >= 0x80u)

         + (block[3][11] >= 0x80u)

         + (block[3][10] >= 0x80u)

         + (block[3][9] >= 0x80u)

         + (block[3][8] >= 0x80u)

         + (block[3][15] >= 0x80u)

         + (block[3][14] >= 0x80u)

         + (block[3][13] >= 0x80u)

         + (block[3][12] >= 0x80u) < 16

        && dxt == 1 )

      {

        v77 = v68 * 0.25f + v66 * 0.75f;

        v78 = v68 * 0.75f + v66 * 0.25f;

        if ( block[3][0] >= 0x80u )

        {

          if ( v182 <= v78 )

          {

            v79 = 0;

            if ( v182 > v77 )

              v79 = 2;

            v139 = v79;

          }

          else

          {

            v139 = 1;

          }

        }

        else

        {

          v139 = 3;

        }

        if ( block[3][1] >= 0x80u )

        {

          if ( v175 <= v78 )

          {

            v80 = 0;

            if ( v175 > v77 )

              v80 = 8;

          }

          else

          {

            v80 = 4;

          }

        }

        else

        {

          v80 = 12;

        }

        if ( block[3][2] >= 0x80u )

        {

          if ( v180 <= v78 )

          {

            v81 = 0;

            if ( v180 > v77 )

              v81 = 32;

          }

          else

          {

            v81 = 16;

          }

        }

        else

        {

          v81 = 48;

        }

        if ( block[3][3] >= 0x80u )

        {

          if ( v170 <= v78 )

            v82 = v170 <= v77 ? 0 : 0x80;

          else

            v82 = 64;

        }

        else

        {

          v82 = -64;

        }

        bits[0] = v139 | v80 | v81 | v82;

        if ( block[3][4] >= 0x80u )

        {

          if ( v164 <= v78 )

          {

            v83 = 0;

            if ( v164 > v77 )

              v83 = 2;

            v140 = v83;

          }

          else

          {

            v140 = 1;

          }

        }

        else

        {

          v140 = 3;

        }

        if ( block[3][5] >= 0x80u )

        {

          if ( v63 <= v78 )

          {

            v84 = 0;

            if ( v63 > v77 )

              v84 = 8;

          }

          else

          {

            v84 = 4;

          }

        }

        else

        {

          v84 = 12;

        }

        if ( block[3][6] >= 0x80u )

        {

          if ( v178 <= v78 )

          {

            v85 = 0;

            if ( v178 > v77 )

              v85 = 32;

          }

          else

          {

            v85 = 16;

          }

        }

        else

        {

          v85 = 48;

        }

        if ( block[3][7] >= 0x80u )

        {

          if ( v65 <= v78 )

            v86 = v65 <= v77 ? 0 : 0x80;

          else

            v86 = 64;

        }

        else

        {

          v86 = -64;

        }

        bits[1] = v86 | v84 | v140 | v85;

        if ( block[3][8] >= 0x80u )

        {

          if ( v166 <= v78 )

          {

            v87 = 0;

            if ( v166 > v77 )

              v87 = 2;

            v141 = v87;

          }

          else

          {

            v141 = 1;

          }

        }

        else

        {

          v141 = 3;

        }

        if ( block[3][9] >= 0x80u )

        {

          if ( v153 <= v78 )

          {

            v88 = 0;

            if ( v153 > v77 )

              v88 = 8;

          }

          else

          {

            v88 = 4;

          }

        }

        else

        {

          v88 = 12;

        }

        if ( block[3][10] >= 0x80u )

        {

          if ( v168 <= v78 )

          {

            v89 = 0;

            if ( v168 > v77 )

              v89 = 32;

          }

          else

          {

            v89 = 16;

          }

        }

        else

        {

          v89 = 48;

        }

        if ( block[3][11] >= 0x80u )

        {

          if ( v155 <= v78 )

            v90 = v155 <= v77 ? 0 : 0x80;

          else

            v90 = 64;

        }

        else

        {

          v90 = -64;

        }

        bits[2] = v90 | v88 | v141 | v89;

        if ( block[3][12] >= 0x80u )

        {

          if ( v160 <= v78 )

          {

            v91 = 0;

            if ( v160 > v77 )

              v91 = 2;

            v142 = v91;

          }

          else

          {

            v142 = 1;

          }

        }

        else

        {

          v142 = 3;

        }

        if ( block[3][13] >= 0x80u )

        {

          if ( v157 <= v78 )

          {

            v92 = 0;

            if ( v157 > v77 )

              v92 = 8;

          }

          else

          {

            v92 = 4;

          }

        }

        else

        {

          v92 = 12;

        }

        if ( block[3][14] >= 0x80u )

        {

          if ( v162 <= v78 )

          {

            v93 = 0;

            if ( v162 > v77 )

              v93 = 32;

          }

          else

          {

            v93 = 16;

          }

        }

        else

        {

          v93 = 48;

        }

        if ( block[3][15] >= 0x80u )

        {

          if ( v67 <= v78 )

            v94 = v67 <= v77 ? 0 : 0x80;

          else

            v94 = 64;

        }

        else

        {

          v94 = -64;

        }

        bits[3] = v94 | v92 | v142 | v93;

        if ( v73 > v173 )

        {

          *(_WORD *)v36 = v173;

          *((_WORD *)v36 + 1) = v73;

          *((_DWORD *)v36 + 1) = *(_DWORD *)bits ^ ~(*(_DWORD *)bits >> 1) & 0x55555555;

        }

        else

        {

          *(_WORD *)v36 = v73;

          *((_WORD *)v36 + 1) = v173;

          *((_DWORD *)v36 + 1) = *(_DWORD *)bits;

        }

      }

      else

      {

        v95 = 0;

        v96 = (v68 + v66) * 0.5f;

        v97 = (v66 * 5.0f + v68) / 6.0f;

        v98 = (v68 * 5.0f + v66) / 6.0f;

        v99 = v182;

        if ( v182 <= v96 )

        {

          if ( v99 > v97 )

            v95 = 2;

        }

        else

        {

          v95 = 2 * (v99 <= v98) + 1;

        }

        v100 = 0;

        v101 = v175;

        if ( v175 <= v96 )

        {

          if ( v101 > v97 )

            v100 = 8;

        }

        else

        {

          v100 = 8 * (v101 <= v98) + 4;

        }

        v102 = v180;

        if ( v180 <= v96 )

        {

          v103 = 0;

          if ( v102 > v97 )

            v103 = 32;

        }

        else

        {

          v103 = 48;

          if ( v102 > v98 )

            v103 = 16;

        }

        v143 = v103;

        v104 = v170;

        if ( v170 <= v96 )

        {

          v105 = 0;

          if ( v104 > v97 )

            v105 = (char)128;


        }

        else

        {

          v105 = -64;

          if ( v104 > v98 )

            v105 = 64;

        }

        v106 = v95 | v100 | v105;

        v107 = 0;

        LOBYTE(v221) = v143 | v106;

        v108 = v164;

        if ( v164 <= v96 )

        {

          if ( v108 > v97 )

            v107 = 2;

        }

        else

        {

          v107 = 2 * (v108 <= v98) + 1;

        }

        v109 = 0;

        v110 = v63;

        if ( v63 <= v96 )

        {

          if ( v110 > v97 )

            v109 = 8;

        }

        else

        {

          v109 = 8 * (v110 <= v98) + 4;

        }

        v111 = v178;

        if ( v178 <= v96 )

        {

          v112 = 0;

          if ( v111 > v97 )

            v112 = 32;

        }

        else

        {

          v112 = 48;

          if ( v111 > v98 )

            v112 = 16;

        }

        v144 = v112;

        v113 = v184;

        if ( v184 <= v96 )

        {

          v114 = 0;

          if ( v113 > v97 )

            v114 = (char)128;


        }

        else

        {

          v114 = -64;

          if ( v113 > v98 )

            v114 = 64;

        }

        v115 = v107 | v114 | v109;

        v116 = 0;

        BYTE1(v221) = v144 | v115;

        v117 = v166;

        if ( v166 <= v96 )

        {

          if ( v117 > v97 )

            v116 = 2;

        }

        else

        {

          v116 = 2 * (v117 <= v98) + 1;

        }

        v118 = 0;

        v119 = v153;

        if ( v153 <= v96 )

        {

          if ( v119 > v97 )

            v118 = 8;

        }

        else

        {

          v118 = 8 * (v119 <= v98) + 4;

        }

        v120 = v168;

        if ( v168 <= v96 )

        {

          v121 = 0;

          if ( v120 > v97 )

            v121 = 32;

        }

        else

        {

          v121 = 48;

          if ( v120 > v98 )

            v121 = 16;

        }

        v145 = v121;

        v122 = v155;

        if ( v155 <= v96 )

        {

          v123 = 0;

          if ( v122 > v97 )

            v123 = (char)128;


        }

        else

        {

          v123 = -64;

          if ( v122 > v98 )

            v123 = 64;

        }

        v124 = v116 | v118 | v123;

        v125 = 0;

        BYTE2(v221) = v145 | v124;

        v126 = v160;

        if ( v160 <= v96 )

        {

          if ( v126 > v97 )

            v125 = 2;

        }

        else

        {

          v125 = 2 * (v126 <= v98) + 1;

        }

        v127 = 0;

        v128 = v157;

        if ( v157 <= v96 )

        {

          if ( v128 > v97 )

            v127 = 8;

        }

        else

        {

          v127 = 8 * (v128 <= v98) + 4;

        }

        v129 = v162;

        if ( v162 <= v96 )

        {

          v130 = 0;

          if ( v129 > v97 )

            v130 = 32;

        }

        else

        {

          v130 = 48;

          if ( v129 > v98 )

            v130 = 16;

        }

        v146 = v130;

        v131 = v67;

        if ( v67 <= v96 )

        {

          v132 = 0;

          if ( v131 > v97 )

            v132 = (char)128;


        }

        else

        {

          v132 = -64;

          if ( v131 > v98 )

            v132 = 64;

        }

        HIBYTE(v221) = v146 | v125 | v127 | v132;

        if ( v73 == v173 )

        {

          *(_WORD *)v36 = v73;

LABEL_368:

          *(_DWORD *)(v36 + 2) = 0;

          *((_WORD *)v36 + 3) = 0;

          goto LABEL_369;

        }

        v133 = v221;

        if ( v73 <= v173 )

        {

          *(_WORD *)v36 = v173;

          v133 = v221 ^ 0x55555555;

          *((_WORD *)v36 + 1) = v73;

        }

        else

        {

          *(_WORD *)v36 = v73;

          *((_WORD *)v36 + 1) = v173;

        }

        *((_DWORD *)v36 + 1) = v133;

      }

LABEL_369:

      v18 = width;

      v21 = v225 + 4;

      v20 += 12;

      v150 = v36 + 8;

      v225 = v21;

    }

    while ( v21 < width );

    v17 = v227;

    v19 = src_line;

LABEL_371:

    v17 += 4;

    v19 += 4 * src_pitch;

    v7 = &v226[dest_pitch];

    src_line = v19;

    v226 += dest_pitch;

    v227 = v17;

  }

  while ( v17 < height );

}

//----- (00464830) --------------------------------------------------------

void __cdecl Blt_DXT_From_SHADOW16(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // edi

  int v7; // ecx

  unsigned __int8 *v8; // eax

  unsigned __int8 *v9; // edx

  unsigned int v10; // esi

  unsigned __int8 *v11; // ebx

  unsigned int v12; // edi

  int v13; // edx

  unsigned int v14; // ecx

  unsigned __int8 *v15; // eax

  int v16; // esi

  int v17; // edx

  unsigned int v18; // ecx

  unsigned __int8 *v19; // eax

  int v20; // esi

  int v21; // edx

  unsigned int v22; // ecx

  unsigned __int8 *v23; // eax

  int v24; // [esp+4h] [ebp-8h]

  unsigned int v25; // [esp+8h] [ebp-4h]



  v6 = width;

  if ( (((unsigned __int8)height | (unsigned __int8)width) & 3) != 0 )

    Logger.g->Panic(

      "::Blt_DXT_From_SHADOW16: Width & Height has to be multiple of 4 (%d x %d)",

      width,

      height);

  if ( height > 0 )

  {

    v7 = src_pitch;

    v8 = src_line;

    v9 = dest_line;

    v10 = ((unsigned int)(height - 1) >> 2) + 1;

    v25 = v10;

    do

    {

      v11 = v9;

      if ( v6 > 0 )

      {

        v24 = 3 * v7;

        v12 = ((unsigned int)(v6 - 1) >> 2) + 1;

        do

        {

          *(_DWORD *)v11 = 0xFFFF;

          v11 += 8;

          v13 = (v8[3] & 6) + 4 * ((v8[5] & 6) + 4 * (v8[7] & 6));

          v14 = v8[1];

          v15 = &v8[src_pitch];

          v16 = ((v14 >> 1) & 3) + 2 * v13;

          v17 = (v15[3] & 6) + 4 * ((v15[5] & 6) + 4 * (v15[7] & 6));

          v18 = v15[1];

          v19 = &v15[src_pitch];

          v20 = ((((v18 >> 1) & 3) + 2 * v17) << 8) | v16;

          v21 = (v19[3] & 6) + 4 * ((v19[5] & 6) + 4 * (v19[7] & 6));

          v22 = v19[1];

          v23 = &v19[src_pitch];

          *((_DWORD *)v11 - 1) = (~(((((v22 >> 1) & 3) + 2 * v21) << 16) | v20 | ((((v23[1] >> 1) & 3)

                                                                                 + 2

                                                                                 * ((v23[3] & 6)

                                                                                  + 4

                                                                                  * ((v23[5] & 6) + 4 * (v23[7] & 0xFE)))) << 24)) >> 1) & 0x55555555;

          v8 = &v23[8 - v24];

          --v12;

        }

        while ( v12 );

        v7 = src_pitch;

        v9 = dest_line;

        v10 = v25;

        v6 = width;

      }

      v9 += dest_pitch;

      v8 = &src_line[4 * v7];

      --v10;

      src_line = v8;

      dest_line = v9;

      v25 = v10;

    }

    while ( v10 );

  }

}



//----- (004649B0) --------------------------------------------------------

void __cdecl Blt_DXT_From_SHADOW32(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // edi

  int v7; // edx

  unsigned __int8 *v8; // eax

  unsigned __int8 *v9; // ecx

  unsigned int v10; // esi

  unsigned __int8 *v11; // ebx

  unsigned int v12; // edi

  int v13; // esi

  unsigned int v14; // ecx

  unsigned __int8 *v15; // eax

  int v16; // esi

  int v17; // edx

  unsigned int v18; // ecx

  unsigned __int8 *v19; // eax

  int v20; // esi

  int v21; // edx

  unsigned int v22; // ecx

  unsigned __int8 *v23; // eax

  unsigned int v24; // esi

  int v25; // [esp+4h] [ebp-8h]

  unsigned int v26; // [esp+8h] [ebp-4h]



  v6 = width;

  if ( (((unsigned __int8)height | (unsigned __int8)width) & 3) != 0 )

    Logger.g->Panic(

      "::Blt_DXT_From_SHADOW32: Width & Height has to be multiple of 4 (%d x %d)",

      width,

      height);

  if ( height > 0 )

  {

    v7 = src_pitch;

    v8 = src_line;

    v9 = dest_line;

    v10 = ((unsigned int)(height - 1) >> 2) + 1;

    v26 = v10;

    do

    {

      v11 = v9;

      if ( v6 > 0 )

      {

        v25 = 3 * v7;

        v12 = ((unsigned int)(v6 - 1) >> 2) + 1;

        do

        {

          *(_DWORD *)v11 = 0xFFFF;

          v11 += 8;

          v13 = (v8[12] & 0xC0) + ((v8[4] >> 4) & 0xC) + ((v8[8] >> 2) & 0x30);

          v14 = *v8;

          v15 = &v8[v7];

          v16 = (v14 >> 6) + v13;

          v17 = (v15[12] & 0xC0) + ((v15[4] >> 4) & 0xC) + ((v15[8] >> 2) & 0x30);

          v18 = *v15;

          v19 = &v15[src_pitch];

          v20 = (((v18 >> 6) + v17) << 8) | v16;

          v21 = (v19[12] & 0xC0) + ((v19[4] >> 4) & 0xC) + ((v19[8] >> 2) & 0x30);

          v22 = *v19;

          v23 = &v19[src_pitch];

          v24 = (((*v23 >> 6) + (v23[12] & 0xC0) + ((v23[4] >> 4) & 0xC) + ((v23[8] >> 2) & 0x30)) << 24) | (((v22 >> 6) + v21) << 16) | v20;

          *((_DWORD *)v11 - 1) = v24 ^ (2 * v24) ^ (v24 ^ (2 * v24) ^ (v24 >> 1)) & 0x55555555;

          v8 = &v23[16 - v25];

          v7 = src_pitch;

          --v12;

        }

        while ( v12 );

        v9 = dest_line;

        v10 = v26;

        v6 = width;

      }

      v9 += dest_pitch;

      v8 = &src_line[4 * v7];

      --v10;

      src_line = v8;

      dest_line = v9;

      v26 = v10;

    }

    while ( v10 );

  }

}



//----- (00464B40) --------------------------------------------------------

void __cdecl Blt_R5G6B5_From_DXT1(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // ecx

  int v7; // eax

  unsigned __int8 *v8; // ebx

  unsigned __int8 *v9; // edx

  int v10; // esi

  int v11; // edi

  unsigned __int16 v12; // ax

  _WORD *v13; // edx

  unsigned __int16 v14; // cx

  unsigned __int16 v15; // si

  unsigned __int16 v16; // ax

  int v17; // ebx

  int v18; // ebx

  int v19; // edi

  unsigned int v20; // kr00_4

  unsigned int v21; // ecx

  unsigned int v22; // edx

  int i; // ecx

  char v24; // al

  unsigned __int8 *v25; // ecx

  int v26; // esi

  int v27; // eax

  unsigned __int8 *v28; // edx

  int v29; // ecx

  unsigned __int8 *v30; // [esp+8h] [ebp-54h]

  int v31; // [esp+10h] [ebp-4Ch]

  unsigned __int8 *v32; // [esp+14h] [ebp-48h]

  int v33; // [esp+18h] [ebp-44h]

  int v34; // [esp+1Ch] [ebp-40h]

  _DWORD *v35; // [esp+20h] [ebp-3Ch]

  unsigned __int8 *v36; // [esp+20h] [ebp-3Ch]

  int v37; // [esp+24h] [ebp-38h]

  int v38; // [esp+28h] [ebp-34h]

  int v39; // [esp+2Ch] [ebp-30h]

  unsigned __int8 *v40; // [esp+2Ch] [ebp-30h]

  unsigned __int16 block[16]; // [esp+30h] [ebp-2Ch]

  unsigned __int16 c[4]; // [esp+50h] [ebp-Ch]



  v6 = height;

  v7 = 0;

  v31 = 0;

  v8 = dest_line;

  v30 = dest_line;

  if ( height > 0 )

  {

    v9 = src_line;

    v10 = 4 * dest_pitch;

    v11 = width;

    do

    {

      v32 = v8;

      v39 = 0;

      if ( v11 > 0 )

      {

        do

        {

          v12 = *(_WORD *)v9;

          v13 = (_WORD *)(v9 + 2);

          v14 = v12;

          c[0] = v12;

          v15 = v12;

          v35 = (_DWORD *)(v13 + 1);

          v16 = *v13;

          c[1] = v16;

          v38 = v16 & 0xF800;

          v37 = v15 & 0xF800;

          v17 = v39;

          if ( v14 <= v16 )

          {

            c[2] = ((unsigned int)(v37 + v38) >> 1) & 0xF800 | ((unsigned __int16)(((v15 & 0x1F) + (v16 & 0x1F)) | ((v15 & 0x7E0) + (v16 & 0x7E0)) & 0xFC0) >> 1);

            c[3] = c[2];

          }

          else

          {

            v34 = v15 & 0x1F;

            v18 = v15 & 0x7E0;

            v33 = v16 & 0x1F;

            v19 = v16 & 0x7E0;

            v20 = v19 + 2 * (v18 + 16);

            v21 = v18 + 2 * (v19 + 16);

            v17 = v39;

            v11 = width;

            c[2] = ((v33 + 2 * v34 + 1) / 3u) & 0x1F | ((v38 + 2048 + 2 * v37) / 3u) & 0xF800 | (v20 / 3) & 0x7E0;

            c[3] = ((v34 + 2 * v33 + 1) / 3u) & 0x1F | ((v37 + 2 * v38 + 2048) / 3u) & 0xF800 | (v21 / 3) & 0x7E0;

          }

          v22 = *v35;

          v36 = (unsigned __int8 *)(v35 + 1);

          for ( i = 0; i < 16; ++i )

          {

            v24 = v22;

            v22 >>= 2;

            block[i] = c[v24 & 3];

          }

          v25 = v32;

          v26 = 0;

          v40 = v32;

          do

          {

            if ( v26 + v31 >= height )

              break;

            v27 = 0;

            v28 = v25;

            do

            {

              if ( v27 + v17 >= v11 )

                break;

              v29 = v27 + 4 * v26;

              ++v27;

              *(_WORD *)v28 = block[v29];

              v28 += 2;

            }

            while ( v27 < 4 );

            ++v26;

            v25 = &v40[dest_pitch];

            v40 += dest_pitch;

          }

          while ( v26 < 4 );

          v32 += 8;

          v9 = v36;

          v39 = v17 + 4;

        }

        while ( v17 + 4 < v11 );

        v7 = v31;

        v6 = height;

        v10 = 4 * dest_pitch;

      }

      v7 += 4;

      v9 = &src_line[src_pitch];

      v8 = &v30[v10];

      src_line += src_pitch;

      v30 += v10;

      v31 = v7;

    }

    while ( v7 < v6 );

  }

}



//----- (00464DA0) --------------------------------------------------------

void __cdecl Blt_R8G8B8_From_X8R8G8B8(

        unsigned __int8 *dest_line,

        int dest_pitch,

        unsigned __int8 *src_line,

        int src_pitch,

        int width,

        int height)

{

  int v6; // edi

  int v7; // ecx

  unsigned __int8 *v8; // ebx

  unsigned __int8 *v9; // eax

  int v10; // esi

  unsigned __int8 *v11; // edx

  unsigned __int8 *v12; // eax

  unsigned __int8 v13; // cl

  unsigned __int8 *heighta; // [esp+20h] [ebp+1Ch]



  v6 = height;

  if ( height )

  {

    v7 = width;

    v8 = src_line + 2;

    v9 = dest_line + 2;

    heighta = dest_line + 2;

    do

    {

      --v6;

      v10 = v7;

      if ( v7 )

      {

        v11 = v9;

        v12 = v8;

        do

        {

          v13 = *(v12 - 2);

          v12 += 4;

          *(v11 - 2) = v13;

          v11 += 3;

          *(v11 - 4) = *(v12 - 5);

          *(v11 - 3) = *(v12 - 4);

          --v10;

        }

        while ( v10 );

        v9 = heighta;

        v7 = width;

      }

      v9 += dest_pitch;

      v8 += src_pitch;

      heighta = v9;

    }

    while ( v6 );

  }

}
