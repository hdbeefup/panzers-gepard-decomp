// mdec/layer3.cpp
// MPEG Layer 3 frame decoding
// Decompiled from: gameSplit/mp3_decoder.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <corecrt_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "decode.h"

// Classes: SMpegAudioDecoder
// Function count: 6

//----- (004700A0) --------------------------------------------------------

int SMpegAudioDecoder::L3_DecodeFrame()

{

  SMpegAudioDecoder *v1; // edi

  int stereo; // edx

  int size; // ecx

  int v4; // eax

  int v5; // ecx

  unsigned int v6; // esi

  unsigned int v8; // esi

  unsigned int *p_block_type; // ecx

  int v10; // esi

  int v11; // esi

  int v12; // edx

  int v13; // ecx

  float *v14; // ecx

  int v15; // esi

  float *v16; // edi

  int v17; // eax

  float *v18; // edx

  int v19; // esi

  float *v20; // eax

  int v21; // ecx

  int v22; // xmm0_4

  float *v23; // eax

  float *v24; // ecx

  float *v25; // edx

  int v26; // esi

  bool v27; // zf

  float *v28; // [esp+8h] [ebp-30h]

  float *v29; // [esp+Ch] [ebp-2Ch]

  int block_end; // [esp+10h] [ebp-28h]

  int block_enda; // [esp+10h] [ebp-28h]

  float *v32; // [esp+14h] [ebp-24h]

  int v34; // [esp+1Ch] [ebp-1Ch]

  int *v35; // [esp+20h] [ebp-18h]

  int *v36; // [esp+24h] [ebp-14h]

  unsigned int channel; // [esp+28h] [ebp-10h]

  unsigned int *channela; // [esp+28h] [ebp-10h]

  int channelb; // [esp+28h] [ebp-10h]

  short *sample; // [esp+2Ch] [ebp-Ch]

  float *v41; // [esp+30h] [ebp-8h]

  int gr; // [esp+34h] [ebp-4h]

  // int savedregs; // IDA stack frame artifact (removed)



  v1 = this;

  this->sfreq = this->Frame.sampling_frequency + 3 * this->Frame.version;

  L3_GetSideInfo();

  stereo = v1->Frame.stereo;

  size = v1->Frame.size;

  if ( v1->Frame.version )

  {

    v4 = 17;

    if ( stereo != 1 )

      v4 = 32;

    v5 = size - v4;

  }

  else if ( stereo == 1 )

  {

    v5 = size - 9;

  }

  else

  {

    v5 = size - 17;

  }

  channel = v5;

  v6 = v1->s2_fillbuf(v5);
  if ( v6 >= channel )

  {

    v8 = v6 - channel;

    if ( v8 >= v1->si.main_data_begin )

    {

      for ( ; v8 > v1->si.main_data_begin; --v8 )

        v1->s2_getbits(8);

      v34 = 0;

      gr = 0;

      if ( v1->Frame.version + 1 > 0 )

      {

        p_block_type = &v1->si.ch[0].gr[0].block_type;

        v35 = (int *)&v1->si.ch[0].gr[0].block_type;

        do

        {

          v10 = 0;

          if ( v1->Frame.stereo > 0 )

          {

            channela = p_block_type - 5;

            do

            {

              block_end = *channela + v1->s2_getpos();

              if ( v1->Frame.version )

                v1->L3_GetMPEG1ScaleFactors(gr, v10);

              else

                v1->L3_GetMPEG2ScaleFactors(gr, v10);

              v1->L3_HuffmanDecode(gr, v10, block_end);

              v1->L3_DequantizeSample(gr, v10);

              channela += 40;

              ++v10;

            }

            while ( v10 < v1->Frame.stereo );

          }

          v11 = gr;

          v1->L3_Stereo(gr);

          v12 = v1->Frame.stereo;

          v13 = 0;

          channelb = 0;

          if ( v12 > 0 )

          {

            v29 = &v1->hybridOut[1][1];

            v28 = v1->hybridOut[1];

            sample = v1->pcmsample[0][0];

            v41 = v1->lr[0];

            v36 = v35;

            do

            {

              v1->L3_Antialias(v11, v13);

              v14 = v41;

              v15 = 0;

              v16 = v1->hybridOut[0];

              do

              {

                if ( *(v36 - 1) && v36[1] && v15 < 2 )

                  v17 = 0;

                else

                  v17 = *v36;

                L3_Hybrid(v14, v16, v15++, channelb, v17);

                v14 = v41 + 18;

                v16 += 18;

                v41 += 18;

              }

              while ( v15 < 32 );

              v18 = v29;

              v19 = 9;

              do

              {

                v20 = v18;

                v21 = 16;

                do

                {

                  v22 = *(_DWORD *)v20;

                  v20 += 36;

                  *((_DWORD *)v20 - 36) = v22 ^ _xmm;

                  --v21;

                }

                while ( v21 );

                v18 += 2;

                --v19;

              }

              while ( v19 );

              v23 = v28;

              v1 = this;

              v32 = v28;

              block_enda = 18;

              do

              {

                v24 = &this->polyPhaseIn[1];

                v25 = v23;

                v26 = 4;

                do

                {

                  v24 += 8;

                  *(v24 - 9) = *(v25 - 18);

                  v25 += 144;

                  *(v24 - 8) = *(v25 - 144);

                  *(v24 - 7) = *(v25 - 126);

                  *(v24 - 6) = *(v25 - 108);

                  *(v24 - 5) = *(v25 - 90);

                  *(v24 - 4) = *(v25 - 72);

                  *(v24 - 3) = *(v25 - 54);

                  *(v24 - 2) = *(v25 - 36);

                  --v26;

                }

                while ( v26 );

                v34 += SubbandSynthesis(this->polyPhaseIn, channelb, sample);

                v23 = v32 + 1;

                sample += 32;

                v27 = block_enda-- == 1;

                ++v32;

              }

              while ( !v27 );

              v36 += 40;

              v13 = channelb + 1;

              v12 = this->Frame.stereo;

              v11 = gr;

              channelb = v13;

            }

            while ( v13 < v12 );

          }

          v1->Callback->DataCallback(

            v1->pcmsample[0][0],

            v1->pcmsample[1][0],

            576,

            v12,

            v1->Frame.real_freq);

          p_block_type = (unsigned int *)(v35 + 18);

          gr = v11 + 1;

          v35 += 18;

        }

        while ( v11 + 1 < v1->Frame.version + 1 );

        if ( v34 )

          Logger.g->Log(2, "%d samples clipped.\n", v34);

      }

      return 1;

    }

    else

    {

      printf("Not enough main data to decode frame, skipping.\n");

      return 1;

    }

  }

  else

  {

    printf("input stream corrupted\n");

    return 0;

  }

}



//----- (004703D0) --------------------------------------------------------

void SMpegAudioDecoder::L3_DequantizeSample(int gr, int ch)

