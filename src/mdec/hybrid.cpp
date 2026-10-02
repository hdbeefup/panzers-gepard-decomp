// mdec/hybrid.cpp
// Hybrid filter (IMDCT) and antialias for MP3 decoder
// Decompiled from: gameSplit/mp3_decoder.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <corecrt_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "decode.h"

// Classes: SMpegAudioDecoder
// Function count: 3

//----- (0046E1B0) --------------------------------------------------------

// local variable allocation has failed, the output may be wrong!

void SMpegAudioDecoder::L3_Hybrid(

        float *fs,

        float *ts,

        int sb,

        int ch,

        int bt)

{

  float *v8; // ecx

  float *v9; // eax

  float v10; // xmm6_4

  float v11; // xmm0_4

  float v12; // xmm2_4

  float v13; // xmm5_4

  float v14; // xmm6_4

  float v15; // xmm1_4

  float v16; // xmm7_4

  float v17; // xmm1_4

  float v18; // xmm0_4

  float v19; // xmm2_4

  float v20; // xmm0_4

  float v21; // xmm2_4

  float v22; // xmm3_4

  float v23; // xmm4_4

  float v24; // xmm2_4

  float v25; // xmm1_4

  float v26; // xmm7_4

  float v27; // xmm1_4

  float v28; // xmm4_4

  float v29; // xmm3_4

  float v30; // xmm0_4

  int v31; // edx

  float v32; // xmm0_4

  float v33; // eax

  float v34; // xmm0_4

  float v35; // xmm0_4

  float v36; // eax

  float v37; // xmm0_4

  float v38; // xmm0_4

  float v39; // eax

  float v40; // xmm0_4

  float v41; // eax

  float v42; // xmm0_4

  float v43; // eax

  float v44; // xmm0_4

  int i; // eax

  int v46; // ecx

  float *v47; // edx

  float v48; // eax

  float v49; // xmm1_4

  __m128 v50; // xmm7

  float v51; // xmm0_4

  float v52; // xmm1_4

  float v53; // xmm1_4

  float v54; // xmm2_4

  float v55; // xmm1_4

  float v56; // xmm0_4

  float v57; // xmm1_4

  float v58; // xmm2_4

  float v59; // xmm1_4

  float v60; // xmm0_4

  float v61; // xmm1_4

  float v62; // xmm2_4

  float v63; // xmm1_4

  float v64; // xmm0_4

  float v65; // xmm2_4

  float v66; // xmm1_4

  float v67; // xmm2_4

  __m128 v68; // xmm0

  float v69; // xmm1_4

  __m128 v70; // xmm2

  float v71; // xmm1_4

  __m128 v72; // xmm0

  __m128 v73; // xmm6

  float v74; // xmm1_4

  float v75; // xmm1_4

  __m128 v76; // xmm0

  __m128 v77; // xmm4

  __m128 v78; // xmm0

  __m128 v79; // xmm5

  __m128 v80; // xmm3

  __m128 v81; // xmm3

  __m128 v82; // xmm1

  __m128 v83; // xmm0

  __m128 v84; // xmm4

  int v85; // edx

  float v86; // [esp-74h] [ebp-10Ch]

  float v87; // [esp-70h] [ebp-108h]

  float v88; // [esp-6Ch] [ebp-104h]

  float v89; // [esp-68h] [ebp-100h]

  __m128 v90; // [esp-64h] [ebp-FCh]

  float v91; // [esp-40h] [ebp-D8h]

  float v92; // [esp-40h] [ebp-D8h]

  float v93; // [esp-3Ch] [ebp-D4h]

  float v94; // [esp-38h] [ebp-D0h]

  float v95; // [esp-34h] [ebp-CCh]

  float v96; // [esp-30h] [ebp-C8h]

  float v97; // [esp-30h] [ebp-C8h]

  float v98; // [esp-2Ch] [ebp-C4h]

  float v99; // [esp-2Ch] [ebp-C4h]

  float v100; // [esp-2Ch] [ebp-C4h]

  float v101; // [esp-28h] [ebp-C0h]

  float v102; // [esp-28h] [ebp-C0h]

  int v103; // [esp-24h] [ebp-BCh]

  float v104; // [esp-24h] [ebp-BCh]

  float v105; // [esp-20h] [ebp-B8h]

  unsigned int v106; // [esp-20h] [ebp-B8h]

  float v107; // [esp-20h] [ebp-B8h]

  float v108; // [esp-1Ch] [ebp-B4h]

  float v109; // [esp-1Ch] [ebp-B4h]

  float v110; // [esp-1Ch] [ebp-B4h]

  float v111; // [esp-18h] [ebp-B0h]

  float v112; // [esp-18h] [ebp-B0h]

  float v113; // [esp-18h] [ebp-B0h]

  float v114; // [esp-18h] [ebp-B0h]

  float v115; // [esp-14h] [ebp-ACh]

  float v116; // [esp-14h] [ebp-ACh]

  float v117; // [esp-10h] [ebp-A8h]

  float v118; // [esp-10h] [ebp-A8h]

  float v119; // [esp-10h] [ebp-A8h]

  // In the original binary, v120 through v132 were contiguous on the stack
  // (36 floats total). The short block IMDCT writes 3×12=36 floats through
  // a pointer starting at v120, spilling into fs_2, v122-v132, and v127.
  _DWORD v120[36]; // [esp-Ch] [ebp-A4h] BYREF — enlarged from [3] for short block

  float fs_2[19]; // [esp+0h] [ebp-98h] OVERLAPPED

  float v122; // [esp+4Ch] [ebp-4Ch]

  float v123; // [esp+50h] [ebp-48h]

  float v124; // [esp+54h] [ebp-44h]

  float v125; // [esp+58h] [ebp-40h]

  float v126; // [esp+5Ch] [ebp-3Ch]

  __m128 v127; // [esp+60h] [ebp-38h]

  float v128; // [esp+70h] [ebp-28h]

  float v129; // [esp+74h] [ebp-24h]

  float v130; // [esp+78h] [ebp-20h]

  float v131; // [esp+7Ch] [ebp-1Ch]

  float v132; // [esp+80h] [ebp-18h]

  int v133; // [esp+8Ch] [ebp-Ch]

  void *v134; // [esp+90h] [ebp-8h]

  // IDA stack frame reconstruction artifacts (removed)
  v133 = 0;

  v134 = 0;

  if ( bt == 2 )

  {

    v8 = (float *)v120;

    v9 = fs + 6;

    v103 = 3;

    do

    {

      if ( *((_DWORD *)v9 - 6)

        || *((_DWORD *)v9 - 3)

        || *(_DWORD *)v9

        || *((_DWORD *)v9 + 3)

        || *((_DWORD *)v9 + 6)

        || *((_DWORD *)v9 + 9) )

      {

        v10 = hm_s1m[0] * *(v9 - 6);

        v11 = (float)(v9[9] * hm_s1m[5]) + v10;

        v12 = (float)(v9[6] * hm_s1m[4]) + (float)(*(v9 - 3) * hm_s1m[1]);

        v13 = v9[3] * hm_s1m[3];

        v14 = (float)(v10 - (float)(v9[9] * hm_s1m[5])) * hm_s2m[0];

        v91 = *v9 * hm_s1m[2];

        v15 = (float)(v13 + v91) + v11;

        v105 = (float)(v11 - (float)(v13 + v91)) * hm_cos[3];

        v96 = v12 + v15;

        v16 = (float)(hm_cos[6] * v15) - v12;

        v17 = (float)((float)(*(v9 - 3) * hm_s1m[1]) - (float)(v9[6] * hm_s1m[4]))

            * hm_s2m[1];

        v18 = (float)(v91 - v13) * hm_s2m[2];

        v19 = v14 - v18;

        v20 = v18 + v14;

        v21 = v19 * hm_cos[3];

        v22 = (float)(hm_cos[6] * v20) - v17;

        v23 = (float)(v17 + v20) + v21;

        v24 = v21 + v22;

        v25 = v24 + v16;

        v26 = v16 + v22;

        *v8 = hm_window[2][0] * v25;

        *((_DWORD *)v8 + 5) = NEGATE_FLOAT_BITS(hm_window[2][5] * v25);

        *((_DWORD *)v8 + 6) = NEGATE_FLOAT_BITS(hm_window[2][6] * (float)(v105 + v24));

        v27 = v23 + v105;

        *((_DWORD *)v8 + 11) = NEGATE_FLOAT_BITS(hm_window[2][11] * (float)(v105 + v24));

        v8[1] = hm_window[2][1] * v26;

        *((_DWORD *)v8 + 4) = NEGATE_FLOAT_BITS(hm_window[2][4] * v26);

        v28 = v23 + v96;

        *((_DWORD *)v8 + 7) = NEGATE_FLOAT_BITS(hm_window[2][7] * v27);

        *((_DWORD *)v8 + 10) = NEGATE_FLOAT_BITS(hm_window[2][10] * v27);

        v29 = v22 + 0.0f;

        v8[2] = hm_window[2][2] * v29;

        *((_DWORD *)v8 + 3) = NEGATE_FLOAT_BITS(hm_window[2][3] * v29);

        *((_DWORD *)v8 + 8) = NEGATE_FLOAT_BITS(hm_window[2][8] * v28);

        *((_DWORD *)v8 + 9) = NEGATE_FLOAT_BITS(hm_window[2][9] * v28);

      }

      else

      {

        memset(v8, 0, 3 * sizeof(_OWORD));

      }

      ++v9;

      v8 += 12;

      --v103;

    }

    while ( v103 );

    // Copy short block IMDCT output from contiguous v120 buffer into named locals
    // (original binary had these as contiguous stack memory; we need explicit copy)
    memcpy(fs_2, (float *)v120 + 3, 19 * sizeof(float));
    v122 = *((float *)v120 + 22);
    v123 = *((float *)v120 + 23);
    v124 = *((float *)v120 + 24);
    v125 = *((float *)v120 + 25);
    v126 = *((float *)v120 + 26);
    memcpy(&v127, (float *)v120 + 27, sizeof(v127));
    v128 = *((float *)v120 + 31);
    v129 = *((float *)v120 + 32);
    v130 = *((float *)v120 + 33);
    v131 = *((float *)v120 + 34);
    v132 = *((float *)v120 + 35);

    v30 = *(float *)v120;

    v31 = 32 * ch + sb;

    *ts = this->hm_buf[ch][sb][0];

    ts[6] = v30 + this->hm_buf[ch][sb][6];

    ts[12] = (float)(fs_2[3] + this->hm_buf[ch][sb][12]) + fs_2[9];

    v32 = v124 + fs_2[15];

    v33 = this->hm_buf[ch][sb][1];

    this->hm_buf[0][v31][12] = 0.0;

    this->hm_buf[0][v31][0] = v32;

    LODWORD(this->hm_buf[0][v31][6]) = v127.m128_i32[3];

    v34 = *(float *)&v120[1];

    ts[1] = v33;

    ts[7] = v34 + this->hm_buf[ch][sb][7];

    ts[13] = (float)(this->hm_buf[ch][sb][13] + fs_2[4]) + fs_2[10];

    v35 = fs_2[16] + v125;

    v36 = this->hm_buf[ch][sb][2];

    this->hm_buf[0][v31][13] = 0.0;

    this->hm_buf[0][v31][1] = v35;

    this->hm_buf[0][v31][7] = v128;

    v37 = *(float *)&v120[2];

    ts[2] = v36;

    ts[8] = v37 + this->hm_buf[ch][sb][8];

    ts[14] = (float)(fs_2[5] + this->hm_buf[ch][sb][14]) + fs_2[11];

    v38 = fs_2[17] + v126;

    v39 = this->hm_buf[ch][sb][3];

    this->hm_buf[0][v31][14] = 0.0;

    this->hm_buf[0][v31][2] = v38;

    this->hm_buf[0][v31][8] = v129;

    ts[3] = v39;

    ts[9] = this->hm_buf[ch][sb][9] + fs_2[0];

    ts[15] = (float)(this->hm_buf[ch][sb][15] + fs_2[6]) + fs_2[12];

    v40 = fs_2[18] + v127.m128_f32[0];

    v41 = this->hm_buf[ch][sb][4];

    this->hm_buf[0][v31][15] = 0.0;

    this->hm_buf[0][v31][3] = v40;

    this->hm_buf[0][v31][9] = v130;

    ts[4] = v41;

    ts[10] = this->hm_buf[ch][sb][10] + fs_2[1];

    ts[16] = (float)(fs_2[7] + this->hm_buf[ch][sb][16]) + fs_2[13];

    v42 = v122 + v127.m128_f32[1];

    v43 = this->hm_buf[ch][sb][5];

    this->hm_buf[0][v31][16] = 0.0;

    this->hm_buf[0][v31][4] = v42;

    this->hm_buf[0][v31][10] = v131;

    ts[5] = v43;

    ts[11] = this->hm_buf[ch][sb][11] + fs_2[2];

    ts[17] = (float)(this->hm_buf[ch][sb][17] + fs_2[8]) + fs_2[14];

    v44 = v123 + v127.m128_f32[2];

    this->hm_buf[0][v31][17] = 0.0;

    this->hm_buf[0][v31][5] = v44;

    this->hm_buf[0][v31][11] = v132;

  }

  else

  {

    for ( i = 0; i < 18; ++i )

    {

      if ( LODWORD(fs[i]) )

      {

        v49 = *fs * hm_1m[0];

        v50 = _mm_load_ss(fs + 7);

        v50.m128_f32[0] = v50.m128_f32[0] * hm_1m[7];

        v51 = fs[16] * hm_1m[16];

        v117 = (float)(fs[17] * hm_1m[17]) + v49;

        v52 = (float)(v49 - (float)(fs[17] * hm_1m[17])) * hm_2m[0];

        fs_2[15] = v117;

        v127.m128_f32[0] = v52;

        v53 = fs[1] * hm_1m[1];

        v54 = v51 + v53;

        v55 = v53 - v51;

        v56 = fs[15] * hm_1m[15];

        v111 = v54;

        fs_2[16] = v54;

        v127.m128_f32[1] = v55 * hm_2m[1];

        v57 = fs[2] * hm_1m[2];

        v58 = v56 + v57;

        v59 = v57 - v56;

        v60 = fs[14] * hm_1m[14];

        v108 = v58;

        fs_2[17] = v58;

        v127.m128_f32[2] = v59 * hm_2m[2];

        v61 = fs[3] * hm_1m[3];

        v62 = v60 + v61;

        v63 = v61 - v60;

        v64 = fs[13] * hm_1m[13];

        v98 = v62;

        fs_2[18] = v62;

        v65 = fs[4] * hm_1m[4];

        v127.m128_f32[3] = v63 * hm_2m[3];

        v66 = v64 + v65;

        v67 = v65 - v64;

        v68 = _mm_load_ss(fs + 12);

        v68.m128_f32[0] = v68.m128_f32[0] * hm_1m[12];

        v115 = v66;

        v69 = fs[5] * hm_1m[5];

        v93 = v67 * hm_2m[4];

        v70 = v68;

        v70.m128_f32[0] = v68.m128_f32[0] + v69;

        v71 = v69 - v68.m128_f32[0];

        v72 = _mm_load_ss(fs + 11);

        v72.m128_f32[0] = v72.m128_f32[0] * hm_1m[11];

        v73 = v72;

        *(float *)&v106 = v71 * hm_2m[5];

        v74 = fs[6] * hm_1m[6];

        v73.m128_f32[0] = v72.m128_f32[0] + v74;

        v75 = v74 - v72.m128_f32[0];

        v76 = _mm_load_ss(fs + 10);

        v76.m128_f32[0] = v76.m128_f32[0] * hm_1m[10];

        v77 = v76;

        v77.m128_f32[0] = v76.m128_f32[0] + v50.m128_f32[0];

        v50.m128_f32[0] = (float)(v50.m128_f32[0] - v76.m128_f32[0]) * hm_2m[7];

        v78 = _mm_load_ss(fs + 9);

        v78.m128_f32[0] = v78.m128_f32[0] * hm_1m[9];

        v79 = _mm_load_ss(fs + 8);

        v79.m128_f32[0] = v79.m128_f32[0] * hm_1m[8];

        v80 = v78;

        v80.m128_f32[0] = v78.m128_f32[0] + v79.m128_f32[0];

        v79.m128_f32[0] = (float)(v79.m128_f32[0] - v78.m128_f32[0]) * hm_2m[8];

        v90 = _mm_add_ps(_mm_unpacklo_ps(_mm_unpacklo_ps(v80, v73), _mm_unpacklo_ps(v77, v70)), _mm_loadu_ps(&fs_2[15]));

        v118 = v117 - v80.m128_f32[0];

        v101 = v79.m128_f32[0] + v127.m128_f32[0];

        v112 = v111 - v77.m128_f32[0];

        v92 = v50.m128_f32[0] + v127.m128_f32[1];

        v109 = v108 - v73.m128_f32[0];

        v81 = _mm_unpacklo_ps(v79, _mm_set_ss(v75 * hm_2m[6]));

        v99 = v98 - v70.m128_f32[0];

        v94 = (float)(v75 * hm_2m[6]) + v127.m128_f32[2];

        v82 = UINT_TO_M128(v106);

        v107 = *(float *)&v106 + v127.m128_f32[3];

        v83 = _mm_sub_ps(v127, _mm_unpacklo_ps(v81, _mm_unpacklo_ps(v50, v82)));

        v70.m128_f32[0] = _mm_shuffle_ps(v90, v90, 85).m128_f32[0];

        v77.m128_f32[0] = _mm_shuffle_ps(v90, v90, 170).m128_f32[0];

        v81.m128_f32[0] = _mm_shuffle_ps(v90, v90, 255).m128_f32[0];

        v86 = (float)((float)((float)(v90.m128_f32[0] + v70.m128_f32[0]) + v77.m128_f32[0]) + v81.m128_f32[0]) + v115;

        v113 = v112 * hm_cos[3];

        v87 = (float)((float)((float)(v118 * hm_cos[1]) + v113) + (float)(v109 * hm_cos[5]))

            + (float)(v99 * hm_cos[7]);

        v97 = (float)((float)((float)((float)(v90.m128_f32[0] * hm_cos[2])

                                    + (float)(v70.m128_f32[0] * hm_cos[6]))

                            - (float)(v77.m128_f32[0] * hm_cos[8]))

                    - (float)(v81.m128_f32[0] * hm_cos[4]))

            - v115;

        v88 = (float)((float)(v118 - v109) - v99) * hm_cos[3];

        v95 = (float)((float)((float)((float)(v90.m128_f32[0] * hm_cos[4])

                                    - (float)(v70.m128_f32[0] * hm_cos[6]))

                            - (float)(v77.m128_f32[0] * hm_cos[2]))

                    + (float)(v81.m128_f32[0] * hm_cos[8]))

            + v115;

        v104 = (float)((float)((float)(v118 * hm_cos[5]) - v113) - (float)(v109 * hm_cos[7]))

             + (float)(v99 * hm_cos[1]);

        v89 = (float)((float)((float)((float)(v77.m128_f32[0] + v90.m128_f32[0]) + v81.m128_f32[0])

                            * hm_cos[6])

                    - v70.m128_f32[0])

            - v115;

        v119 = (float)((float)((float)(v118 * hm_cos[7]) - v113) + (float)(v109 * hm_cos[1]))

             - (float)(v99 * hm_cos[5]);

        v50.m128_f32[0] = (float)((float)(v90.m128_f32[0] * hm_cos[8])

                                - (float)(v70.m128_f32[0] * hm_cos[6]))

                        + (float)(v77.m128_f32[0] * hm_cos[4]);

        v84 = v83;

        v73.m128_f32[0] = _mm_shuffle_ps(v84, v84, 85).m128_f32[0] * hm_cos[3];

        v90.m128_f32[0] = (float)(v50.m128_f32[0] - (float)(v81.m128_f32[0] * hm_cos[2])) + v115;

        v79.m128_f32[0] = _mm_shuffle_ps(v84, v84, 170).m128_f32[0];

        v84.m128_f32[0] = _mm_shuffle_ps(v84, v84, 255).m128_f32[0];

        v114 = (float)((float)((float)((float)(v101 + v92) + v94) + v107) + v93)

             + (float)((float)((float)((float)(v83.m128_f32[0] * hm_cos[1]) + v73.m128_f32[0])

                             + (float)(v79.m128_f32[0] * hm_cos[5]))

                     + (float)(v84.m128_f32[0] * hm_cos[7]));

        v82.m128_f32[0] = (float)((float)((float)((float)(v101 * hm_cos[2])

                                                + (float)(v92 * hm_cos[6]))

                                        - (float)(v94 * hm_cos[8]))

                                - (float)(v107 * hm_cos[4]))

                        - v93;

        v81.m128_f32[0] = (float)((float)(v83.m128_f32[0] - v79.m128_f32[0]) - v84.m128_f32[0])

                        * hm_cos[3];

        v116 = (float)((float)((float)((float)(v83.m128_f32[0] * hm_cos[1]) + v73.m128_f32[0])

                             + (float)(v79.m128_f32[0] * hm_cos[5]))

                     + (float)(v84.m128_f32[0] * hm_cos[7]))

             + v82.m128_f32[0];

        v110 = v81.m128_f32[0] + v82.m128_f32[0];

        v82.m128_f32[0] = (float)((float)((float)((float)(v101 * hm_cos[4])

                                                - (float)(v92 * hm_cos[6]))

                                        - (float)(v94 * hm_cos[2]))

                                + (float)(v107 * hm_cos[8]))

                        + v93;

        v50.m128_f32[0] = (float)((float)((float)(v83.m128_f32[0] * hm_cos[5]) - v73.m128_f32[0])

                                - (float)(v79.m128_f32[0] * hm_cos[7]))

                        + (float)(v84.m128_f32[0] * hm_cos[1]);

        v100 = v81.m128_f32[0] + v82.m128_f32[0];

        v81.m128_f32[0] = v50.m128_f32[0] + v82.m128_f32[0];

        v82.m128_f32[0] = (float)(v83.m128_f32[0] * hm_cos[7]) - v73.m128_f32[0];

        v83.m128_f32[0] = (float)((float)((float)((float)(v94 + v101) + v107) * hm_cos[6]) - v92) - v93;

        v50.m128_f32[0] = v50.m128_f32[0] + v83.m128_f32[0];

        v73.m128_f32[0] = (float)(v82.m128_f32[0] + (float)(v79.m128_f32[0] * hm_cos[1]))

                        - (float)(v84.m128_f32[0] * hm_cos[5]);

        v84.m128_f32[0] = v73.m128_f32[0] + v83.m128_f32[0];

        v70.m128_f32[0] = v81.m128_f32[0] + v104;

        v81.m128_f32[0] = v81.m128_f32[0] + v95;

        v85 = 32 * ch + sb;

        v82.m128_f32[0] = (float)((float)((float)((float)(v101 * hm_cos[8])

                                                - (float)(v92 * hm_cos[6]))

                                        + (float)(v94 * hm_cos[4]))

                                - (float)(v107 * hm_cos[2]))

                        + v93;

        *ts = (float)(hm_window[bt][0] * v70.m128_f32[0]) + this->hm_buf[ch][sb][0];

        v73.m128_f32[0] = v73.m128_f32[0] + v82.m128_f32[0];

        v102 = v82.m128_f32[0];

        v82.m128_f32[0] = hm_window[bt][17] * v70.m128_f32[0];

        v70.m128_f32[0] = v50.m128_f32[0] + v104;

        v50.m128_f32[0] = v50.m128_f32[0] + v89;

        ts[17] = this->hm_buf[ch][sb][17] - v82.m128_f32[0];

        LODWORD(this->hm_buf[0][v85][0]) = NEGATE_FLOAT_BITS(hm_window[bt][18] * v81.m128_f32[0]);

        LODWORD(this->hm_buf[0][v85][17]) = NEGATE_FLOAT_BITS(hm_window[bt][35] * v81.m128_f32[0]);

        ts[1] = (float)(hm_window[bt][1] * v70.m128_f32[0]) + this->hm_buf[ch][sb][1];

        ts[16] = this->hm_buf[ch][sb][16] - (float)(hm_window[bt][16] * v70.m128_f32[0]);

        LODWORD(this->hm_buf[0][v85][1]) = NEGATE_FLOAT_BITS(hm_window[bt][19] * (float)(v100 + v95));

        LODWORD(this->hm_buf[0][v85][16]) = NEGATE_FLOAT_BITS(hm_window[bt][34] * (float)(v100 + v95));

        ts[2] = (float)(hm_window[bt][2] * v50.m128_f32[0]) + this->hm_buf[ch][sb][2];

        ts[15] = this->hm_buf[ch][sb][15] - (float)(hm_window[bt][15] * v50.m128_f32[0]);

        LODWORD(this->hm_buf[0][v85][2]) = NEGATE_FLOAT_BITS(hm_window[bt][20] * (float)(v100 + v88));

        LODWORD(this->hm_buf[0][v85][15]) = NEGATE_FLOAT_BITS(hm_window[bt][33] * (float)(v100 + v88));

        v70.m128_f32[0] = v84.m128_f32[0] + v89;

        v84.m128_f32[0] = v84.m128_f32[0] + v119;

        ts[3] = (float)(hm_window[bt][3] * v70.m128_f32[0]) + this->hm_buf[ch][sb][3];

        ts[14] = this->hm_buf[ch][sb][14] - (float)(hm_window[bt][14] * v70.m128_f32[0]);

        LODWORD(this->hm_buf[0][v85][3]) = NEGATE_FLOAT_BITS(hm_window[bt][21] * (float)(v110 + v88));

        LODWORD(this->hm_buf[0][v85][14]) = NEGATE_FLOAT_BITS(hm_window[bt][32] * (float)(v110 + v88));

        ts[4] = (float)(hm_window[bt][4] * v84.m128_f32[0]) + this->hm_buf[ch][sb][4];

        ts[13] = this->hm_buf[ch][sb][13] - (float)(hm_window[bt][13] * v84.m128_f32[0]);

        LODWORD(this->hm_buf[0][v85][4]) = NEGATE_FLOAT_BITS(hm_window[bt][22] * (float)(v110 + v97));

        LODWORD(this->hm_buf[0][v85][13]) = NEGATE_FLOAT_BITS(hm_window[bt][31] * (float)(v110 + v97));

        ts[5] = (float)(hm_window[bt][5] * (float)(v73.m128_f32[0] + v119)) + this->hm_buf[ch][sb][5];

        ts[12] = this->hm_buf[ch][sb][12] - (float)(hm_window[bt][12] * (float)(v73.m128_f32[0] + v119));

        LODWORD(this->hm_buf[0][v85][5]) = NEGATE_FLOAT_BITS(hm_window[bt][23] * (float)(v116 + v97));

        LODWORD(this->hm_buf[0][v85][12]) = NEGATE_FLOAT_BITS(hm_window[bt][30] * (float)(v116 + v97));

        ts[6] = (float)(hm_window[bt][6] * (float)(v73.m128_f32[0] + v90.m128_f32[0]))

              + this->hm_buf[ch][sb][6];

        ts[11] = this->hm_buf[ch][sb][11]

               - (float)(hm_window[bt][11] * (float)(v73.m128_f32[0] + v90.m128_f32[0]));

        LODWORD(this->hm_buf[0][v85][6]) = NEGATE_FLOAT_BITS(hm_window[bt][24] * (float)(v116 + v87));

        LODWORD(this->hm_buf[0][v85][11]) = NEGATE_FLOAT_BITS(hm_window[bt][29] * (float)(v116 + v87));

        ts[7] = (float)(hm_window[bt][7] * (float)(v102 + v90.m128_f32[0])) + this->hm_buf[ch][sb][7];

        ts[10] = this->hm_buf[ch][sb][10] - (float)(hm_window[bt][10] * (float)(v102 + v90.m128_f32[0]));

        LODWORD(this->hm_buf[0][v85][7]) = NEGATE_FLOAT_BITS(hm_window[bt][25] * (float)(v114 + v87));

        LODWORD(this->hm_buf[0][v85][10]) = NEGATE_FLOAT_BITS(hm_window[bt][28] * (float)(v114 + v87));

        ts[8] = (float)(hm_window[bt][8] * (float)(v102 + 0.0)) + this->hm_buf[ch][sb][8];

        ts[9] = this->hm_buf[ch][sb][9] - (float)(hm_window[bt][9] * (float)(v102 + 0.0));

        LODWORD(this->hm_buf[0][v85][8]) = NEGATE_FLOAT_BITS(hm_window[bt][26] * (float)(v114 + v86));

        LODWORD(this->hm_buf[0][v85][9]) = NEGATE_FLOAT_BITS(hm_window[bt][27] * (float)(v114 + v86));

        return;

      }

    }

    v46 = 0;

    v47 = this->hm_buf[ch][sb];

    do

    {

      v48 = *v47++;

      ts[v46++] = v48;

      *(v47 - 1) = 0.0;

    }

    while ( v46 < 18 );

  }

}