{

  SMpegAudioDecoder *v3; // esi

  char *v4; // ecx

  float v5;

  unsigned int v6; // edx

  int v7; // edi

  int v8; // ecx

  int v9; // ebx

  float v10; // xmm0_4

  float v11; // xmm1_4

  int *v12; // esi

  int v13; // eax

  _iobuf *v14; // eax

  FILE *v15; // eax

  long long v16; // rax

  int v17; // ecx

  int v18; // ebx

  char *v19; // esi

  float v20; // xmm0_4

  SMpegAudioDecoder *v21; // esi

  int v22; // ecx

  char *v23; // ebx

  int v24; // edx

  int v25; // ebx

  float v26;

  int v27; // eax

  float v28; // xmm0_4

  float *v29; // ecx

  int *v30; // edi

  float v31; // xmm1_4

  int v32; // eax

  _iobuf *v33; // eax

  FILE *v34; // eax

  float v35; // xmm0_4

  int v36; // eax

  int v37; // [esp-4h] [ebp-4Ch]

  int v38; // [esp-4h] [ebp-4Ch]

  int v39; // [esp+Ch] [ebp-3Ch]

  char *v40; // [esp+14h] [ebp-34h]

  float v42; // [esp+20h] [ebp-28h]

  float v43; // [esp+20h] [ebp-28h]

  int l; // [esp+24h] [ebp-24h]

  int v45; // [esp+24h] [ebp-24h]

  int v46; // [esp+28h] [ebp-20h]

  char *v47; // [esp+28h] [ebp-20h]

  int *v48; // [esp+2Ch] [ebp-1Ch]

  char *v49; // [esp+2Ch] [ebp-1Ch]

  int v50; // [esp+30h] [ebp-18h]

  int v51; // [esp+34h] [ebp-14h]

  int v52; // [esp+34h] [ebp-14h]

  float subblock_gain[3]; // [esp+38h] [ebp-10h]



  v3 = this;

  v4 = (char *)this + 160 * ch;

  v40 = v4;

  if ( *(_DWORD *)&v4[72 * gr + 112] && *(_DWORD *)&v4[72 * gr + 116] == 2 )

    v51 = *(_DWORD *)&v4[72 * gr + 120] != 0 ? 8 : 0;

  else

    v51 = 22;

  // IDA lost pow arguments (_libm_sse2_pow_precise). Reconstructed from MPEG III spec.
  // global_gain is at SGrInfo offset 8, which is v4[72*gr + 104]
  {
    SGrInfo *_gr = &this->si.ch[ch].gr[gr];
    double _scalefac_mult = 0.5 * (1.0 + _gr->scalefac_scale);
    v5 = (float)(pow(2.0f, 0.25f * ((double)(int)_gr->global_gain - 210.0f)));

  }

  v6 = v51;

  v7 = 0;

  *(float *)&v5 = v5;

  v42 = *(float *)&v5;

  if ( v51 )

  {

    v8 = 0;

    v46 = 0;

    l = (int)v3->scalefac[ch].l;

    do

    {

      v9 = v7;

      v7 = dword_58C55C[37 * v3->sfreq + v8];

      v3 = this;

      {
        SGrInfo *_gr = &this->si.ch[ch].gr[gr];
        double _scalefac_mult = 0.5 * (1.0 + _gr->scalefac_scale);
        v10 = (float)pow(2.0, -_scalefac_mult * (double)*(int *)l) * v42;
      }

      if ( v9 < v7 )

      {

        v11 = v10;

        v12 = &this->is[v9];

        v48 = v12;

        do

        {

          v13 = *v12;

          if ( *v12 <= -8704 || v13 >= 8704 )

          {

            v37 = *v12;

            v14 = stderr;

            fprintf(v14, "\n!!! Maximum quanted value %ld is too large\n", v37);

            v15 = stderr;

            fflush(v15);

            v11 = v10;

            v16 = *v12;

            v17 = HIDWORD(v16) ^ *v12;

            v12 = v48;

            v13 = 8704 * ((int)v16 / (v17 - HIDWORD(v16)));

            *v48 = v13;

          }

          v48 = ++v12;

          this->dq[ch][v9++] = qword_5A6F30[v13] * v11;

        }

        while ( v9 < v7 );

        v3 = this;

      }

      v8 = v46 + 1;

      v46 = v8;

      l += 4;

    }

    while ( v8 < v51 );

    v6 = v51;

  }

  if ( v6 < 0x16 )

  {

    v18 = 0;

    v19 = &v40[72 * gr + 136];

    do

    {
      // subblock_gain[win] at v19, v19 advances by 4 bytes (1 int) per iteration
      // IDA had v19 += 4 before pow, but pow arg was loaded before increment in original asm
      v20 = (float)pow(2.0, -2.0 * (double)*(_DWORD *)v19) * v42;

      v19 += 4;

      subblock_gain[v18++] = v20;

    }

    while ( v18 < 3 );

    v21 = this;

    v22 = v51 != 0 ? 3 : 0;

    v52 = v22;

    v23 = (char *)this->scalefac[ch].s + 4 * v22;

    v49 = v23;

    do

    {

      v50 = 0;

      v47 = v23;

      v24 = dword_58C5B8[37 * v21->sfreq + v22] - dword_58C5B4[37 * v21->sfreq + v22];

      v39 = v24;

      do

      {

        v25 = v7;

        v7 += v24;

        v45 = v7;

        {
          SGrInfo *_gr = &this->si.ch[ch].gr[gr];
          double _scalefac_mult = 0.5 * (1.0 + _gr->scalefac_scale);
          v26 = (float)(pow(2.0f, -_scalefac_mult * (double)*(int *)v47));

        }

        v27 = v50;

        v28 = v26 * subblock_gain[v50];

        v43 = v28;

        if ( v25 < v7 )

        {

          v29 = this->dq[ch];

          v30 = &v21->is[v25];

          v31 = v28;

          do

          {

            v32 = *v30;

            if ( *v30 <= -8704 || v32 >= 8704 )

            {

              v38 = *v30;

              v33 = stderr;

              fprintf(v33, "\n!!! Maximum quanted value %ld is too large\n", v38);

              v34 = stderr;

              fflush(v34);

              v31 = v43;

              v21 = this;

              v29 = this->dq[ch];

              v32 = 8704 * (*v30 / (int)abs(*v30));

              *v30 = v32;

            }

            ++v30;

            v35 = qword_5A6F30[v32] * v31;

            v36 = v25 + 576 * v21->sfreq;

            ++v25;

            v29[reorder_table[v36]] = v35;

          }

          while ( v25 < v45 );

          v7 = v45;

          v27 = v50;

        }

        v24 = v39;

        v50 = v27 + 1;

        v47 += 52;

      }

      while ( v27 + 1 < 3 );

      v22 = v52 + 1;

      v23 = v49 + 4;

      v52 = v22;

      v49 += 4;

    }

    while ( v22 < 13 );

  }

}



//----- (00470860) --------------------------------------------------------

void SMpegAudioDecoder::L3_GetMPEG1ScaleFactors(int gr, int ch)