//----- (0046F3D0) --------------------------------------------------------

void SMpegAudioDecoder::L3_HybridInitialize()

{

  memset(this->hm_buf, 0, sizeof(this->hm_buf));

}



//----- (0046FD80) --------------------------------------------------------

void SMpegAudioDecoder::L3_Antialias(int gr, int ch)

{

  char *v4; // edx

  int v5; // ecx

  bool v6; // zf

  int v7; // ecx

  float *v8; // eax

  float v9; // xmm3_4

  float v10; // xmm3_4

  float v11; // xmm3_4

  float v12; // xmm3_4

  float v13; // xmm3_4

  float v14; // xmm3_4

  float v15; // xmm3_4

  float v16; // xmm3_4



  v4 = (char *)this + 160 * ch;

  if ( !*(_DWORD *)&v4[72 * gr + 112] )

  {

LABEL_6:

    v7 = 31;

LABEL_7:

    v8 = &this->lr[ch][18];

    do

    {

      v9 = *(v8 - 1);

      *(v8 - 1) = (float)(cs[0] * v9) - (float)(ca[0] * *v8);

      *v8 = (float)(cs[0] * *v8) + (float)(ca[0] * v9);

      v10 = *(v8 - 2);

      *(v8 - 2) = (float)(cs[1] * v10) - (float)(ca[1] * v8[1]);

      v8[1] = (float)(cs[1] * v8[1]) + (float)(ca[1] * v10);

      v11 = *(v8 - 3);

      *(v8 - 3) = (float)(cs[2] * v11) - (float)(ca[2] * v8[2]);

      v8[2] = (float)(cs[2] * v8[2]) + (float)(ca[2] * v11);

      v12 = *(v8 - 4);

      *(v8 - 4) = (float)(cs[3] * v12) - (float)(ca[3] * v8[3]);

      v8[3] = (float)(cs[3] * v8[3]) + (float)(ca[3] * v12);

      v13 = *(v8 - 5);

      *(v8 - 5) = (float)(cs[4] * v13) - (float)(ca[4] * v8[4]);

      v8[4] = (float)(cs[4] * v8[4]) + (float)(ca[4] * v13);

      v14 = *(v8 - 6);

      *(v8 - 6) = (float)(cs[5] * v14) - (float)(ca[5] * v8[5]);

      v8[5] = (float)(cs[5] * v8[5]) + (float)(ca[5] * v14);

      v15 = *(v8 - 7);

      *(v8 - 7) = (float)(cs[6] * v15) - (float)(ca[6] * v8[6]);

      v8[6] = (float)(cs[6] * v8[6]) + (float)(ca[6] * v15);

      v16 = *(v8 - 8);

      *(v8 - 8) = (float)(cs[7] * v16) - (float)(ca[7] * v8[7]);

      v8[7] = (float)(cs[7] * v8[7]) + (float)(ca[7] * v16);

      v8 += 18;

      --v7;

    }

    while ( v7 );

    return;

  }

  v5 = *(_DWORD *)&v4[72 * gr + 116];

  if ( v5 != 2 || *(_DWORD *)&v4[72 * gr + 120] )

  {

    if ( *(_DWORD *)&v4[72 * gr + 120] )

    {

      v6 = v5 == 2;

      v7 = 1;

      if ( v6 )

        goto LABEL_7;

    }

    goto LABEL_6;

  }

}