{

  int v4; // edi

  int *v5; // ecx

  int v6; // edi

  SMpegAudioDecoder::ScaleFac *v7; // esi

  unsigned int v8; // eax

  int *v9; // esi

  char *v10; // eax

  char *v11; // edi

  unsigned int v12; // eax

  bool v13; // zf

  char *v14; // eax

  char *v15; // edi

  unsigned int v16; // eax

  SMpegAudioDecoder *v17; // edx

  SMpegAudioDecoder *v18; // ecx

  int v19; // eax

  SMpegAudioDecoder *v20; // esi

  int v21; // eax

  int *v22; // esi

  int v23; // edx

  char *v24; // eax

  char *v25; // esi

  int v26; // edi

  unsigned int v27; // eax

  int v28; // esi

  SMpegAudioDecoder::SideInfoCh *v29; // eax

  int v30; // edi

  int v31; // edx

  unsigned int v32; // eax

  int *v33; // [esp+Ch] [ebp-10h]

  unsigned int *v34; // [esp+Ch] [ebp-10h]

  int v35; // [esp+10h] [ebp-Ch]

  int v36; // [esp+10h] [ebp-Ch]

  int v37; // [esp+10h] [ebp-Ch]

  SMpegAudioDecoder::SideInfoCh *v38; // [esp+10h] [ebp-Ch]

  char *v39; // [esp+14h] [ebp-8h]

  char *v40; // [esp+14h] [ebp-8h]

  char *v41; // [esp+14h] [ebp-8h]

  int v42; // [esp+14h] [ebp-8h]

  int *v43; // [esp+18h] [ebp-4h]

  int v44; // [esp+18h] [ebp-4h]

  int v45; // [esp+18h] [ebp-4h]

  int gra; // [esp+24h] [ebp+8h]

  int grb; // [esp+24h] [ebp+8h]



  v4 = gr;

  v5 = &this->Frame.version + 40 * ch + 18 * gr;

  v43 = v5;

  if ( v5[28] && v5[29] == 2 )

  {

    if ( v5[30] )

    {

      v6 = 8;

      gra = 244 * ch;

      v7 = &this->scalefac[ch];

      do

      {

        v8 = s2_getbits(slen[0][v5[27]]);

        v5 = v43;

        v7 = (SMpegAudioDecoder::ScaleFac *)((char *)v7 + 4);

        v7[-1].s[2][12] = v8;

        --v6;

      }

      while ( v6 );

      v9 = v43;

      v35 = 3;

      v10 = (char *)&this->scalefac[0].s[0][3] + gra;

      v39 = v10;

      do

      {

        v11 = v10;

        v44 = 3;

        do

        {

          v12 = s2_getbits(slen[0][v9[27]]);

          v13 = v44-- == 1;

          v11 += 52;

          *((_DWORD *)v11 - 13) = v12;

        }

        while ( !v13 );

        v10 = v39 + 4;

        v13 = v35-- == 1;

        v39 += 4;

      }

      while ( !v13 );

      v45 = 6;

      v14 = (char *)&this->scalefac[0].s[0][6] + gra;

      v40 = v14;

      do

      {

        v15 = v14;

        v36 = 3;

        do

        {

          v16 = s2_getbits(dword_58C518[v9[27]]);

          v13 = v36-- == 1;

          v15 += 52;

          *((_DWORD *)v15 - 13) = v16;

        }

        while ( !v13 );

        v14 = v40 + 4;

        v13 = v45-- == 1;

        v40 += 4;

      }

      while ( !v13 );

      v17 = (SMpegAudioDecoder *)(244 * ch);

      v18 = this;

      v19 = ch;

      v20 = this;

    }

    else

    {

      v21 = 0;

      v22 = &sfb_table.s[1];  // IDA had (int*)sfb_table.s[1] — value-as-pointer bug

      grb = 0;

      v33 = &sfb_table.s[1];

      do

      {

        v23 = *(v22 - 1);

        v37 = v23;

        if ( v23 < *v22 )

        {

          v24 = (char *)this->scalefac[ch].s + 4 * v23;

          v41 = v24;

          do

          {

            v25 = v24;

            v26 = 3;

            do

            {

              v27 = s2_getbits(slen[0][grb + v5[27]]);

              v5 = v43;

              v25 += 52;

              *((_DWORD *)v25 - 13) = v27;

              --v26;

            }

            while ( v26 );

            v22 = v33;

            v24 = v41 + 4;

            ++v37;

            v41 += 4;

          }

          while ( v37 < *v33 );

          v21 = grb;

        }

        ++v22;

        v21 += 16;

        grb = v21;

        v33 = v22;

      }

      while ( v22 < sfb_table.s + 3 );  // IDA had (int)v22 < (int)slen[0] — relied on contiguous layout

      v19 = ch;

      v17 = this;

      v18 = (SMpegAudioDecoder *)(244 * ch);

      gra = (int)this;

      v20 = (SMpegAudioDecoder *)(244 * ch);

    }

    this->scalefac[v19].s[0][12] = 0;

    *(int *)((char *)&v17->scalefac[0].s[1][12] + (_DWORD)v20) = 0;

    *(int *)((char *)&v18->scalefac[0].s[2][12] + gra) = 0;

  }

  else

  {

    v28 = 0;

    v29 = &this->si.ch[ch];

    v38 = v29;

    do

    {

      if ( !v29->scfsi[0] || !v4 )

      {

        v30 = sfb_table.l[v28];

        if ( v30 < ((int *)sfb_table.s)[v28 - 4] )

        {

          v31 = 0;

          if ( v28 >= 2 )

            v31 = 16;

          v42 = v31;

          v34 = (unsigned int *)&this->scalefac[ch].l[v30];

          do

          {

            v32 = s2_getbits(slen[0][v31 + v5[27]]);

            ++v30;

            v31 = v42;

            *v34++ = v32;

            v5 = v43;

          }

          while ( v30 < ((int *)sfb_table.s)[v28 - 4] );

          v29 = v38;

        }

        v4 = gr;

      }

      ++v28;

      v29 = (SMpegAudioDecoder::SideInfoCh *)((char *)v29 + 4);

      v38 = v29;

    }

    while ( v28 < 4 );

    this->scalefac[ch].l[21] = 0;

  }

}



//----- (00470B10) --------------------------------------------------------

void SMpegAudioDecoder::L3_GetMPEG2ScaleFactors(int gr, int ch)

{

  int *v4; // edi

  int v5; // ecx

  int v6; // eax

  int mode_ext; // eax

  int v8; // ecx

  int v9; // edx

  int v10; // esi

  int v11; // eax

  int v12; // eax

  signed int v13; // esi

  int v14; // eax

  int v15; // edx

  int v16; // et2

  unsigned int *v17; // edi

  int v18; // eax

  int *v19; // ecx

  int v20; // esi

  int v21; // ebx

  unsigned int v22; // eax

  int *v23; // esi

  unsigned int *v24; // ecx

  int *v25; // edx

  int v26; // edi

  int v27; // eax

  unsigned int *v28; // ecx

  int v29; // edi

  int *v30; // edx

  int v31; // eax

  char *v32; // ecx

  unsigned int v33; // eax

  int *v34; // [esp+Ch] [ebp-100h]

  int v35; // [esp+10h] [ebp-FCh]

  int v37; // [esp+1Ch] [ebp-F0h]

  int *v38; // [esp+1Ch] [ebp-F0h]

  unsigned int scalefac_buffer[54]; // [esp+20h] [ebp-ECh] BYREF

  unsigned int new_slen[4]; // [esp+F8h] [ebp-14h]



  v4 = &this->Frame.version + 40 * ch + 18 * gr;

  v34 = v4;

  if ( v4[29] == 2 )

  {

    v5 = v4[30];

    if ( v5 )

    {

      v6 = 0;

      if ( v5 == 1 )

        v6 = 2;

      v37 = v6;

    }

    else

    {

      v37 = 1;

    }

  }

  else

  {

    v37 = 0;

  }

  mode_ext = this->Frame.mode_ext;

  if ( mode_ext != 1 && mode_ext != 3 || ch != 1 )

  {

    v8 = v4[27];

    if ( v8 < 400 )

    {

      v4[39] = 0;

      v9 = (v8 >> 4) % 5;

      new_slen[0] = (v8 >> 4) / 5;

      new_slen[3] = v8 & 3;

      new_slen[2] = (v8 >> 2) & 3;

      v10 = 0;

LABEL_23:

      new_slen[1] = v9;

      goto LABEL_24;

    }

    new_slen[3] = 0;

    if ( v8 < 500 )

    {

      v4[39] = 0;

      v11 = ((v8 - 400) >> 2) / 5;

      v9 = ((v8 - 400) >> 2) % 5;

      v10 = 1;

      new_slen[2] = v8 & 3;

LABEL_22:

      new_slen[0] = v11;

      goto LABEL_23;

    }

    v12 = v8 - 500;

    v4[39] = 1;

    v10 = 2;

LABEL_21:

    new_slen[2] = 0;

    v16 = v12 % 3;

    v11 = v12 / 3;

    v9 = v16;

    goto LABEL_22;

  }

  v13 = (unsigned int)v4[27] >> 1;

  v4[39] = 0;

  new_slen[3] = 0;

  if ( (unsigned int)v13 >= 0xB4 )

  {

    if ( (unsigned int)v13 >= 0xF4 )

    {

      v12 = v13 - 244;

      v10 = 5;

      goto LABEL_21;

    }

    new_slen[1] = ((v13 - 180) >> 2) & 3;

    new_slen[0] = ((v13 - 180) >> 4) & 3;

    new_slen[2] = v13 & 3;

    v10 = 4;

  }

  else

  {

    new_slen[0] = v13 / 36;

    v14 = v13 % 36 / 6;

    v15 = v13 % 36 % 6;

    v10 = 3;

    new_slen[1] = v14;

    new_slen[2] = v15;

  }

LABEL_24:

  memset(scalefac_buffer, 0, sizeof(scalefac_buffer));

  v17 = scalefac_buffer;

  v18 = 0;

  v35 = 0;

  v19 = &nr_of_sfb_block[0][2 * v10][4 * v10 + 4 * v37];

  v38 = v19;

  do

  {

    v20 = 0;

    if ( *v19 > 0 )

    {

      v21 = new_slen[v18];

      do

      {

        if ( v21 )

        {

          v22 = s2_getbits(v21);

          v19 = v38;

        }

        else

        {

          v22 = 0;

        }

        *v17 = v22;

        ++v20;

        ++v17;

      }

      while ( v20 < *v19 );

      v18 = v35;

    }

    ++v18;

    ++v19;

    v35 = v18;

    v38 = v19;

  }

  while ( v18 < 4 );

  if ( v34[28] && v34[29] == 2 )

  {

    v23 = &this->Frame.version + 61 * ch;

    if ( v34[30] )

    {

      v24 = &scalefac_buffer[10];

      v23[100] = scalefac_buffer[0];

      v25 = v23 + 138;

      v26 = 9;

      v23[101] = scalefac_buffer[1];

      v23[102] = scalefac_buffer[2];

      v23[103] = scalefac_buffer[3];

      v23[104] = scalefac_buffer[4];

      v23[105] = scalefac_buffer[5];

      v23[106] = scalefac_buffer[6];

      v23[107] = scalefac_buffer[7];

      do

      {

        v27 = *(v24 - 2);

        v24 += 3;

        *(v25 - 13) = v27;

        *v25++ = *(v24 - 4);

        v25[12] = *(v24 - 3);

        --v26;

      }

      while ( v26 );

    }

    else

    {

      v28 = &scalefac_buffer[2];

      v29 = 12;

      v30 = v23 + 135;

      do

      {

        v31 = *(v28 - 2);

        v28 += 3;

        *(v30 - 13) = v31;

        *v30++ = *(v28 - 4);

        v30[12] = *(v28 - 3);

        --v29;

      }

      while ( v29 );

    }

    this->scalefac[ch].s[0][12] = 0;

    v23[147] = 0;

    v23[160] = 0;

  }

  else

  {

    v32 = (char *)this + 244 * ch;

    v33 = scalefac_buffer[20];

    *((_OWORD *)v32 + 25) = *(_OWORD *)scalefac_buffer;

    *((_OWORD *)v32 + 26) = *(_OWORD *)&scalefac_buffer[4];

    *((_OWORD *)v32 + 27) = *(_OWORD *)&scalefac_buffer[8];

    *((_OWORD *)v32 + 28) = *(_OWORD *)&scalefac_buffer[12];

    *((_OWORD *)v32 + 29) = *(_OWORD *)&scalefac_buffer[16];

    *((_DWORD *)v32 + 120) = v33;

    *((_DWORD *)v32 + 121) = 0;

  }

}



//----- (00470F10) --------------------------------------------------------

void SMpegAudioDecoder::L3_GetSideInfo()

{

  SMpegAudioDecoder::SideInfoCh *ch; // eax

  SMpegAudioDecoder::SideInfoCh *v3; // edi

  int v4; // ebx

  int v5; // eax

  unsigned int *p_big_values; // ecx

  unsigned int *v7; // ebx

  unsigned int v8; // eax

  unsigned int *v9; // edi

  int v10; // ebx

  int v11; // ebx

  unsigned int *v12; // edi

  unsigned int *v13; // ebx

  unsigned int v14; // eax

  int v15; // ecx

  unsigned int v16; // eax

  int v17; // ebx

  unsigned int *v18; // ebx

  unsigned int v19; // eax

  unsigned int *v20; // edi

  int v21; // ebx

  int v22; // ebx

  unsigned int *v23; // edi

  unsigned int *v24; // ebx

  unsigned int v25; // eax

  int v26; // ecx

  unsigned int v27; // eax

  int v28; // ebx

  unsigned int *v29; // [esp+Ch] [ebp-10h]

  int v30; // [esp+Ch] [ebp-10h]

  int v31; // [esp+10h] [ebp-Ch]

  SMpegAudioDecoder::SideInfoCh *v32; // [esp+14h] [ebp-8h]

  int v33; // [esp+14h] [ebp-8h]

  int v34; // [esp+18h] [ebp-4h]

  unsigned int *v35; // [esp+18h] [ebp-4h]

  unsigned int *v36; // [esp+18h] [ebp-4h]



  if ( this->Frame.version )

  {

    this->si.main_data_begin = in_getbits(9);

    this->si.private_bits = in_getbits(2 * (this->Frame.stereo == 1) + 3);

    v34 = 0;

    if ( this->Frame.stereo > 0 )

    {

      ch = this->si.ch;

      v32 = this->si.ch;

      do

      {

        v3 = ch;

        v4 = 4;

        do

        {

          v3->scfsi[0] = in_getbits(1);

          v3 = (SMpegAudioDecoder::SideInfoCh *)((char *)v3 + 4);

          --v4;

        }

        while ( v4 );

        ch = v32 + 1;

        ++v34;

        ++v32;

      }

      while ( v34 < this->Frame.stereo );

    }

    v5 = 0;

    p_big_values = &this->si.ch[0].gr[0].big_values;

    v31 = 0;

    v29 = &this->si.ch[0].gr[0].big_values;

    do

    {

      v33 = 0;

      if ( this->Frame.stereo > 0 )

      {

        v7 = p_big_values;

        v35 = p_big_values;

        do

        {

          *(v7 - 1) = in_getbits(12);

          *v7 = in_getbits(9);

          v7[1] = in_getbits(8);

          v7[2] = in_getbits(4);

          v8 = in_getbits(1);

          v7[3] = v8;

          v9 = v7 + 6;

          if ( v8 )

          {

            v7[4] = in_getbits(2);

            v7[5] = in_getbits(1);

            v10 = 2;

            do

            {

              *v9++ = in_getbits(5);

              --v10;

            }

            while ( v10 );

            v11 = 3;

            v12 = v35 + 9;

            do

            {

              *v12++ = in_getbits(3);

              --v11;

            }

            while ( v11 );

            v13 = v35;

            v14 = v35[4];

            if ( !v14 )

              goto LABEL_44;

            if ( v14 != 2 || (v15 = 8, v35[5]) )

              v15 = 7;

            v35[12] = v15;

            v16 = 20 - v15;

          }

          else

          {

            v17 = 3;

            do

            {

              *v9++ = in_getbits(5);

              --v17;

            }

            while ( v17 );

            v13 = v35;

            v35[12] = in_getbits(4);

            v16 = in_getbits(3);

            v35[4] = 0;

          }

          v13[13] = v16;

          v13[14] = in_getbits(1);

          v13[15] = in_getbits(1);

          v13[16] = in_getbits(1);

          v7 = v13 + 40;

          v35 = v7;

          ++v33;

        }

        while ( v33 < this->Frame.stereo );

        v5 = v31;

        p_big_values = v29;

      }

      ++v5;

      p_big_values += 18;

      v31 = v5;

      v29 = p_big_values;

    }

    while ( v5 < 2 );

  }

  else

  {

    this->si.main_data_begin = in_getbits(8);

    this->si.private_bits = in_getbits((this->Frame.stereo != 1) + 1);

    v30 = 0;

    if ( this->Frame.stereo > 0 )

    {

      v18 = &this->si.ch[0].gr[0].big_values;

      v36 = &this->si.ch[0].gr[0].big_values;

      do

      {

        *(v18 - 1) = in_getbits(12);

        *v18 = in_getbits(9);

        v18[1] = in_getbits(8);

        v18[2] = in_getbits(9);

        v19 = in_getbits(1);

        v18[3] = v19;

        v20 = v18 + 6;

        if ( v19 )

        {

          v18[4] = in_getbits(2);

          v18[5] = in_getbits(1);

          v21 = 2;

          do

          {

            *v20++ = in_getbits(5);

            --v21;

          }

          while ( v21 );

          v22 = 3;

          v23 = v36 + 9;

          do

          {

            *v23++ = in_getbits(3);

            --v22;

          }

          while ( v22 );

          v24 = v36;

          v25 = v36[4];

          if ( !v25 )

LABEL_44:

            Logger.g->Panic(
              "SMpegAudioDecoder::L3_GetSideInfo: Side info bad: block_type == 0 in split block.\n");

          if ( v25 != 2 || (v26 = 8, v36[5]) )

            v26 = 7;

          v36[12] = v26;

          v27 = 20 - v26;

        }

        else

        {

          v28 = 3;

          do

          {

            *v20++ = in_getbits(5);

            --v28;

          }

          while ( v28 );

          v24 = v36;

          v36[12] = in_getbits(4);

          v27 = in_getbits(3);

          v36[4] = 0;

        }

        v24[13] = v27;

        v24[15] = in_getbits(1);

        v24[16] = in_getbits(1);

        v18 = v24 + 40;

        v36 = v18;

        ++v30;

      }

      while ( v30 < this->Frame.stereo );

    }

  }

}



//----- (004714E0) --------------------------------------------------------

void SMpegAudioDecoder::L3_Stereo(int gr)

{

  SMpegAudioDecoder *v2; // ebx

  int mode; // eax

  int v4; // esi

  int v5; // ecx

  bool v6; // zf

  int version; // eax

  float *v8; // ecx

  int v9; // edx

  int v10; // eax

  float v11; // xmm3_4

  int sfreq; // eax

  float *v13; // ecx

  int v14; // edi

  float v15; // xmm1_4

  int v16; // edx

  int v17; // esi

  int *v18; // eax

  float v19; // xmm2_4

  int v20; // ebx

  float v21; // xmm0_4

  int v22; // eax

  int *v23; // edx

  int *v24; // edi

  int v25; // ecx

  float v26;

  int v27; // ecx

  int v28; // eax

  SMpegAudioDecoder *v29; // esi

  int v30; // edx

  float *v31; // ecx

  int *v32; // ebx

  float v33; // xmm0_4

  int *v34; // edx

  int *l; // eax

  int v36; // ecx

  int v37; // eax

  int v38; // ecx

  float v39;

  int v40; // esi

  int v41; // ebx

  int *v42; // ebx

  int v43; // edx

  int v44; // esi

  int *v45; // eax

  int v46; // ebx

  float v47; // xmm0_4

  int v48; // eax

  int *v49; // edx

  int *v50; // edi

  int v51; // ecx

  float v52;

  int v53; // ecx

  int v54; // eax

  int v55; // edx

  float *v56; // ecx

  float v57; // xmm1_4

  int v58; // esi

  int v59; // ecx

  int *v60; // eax

  int v61; // edx

  float v62; // xmm0_4

  SMpegAudioDecoder *v63; // edi

  int v64; // eax

  int *v65; // ebx

  int *v66; // esi

  int *v67; // eax

  int v68; // ecx

  float v69;

  int v70; // esi

  int v71; // ebx

  unsigned int v72; // eax

  int v73; // ecx

  float v74; // xmm2_4

  float v75; // xmm3_4

  float v76; // xmm1_4

  float v77; // xmm1_4

  float v78; // xmm0_4

  float v79; // xmm1_4

  float *v80; // eax

  int v81; // ecx

  float v82; // xmm1_4

  float v83; // xmm0_4

  SMpegAudioDecoder *lr; // ecx

  int v85; // edx

  float v86; // eax

  _DWORD v87[1940]; // [esp+0h] [ebp-3984h]

  int v88; // [esp+1E50h] [ebp-1B34h]

  int v89; // [esp+1E54h] [ebp-1B30h]

  int v90; // [esp+1E58h] [ebp-1B2Ch]

  char *v91; // [esp+1E5Ch] [ebp-1B28h]

  int v92; // [esp+1E60h] [ebp-1B24h]

  SMpegAudioDecoder *v93; // [esp+1E64h] [ebp-1B20h]

  int *v94; // [esp+1E68h] [ebp-1B1Ch]

  int v95; // [esp+1E6Ch] [ebp-1B18h]

  int j; // [esp+1E70h] [ebp-1B14h]

  int i; // [esp+1E74h] [ebp-1B10h]

  int v98; // [esp+1E78h] [ebp-1B0Ch]

  float v99; // [esp+1E7Ch] [ebp-1B08h]

  _DWORD v100[1152]; // [esp+1E80h] [ebp-1B04h] BYREF

  _DWORD v101[576]; // [esp+3080h] [ebp-904h] BYREF



  v2 = this;

  v93 = this;

  mode = this->Frame.mode;

  if ( mode == 1 && (this->Frame.mode_ext & 2) != 0 )

  {

    v4 = this->Frame.mode;

    v88 = v4;

  }

  else

  {

    v4 = 0;

    v88 = 0;

    if ( mode != 1 )

      goto LABEL_7;

  }

  if ( (this->Frame.mode_ext & 1) != 0 )

  {

    v5 = 1;

    goto LABEL_8;

  }

LABEL_7:

  v5 = 0;

LABEL_8:

  v6 = v2->Frame.stereo == 1;

  version = v2->Frame.version;

  v90 = 9 * gr;

  v92 = version;

  if ( v6 )

  {

    v8 = v2->lr[0];

    v9 = 64;

    do

    {

      v10 = *((_DWORD *)v8 - 1152);

      v8 += 9;

      *((_DWORD *)v8 - 9) = v10;

      *(v8 - 8) = *(v8 - 1160);

      *(v8 - 7) = *(v8 - 1159);

      *(v8 - 6) = *(v8 - 1158);

      *(v8 - 5) = *(v8 - 1157);

      *(v8 - 4) = *(v8 - 1156);

      *(v8 - 3) = *(v8 - 1155);

      *(v8 - 2) = *(v8 - 1154);

      *(v8 - 1) = *(v8 - 1153);

      --v9;

    }

    while ( v9 );

    return;

  }

  if ( v5 )

  {

    v6 = v2->si.ch[0].gr[gr].window_switching_flag == 0;

    v11 = 1.0f;

    memset(v101, 7, 0x240u);

    if ( !v6 && v2->si.ch[0].gr[gr].block_type == 2 )

    {

      sfreq = v2->sfreq;

      v13 = &v2->dq[1][570];

      v14 = 148 * sfreq;

      v15 = 0.0;

      v6 = v2->si.ch[0].gr[gr].mixed_block_flag == 0;

      v98 = (int)&v2->dq[1][570];

      v91 = (char *)sfreq;

      v89 = 148 * sfreq;

      if ( v6 )

      {

        v42 = (int *)((char *)dword_58C5B4 + v14);

        v94 = 0;

        v89 = (int)dword_58C5B4 + v14;

        for ( i = 183; i < 222; i += 13 )

        {

          v43 = 191;

          while ( 1 )

          {

            if ( v13[3] != 0.0 )

              goto LABEL_101;

            if ( *v13 != 0.0 )

              break;

            if ( *(v13 - 3) != 0.0 )

            {

              v43 -= 2;

              goto LABEL_101;

            }

            if ( *(v13 - 6) != 0.0 )

            {

              v43 -= 3;

              goto LABEL_101;

            }

            if ( *(v13 - 9) != 0.0 )

            {

              v43 -= 4;

              goto LABEL_101;

            }

            if ( *(v13 - 12) != 0.0 )

            {

              v43 -= 5;

              goto LABEL_101;

            }

            v13 -= 18;

            v43 -= 6;

            if ( v43 < 0 )

              goto LABEL_101;

          }

          --v43;

LABEL_101:

          v44 = 0;

          if ( *v42 <= v43 )

          {

            v45 = v42;

            do

            {

              ++v45;

              ++v44;

            }

            while ( *v45 <= v43 );

          }

          v99 = 0.0;

          v46 = 7;

          v47 = 0.0;

          if ( v44 < 13 )

          {

            v48 = v44 + 37 * (_DWORD)v91;

            v49 = &dword_58C5B4[v48];

            v50 = &dword_58C5B8[v48];

            j = (int)v49;

            do

            {

              if ( v44 != 12 )

              {

                v46 = *(&v93->Frame.version + v44 + i);

                if ( v46 != 7 )

                {

                  if ( v92 )

                  {

                    v52 = (float)(tan(0.0f));


                    v15 = v99;

                    v11 = 1.0f;

                    v49 = (int *)j;

                    v47 = v52;

                  }

                  else if ( v46 )

                  {

                    v51 = 32 * (v93->si.ch[0].gr[v90 / 9u].scalefac_compress & 1);

                    if ( (v46 & 1) != 0 )

                    {

                      v15 = v11;

                      v99 = v11;

                      v47 = io[v51 + ((v46 + 1) >> 1)];

                    }

                    else

                    {

                      v47 = v11;

                      v15 = io[v51 + (v46 >> 1)];

                      v99 = v15;

                    }

                  }

                  else

                  {

                    v15 = v11;

                    v47 = v11;

                    v99 = v11;

                  }

                }

              }

              v53 = (int)v94 + 2 * *v49 + *v49;

              v54 = 3 * *v50;

              if ( v53 < v54 )

              {

                if ( v92 )

                {

                  for ( ; v53 < v54; v53 += 3 )

                  {

                    v101[v53] = v46;

                    *(float *)&v100[v53] = v47;

                  }

                }

                else

                {

                  do

                  {

                    v101[v53] = v46;

                    *(float *)&v100[v53] = v47;

                    *(float *)&v100[v53 + 576] = v15;

                    v53 += 3;

                  }

                  while ( v53 < v54 );

                }

              }

              ++v44;

              ++v49;

              ++v50;

              j = (int)v49;

            }

            while ( v44 < 13 );

          }

          v15 = 0.0;

          v94 = (int *)((char *)v94 + 1);

          v13 = (float *)(v98 + 4);

          v42 = (int *)v89;

          v98 += 4;

        }

      }

      else

      {

        v95 = 0;

        i = 0;

        for ( j = 183; j < 222; j += 13 )

        {

          v16 = 191;

          while ( 1 )

          {

            if ( v13[3] != 0.0 )

              goto LABEL_31;

            if ( *v13 != 0.0 )

            {

              --v16;

              goto LABEL_31;

            }

            if ( *(v13 - 3) != 0.0 )

            {

              v16 -= 2;

              goto LABEL_31;

            }

            if ( *(v13 - 6) != 0.0 )

            {

              v16 -= 3;

              goto LABEL_31;

            }

            if ( *(v13 - 9) != 0.0 )

            {

              v16 -= 4;

              goto LABEL_31;

            }

            if ( *(v13 - 12) != 0.0 )

              break;

            v16 -= 6;

            v13 -= 18;

            if ( v16 < 12 )

              goto LABEL_31;

          }

          v16 -= 5;

LABEL_31:

          v17 = 3;

          if ( *(int *)((char *)dword_58C5C0 + v14) > v16 )

            goto LABEL_35;

          v18 = (int *)((char *)dword_58C5C0 + v14);

          do

          {

            ++v18;

            ++v17;

          }

          while ( *v18 <= v16 );

          if ( v17 == 3 )

LABEL_35:

            v95 = 1;

          v19 = 0.0;

          v20 = 7;

          v99 = 0.0;

          v21 = 0.0;

          if ( v17 < 13 )

          {

            v22 = v17 + 37 * (_DWORD)v91;

            v23 = &dword_58C5B4[v22];

            v24 = &dword_58C5B8[v22];

            v94 = v23;

            do

            {

              if ( v17 != 12 )

              {

                v20 = *(&v93->Frame.version + v17 + j);

                if ( v20 != 7 )

                {

                  if ( v92 )

                  {

                    v26 = (float)(tan(0.0f));


                    v19 = v99;

                    v11 = 1.0f;

                    v23 = v94;

                    v21 = v26;

                  }

                  else if ( v20 )

                  {

                    v25 = 32 * (v93->si.ch[0].gr[v90 / 9u].scalefac_compress & 1);

                    if ( (v20 & 1) != 0 )

                    {

                      v19 = v11;

                      v99 = v11;

                      v21 = io[v25 + ((v20 + 1) >> 1)];

                    }

                    else

                    {

                      v21 = v11;

                      v19 = io[v25 + (v20 >> 1)];

                      v99 = v19;

                    }

                  }

                  else

                  {

                    v19 = v11;

                    v21 = v11;

                    v99 = v11;

                  }

                }

              }

              v27 = *v23 + i + 2 * *v23;

              v28 = 3 * *v24;

              if ( v27 < v28 )

              {

                if ( v92 )

                {

                  for ( ; v27 < v28; v27 += 3 )

                  {

                    v101[v27] = v20;

                    *(float *)&v100[v27] = v21;

                  }

                }

                else

                {

                  do

                  {

                    v101[v27] = v20;

                    *(float *)&v100[v27] = v21;

                    *(float *)&v100[v27 + 576] = v19;

                    v27 += 3;

                  }

                  while ( v27 < v28 );

                }

              }

              ++v17;

              ++v23;

              ++v24;

              v94 = v23;

            }

            while ( v17 < 13 );

            v14 = v89;

            v15 = 0.0;

          }

          ++i;

          v13 = (float *)(v98 + 4);

          v98 += 4;

        }

        if ( v95 )

        {

          v29 = v93;

          v30 = 35;

          v31 = &v93->dq[1][34];

          do

          {

            if ( v31[1] != 0.0 )

              break;

            if ( *v31 != 0.0 )

              break;

            if ( *(v31 - 1) != 0.0 )

              break;

            if ( *(v31 - 2) != 0.0 )

              break;

            if ( *(v31 - 3) != 0.0 )

              break;

            if ( *(v31 - 4) != 0.0 )

              break;

            v31 -= 6;

            v30 -= 6;

          }

          while ( v30 >= 0 );

          v32 = (int *)((char *)sfb_index[0].l + v14);

          v95 = 0;

          v33 = 0.0;

          v91 = (char *)sfb_index + v14;

          v34 = (int *)((char *)dword_58C55C + v14);

          v99 = 0.0;

          l = v93->scalefac[1].l;

          v94 = (int *)((char *)dword_58C55C + v14);

          v36 = 8;

          j = (int)v93->scalefac[1].l;

          v98 = 8;

          while ( 1 )

          {

            v37 = *l;

            i = v37;

            if ( v37 == 7 )

              goto LABEL_76;

            if ( v92 )

              break;

            if ( v37 )

            {

              v38 = 32 * (v29->si.ch[0].gr[v90 / 9u].scalefac_compress & 1);

              if ( (v37 & 1) != 0 )

              {

                v33 = v11;

                v99 = v11;

                v15 = io[v38 + ((v37 + 1) >> 1)];

              }

              else

              {

                v15 = v11;

                v33 = io[v38 + (v37 >> 1)];

                v99 = v33;

              }

              goto LABEL_74;

            }

            v33 = v11;

            v15 = v11;

            v99 = v11;

LABEL_75:

            v95 = LODWORD(v15);

LABEL_76:

            v40 = *v32;

            v41 = *v34;

            if ( v40 < *v34 )

            {

              if ( !v92 )

              {

                memset(&v101[v40], v37, v41 - v40);

                do

                {

                  *(float *)&v100[v40] = v15;

                  *(float *)&v100[v40++ + 576] = v33;

                }

                while ( v40 < v41 );

                goto LABEL_83;

              }

              if ( v40 < v41 )

              {

                memset(&v100[v40], v95, (unsigned int)(4 * (v41 - v40)) >> 2);

                v34 = v94;

                memset(&v101[v40], *(_DWORD *)j, (unsigned int)(4 * (v41 - v40)) >> 2);

LABEL_83:

                v36 = v98;

              }

            }

            ++v34;

            l = (int *)(j + 4);

            v11 = 1.0f;

            v32 = (int *)(v91 + 4);

            --v36;

            v29 = v93;

            j += 4;

            v91 += 4;

            v94 = v34;

            v98 = v36;

            if ( !v36 )

            {

              v2 = v93;

              goto LABEL_164;

            }

          }

          v39 = (float)(tan(0.0f));


          v34 = v94;

          v15 = v39;

          v33 = v99;

LABEL_74:

          v37 = i;

          v36 = v98;

          goto LABEL_75;

        }

      }

LABEL_163:

      v2 = v93;

      goto LABEL_164;

    }

    v55 = 575;

    v56 = &v2->dq[1][574];

    v57 = 0.0;

    while ( v56[1] == 0.0 )

    {

      if ( *v56 != 0.0 )

      {

        --v55;

        break;

      }

      if ( *(v56 - 1) != 0.0 )

      {

        v55 -= 2;

        break;

      }

      if ( *(v56 - 2) != 0.0 )

      {

        v55 -= 3;

        break;

      }

      if ( *(v56 - 3) != 0.0 )

      {

        v55 -= 4;

        break;

      }

      if ( *(v56 - 4) != 0.0 )

      {

        v55 -= 5;

        break;

      }

      v56 -= 6;

      v55 -= 6;

      if ( v55 < 0 )

        break;

    }

    v58 = v2->sfreq;

    v59 = 0;

    v98 = 0;

    v60 = (int *)&sfb_index[v58];

    if ( *v60 <= v55 )

    {

      do

      {

        v60 = (int *)((char *)v60 + 4);

        ++v59;

      }

      while ( *v60 <= v55 );

      v98 = v59;

    }

    v95 = 0;

    v61 = 7;

    i = 7;

    v62 = 0.0;

    *(float *)&j = 0.0;

    if ( v59 < 22 )

    {

      v63 = v93;

      v64 = v59 + 37 * v58;

      v65 = &dword_58C55C[v64];

      v66 = &sfb_index[0].l[v64];

      v94 = v65;

      v91 = (char *)v66;

      v67 = &v93->scalefac[1].l[v59];

      v99 = *(float *)&v67;

      while ( 1 )

      {

        if ( v59 != 21 )

        {

          v61 = *v67;

          i = v61;

          if ( v61 != 7 )

            break;

        }

LABEL_154:

        v70 = *v66;

        v71 = *v65;

        if ( v70 < v71 )

        {

          if ( !v92 )

          {

            memset(&v101[v70], v61, v71 - v70);

            do

            {

              *(float *)&v100[v70] = v57;

              *(float *)&v100[v70++ + 576] = v62;

            }

            while ( v70 < v71 );

            goto LABEL_161;

          }

          if ( v70 < v71 )

          {

            memset(&v100[v70], v95, (unsigned int)(4 * (v71 - v70)) >> 2);

            memset(&v101[v70], i, (unsigned int)(4 * (v71 - v70)) >> 2);

LABEL_161:

            v63 = v93;

            *(float *)&v67 = v99;

            v59 = v98;

          }

        }

        ++v59;

        ++v67;

        v61 = i;

        v66 = (int *)(v91 + 4);

        v65 = v94 + 1;

        v98 = v59;

        v99 = *(float *)&v67;

        v91 += 4;

        ++v94;

        if ( v59 >= 22 )

          goto LABEL_163;

      }

      if ( v92 )

      {

        v69 = (float)(tan(0.0f));


        v11 = 1.0f;

        v61 = i;

        v57 = v69;

        v62 = *(float *)&j;

      }

      else

      {

        if ( !v61 )

        {

          v62 = v11;

          v57 = v11;

          *(float *)&j = v11;

LABEL_153:

          v95 = LODWORD(v57);

          goto LABEL_154;

        }

        v68 = 32 * (v63->si.ch[0].gr[v90 / 9u].scalefac_compress & 1);

        if ( (v61 & 1) != 0 )

        {

          v62 = v11;

          *(float *)&j = v11;

          v57 = io[v68 + ((v61 + 1) >> 1)];

        }

        else

        {

          v57 = v11;

          v62 = io[v68 + (v61 >> 1)];

          *(float *)&j = v62;

        }

      }

      v59 = v98;

      *(float *)&v67 = v99;

      goto LABEL_153;

    }

LABEL_164:

    v72 = 7808;

    v73 = v88;

    do

    {

      v74 = *(float *)((char *)v2 + v72 - 4608);

      if ( v87[v72 / 4 + 1152] == 7 )

      {

        v75 = *(float *)((char *)v2 + v72 - 2304);

        if ( v73 )

        {

          v76 = (float)(*(float *)((char *)v2 + v72 - 4608) - v75) * 0.707106782373f;

          v74 = (float)(v74 + v75) * 0.707106782373f;

        }

        else

        {

          v76 = *(float *)((char *)v2 + v72 - 2304);

        }

        *(float *)((char *)&v2->Frame.version + v72) = v74;

      }

      else

      {

        v77 = *(float *)&v87[v72 / 4];

        if ( v92 )

        {

          *(float *)((char *)&v2->Frame.version + v72) = (float)(v77 / (float)(v77 + 1.0)) * v74;

          v79 = 1.0f / v77 + 1.0f;

        }

        else

        {

          v78 = v74 * v77;

          v79 = *(float *)&v87[v72 / 4 + 576];

          *(float *)((char *)&v2->Frame.version + v72) = v78;

        }

        v76 = v79 * v74;

      }

      *(float *)&v2->is[v72 / 4 + 354] = v76;

      v72 += 4;

    }

    while ( (int)v72 < 10112 );

  }

  else if ( v4 )

  {

    v80 = v2->dq[0];

    v81 = 576;

    do

    {

      v82 = *v80 - v80[576];

      v83 = (float)(v80[576] + *v80) * 0.707106782373f;

      v80[1152] = v83;

      v80[1728] = v82 * 0.707106782373f;

      ++v80;

      --v81;

    }

    while ( v81 );

  }

  else

  {

    lr = (SMpegAudioDecoder *)v2->lr;

    v85 = 64;

    do

    {

      v86 = lr[-1].hm_buf[0][0][0];

      lr = (SMpegAudioDecoder *)((char *)lr + 36);

      lr[-1].hm_buf[1][31][9] = v86;

      lr->is[345] = LODWORD(lr[-1].hm_buf[0][31][9]);

      lr[-1].hm_buf[1][31][10] = lr[-1].sb_buf[1][538];

      lr->is[346] = LODWORD(lr[-1].hm_buf[0][31][10]);

      lr[-1].hm_buf[1][31][11] = lr[-1].sb_buf[1][539];

      lr->is[347] = LODWORD(lr[-1].hm_buf[0][31][11]);

      lr[-1].hm_buf[1][31][12] = lr[-1].sb_buf[1][540];

      lr->is[348] = LODWORD(lr[-1].hm_buf[0][31][12]);

      lr[-1].hm_buf[1][31][13] = lr[-1].sb_buf[1][541];

      lr->is[349] = LODWORD(lr[-1].hm_buf[0][31][13]);

      lr[-1].hm_buf[1][31][14] = lr[-1].sb_buf[1][542];

      lr->is[350] = LODWORD(lr[-1].hm_buf[0][31][14]);

      lr[-1].hm_buf[1][31][15] = lr[-1].sb_buf[1][543];

      lr->is[351] = LODWORD(lr[-1].hm_buf[0][31][15]);

      LODWORD(lr[-1].hm_buf[1][31][16]) = lr[-1].sb_buf_ofs[0];

      lr->is[352] = LODWORD(lr[-1].hm_buf[0][31][16]);

      LODWORD(lr[-1].hm_buf[1][31][17]) = lr[-1].sb_buf_ofs[1];

      lr->is[353] = LODWORD(lr[-1].hm_buf[0][31][17]);

      --v85;

    }

    while ( v85 );

  }

}
