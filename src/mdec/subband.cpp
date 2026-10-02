// mdec/subband.cpp
// Subband synthesis for MP3 decoder
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

//----- (00472270) --------------------------------------------------------

void SMpegAudioDecoder::SubbandInitialize()

{

  int *sb_buf_ofs; // edx

  int v2; // ebx

  float *v3; // esi



  sb_buf_ofs = this->sb_buf_ofs;

  v2 = 2;

  v3 = this->sb_buf[0];

  do

  {

    ++sb_buf_ofs;

    memset(v3, 0, 0x880u);

    v3 += 544;

    *(sb_buf_ofs - 1) = 64;

    --v2;

  }

  while ( v2 );

}



//----- (004724B0) --------------------------------------------------------

int SMpegAudioDecoder::SubbandSynthesis(float *band, int channel, short *sample)

{

  float v4; // xmm4_4

  float v6; // xmm0_4

  float v7; // xmm5_4

  float v8; // xmm4_4

  float v9; // xmm6_4

  float v10; // xmm2_4

  float v11; // xmm5_4

  float v12; // xmm0_4

  float v13; // xmm6_4

  float v14; // xmm4_4

  float v15; // xmm0_4

  float v16; // xmm6_4

  float v17; // xmm5_4

  float v18; // xmm4_4

  float v19; // xmm6_4

  float v20; // xmm2_4

  float v21; // xmm5_4

  float v22; // xmm0_4

  float v23; // xmm6_4

  float v24; // xmm4_4

  float v25; // xmm0_4

  float v26; // xmm6_4

  float v27; // xmm5_4

  float v28; // xmm4_4

  float v29; // xmm2_4

  float v30; // xmm6_4

  float v31; // xmm5_4

  float v32; // xmm0_4

  float v33; // xmm6_4

  float v34; // xmm4_4

  float v35; // xmm0_4

  float v36; // xmm5_4

  float v37; // xmm2_4

  float v38; // xmm6_4

  float v39; // xmm4_4

  float v40; // xmm5_4

  float v41; // xmm0_4

  float v42; // xmm6_4

  float v43; // xmm4_4

  float v44; // xmm0_4

  float v45; // xmm5_4

  float v46; // xmm6_4

  float v47; // xmm5_4

  float v48; // xmm4_4

  float v49; // xmm6_4

  float v50; // xmm2_4

  float v51; // xmm5_4

  float v52; // xmm0_4

  float v53; // xmm6_4

  float v54; // xmm4_4

  float v55; // xmm0_4

  float v56; // xmm6_4

  float v57; // xmm5_4

  float v58; // xmm4_4

  float v59; // xmm6_4

  float v60; // xmm2_4

  float v61; // xmm5_4

  float v62; // xmm0_4

  float v63; // xmm6_4

  float v64; // xmm4_4

  float v65; // xmm0_4

  float v66; // xmm6_4

  float v67; // xmm5_4

  float v68; // xmm4_4

  float v69; // xmm2_4

  float v70; // xmm6_4

  float v71; // xmm5_4

  float v72; // xmm0_4

  float v73; // xmm6_4

  float v74; // xmm4_4

  float v75; // xmm7_4

  float v76; // xmm0_4

  float v77; // xmm6_4

  float v78; // xmm4_4

  float v79; // xmm5_4

  float v80; // xmm7_4

  float v81; // xmm1_4

  float v82; // xmm6_4

  float v83; // xmm7_4

  float v84; // xmm1_4

  float v85; // xmm4_4

  float v86; // xmm6_4

  float v87; // xmm2_4

  float v88; // xmm1_4

  float v89; // xmm3_4

  float v90; // xmm5_4

  float v91; // xmm7_4

  float v92; // xmm2_4

  float v93; // xmm3_4

  float v94; // xmm1_4

  float v95; // xmm2_4

  float v96; // xmm0_4

  float v97;

  float v98; // xmm2_4

  float v99; // xmm1_4

  float v100;

  float v101; // xmm1_4

  float v102; // xmm2_4

  float v103; // xmm5_4

  float v104; // xmm2_4

  float v105; // xmm2_4

  float v106; // xmm1_4

  float v107; // xmm3_4

  float v108; // xmm4_4

  float v109; // xmm2_4

  float v110; // xmm7_4

  float v111; // xmm4_4

  float v112; // xmm2_4

  float v113; // xmm4_4

  float v114; // xmm1_4

  float v115; // xmm1_4

  float v116; // xmm2_4

  float v117; // xmm3_4

  float v118; // xmm4_4

  float v119; // xmm6_4

  float v120; // xmm4_4

  float v121; // xmm6_4

  float v122; // xmm2_4

  float v123; // xmm1_4

  float v124; // xmm3_4

  float v125; // xmm4_4

  int v126; // ecx

  float *v127; // edx

  float v128; // xmm5_4

  float v129; // xmm3_4

  float v130; // xmm2_4

  float v131; // xmm4_4

  float v132; // xmm1_4

  float v133; // xmm2_4

  float v134; // xmm3_4

  float v135; // xmm2_4

  float v136; // xmm7_4

  float v137; // xmm1_4

  float v138; // xmm2_4

  float *v139; // edx

  float v140; // xmm1_4

  float v141; // xmm3_4

  float v142; // xmm7_4

  float *v143; // edx

  float *v144; // ecx

  int i; // esi

  float v146; // xmm1_4

  float v147; // xmm0_4

  float v148; // xmm1_4

  float v149; // xmm0_4

  int v150; // esi

  float v151; // xmm0_4

  float v152; // xmm0_4

  int *v153; // esi

  int j; // eax

  short v155; // dx

  int v156; // ecx

  float v158; // [esp+10h] [ebp-114h]

  float v159; // [esp+10h] [ebp-114h]

  float v160; // [esp+14h] [ebp-110h]

  int v161; // [esp+14h] [ebp-110h]

  float v162; // [esp+18h] [ebp-10Ch]

  float v163; // [esp+18h] [ebp-10Ch]

  float v164; // [esp+1Ch] [ebp-108h]

  float v165; // [esp+1Ch] [ebp-108h]

  float v166; // [esp+20h] [ebp-104h]

  float v167; // [esp+20h] [ebp-104h]

  float v168; // [esp+24h] [ebp-100h]

  float v169; // [esp+24h] [ebp-100h]

  float v170; // [esp+28h] [ebp-FCh]

  float v171; // [esp+28h] [ebp-FCh]

  float v172; // [esp+2Ch] [ebp-F8h]

  float v173; // [esp+2Ch] [ebp-F8h]

  float v174; // [esp+30h] [ebp-F4h]

  float v175; // [esp+30h] [ebp-F4h]

  float v176; // [esp+34h] [ebp-F0h]

  float v177; // [esp+34h] [ebp-F0h]

  float v178; // [esp+38h] [ebp-ECh]

  float v179; // [esp+38h] [ebp-ECh]

  float v180; // [esp+3Ch] [ebp-E8h]

  float v181; // [esp+3Ch] [ebp-E8h]

  int v182; // [esp+40h] [ebp-E4h]

  float v183; // [esp+44h] [ebp-E0h]

  float v184; // [esp+44h] [ebp-E0h]

  float v185; // [esp+48h] [ebp-DCh]

  float v186; // [esp+48h] [ebp-DCh]

  float v187; // [esp+4Ch] [ebp-D8h]

  float v188; // [esp+4Ch] [ebp-D8h]

  float v189; // [esp+4Ch] [ebp-D8h]

  float v190; // [esp+50h] [ebp-D4h]

  float v191; // [esp+50h] [ebp-D4h]

  int v192; // [esp+54h] [ebp-D0h]

  float v193; // [esp+58h] [ebp-CCh]

  float v194; // [esp+58h] [ebp-CCh]

  float v195; // [esp+58h] [ebp-CCh]

  float v196; // [esp+5Ch] [ebp-C8h]

  float v197; // [esp+5Ch] [ebp-C8h]

  float v198; // [esp+5Ch] [ebp-C8h]

  float v199; // [esp+60h] [ebp-C4h]

  float v200; // [esp+60h] [ebp-C4h]

  float v201; // [esp+64h] [ebp-C0h]

  float v202; // [esp+64h] [ebp-C0h]

  float v203; // [esp+64h] [ebp-C0h]

  float v204; // [esp+68h] [ebp-BCh]

  float v205; // [esp+68h] [ebp-BCh]

  float v206; // [esp+68h] [ebp-BCh]

  float v207; // [esp+6Ch] [ebp-B8h]

  float v208; // [esp+6Ch] [ebp-B8h]

  float v209; // [esp+6Ch] [ebp-B8h]

  float v210; // [esp+70h] [ebp-B4h]

  float v211; // [esp+70h] [ebp-B4h]

  float v212; // [esp+70h] [ebp-B4h]

  float v213; // [esp+74h] [ebp-B0h]

  float v214; // [esp+74h] [ebp-B0h]

  float v215; // [esp+74h] [ebp-B0h]

  float v216; // [esp+78h] [ebp-ACh]

  float v217; // [esp+78h] [ebp-ACh]

  float v218; // [esp+78h] [ebp-ACh]

  float v219; // [esp+7Ch] [ebp-A8h]

  float v220; // [esp+7Ch] [ebp-A8h]

  float v221; // [esp+7Ch] [ebp-A8h]

  float v222; // [esp+80h] [ebp-A4h]

  float v223; // [esp+80h] [ebp-A4h]

  float v224; // [esp+80h] [ebp-A4h]

  float v225; // [esp+80h] [ebp-A4h]

  float v226; // [esp+84h] [ebp-A0h]

  float v227; // [esp+84h] [ebp-A0h]

  float v228; // [esp+84h] [ebp-A0h]

  float v229; // [esp+84h] [ebp-A0h]

  float v230; // [esp+88h] [ebp-9Ch]

  float v231; // [esp+88h] [ebp-9Ch]

  float v232; // [esp+88h] [ebp-9Ch]

  float v233; // [esp+88h] [ebp-9Ch]

  float v234; // [esp+8Ch] [ebp-98h]

  float v235; // [esp+8Ch] [ebp-98h]

  float v236; // [esp+8Ch] [ebp-98h]

  float v237; // [esp+90h] [ebp-94h]

  float v238; // [esp+90h] [ebp-94h]

  float v239; // [esp+90h] [ebp-94h]

  float v240; // [esp+90h] [ebp-94h]

  float v241; // [esp+94h] [ebp-90h]

  float v242; // [esp+94h] [ebp-90h]

  float v243; // [esp+94h] [ebp-90h]

  float v244; // [esp+94h] [ebp-90h]

  float v245; // [esp+98h] [ebp-8Ch]

  float v246; // [esp+98h] [ebp-8Ch]

  float v247; // [esp+98h] [ebp-8Ch]

  float v248; // [esp+98h] [ebp-8Ch]

  int samp[33]; // [esp+9Ch] [ebp-88h] BYREF



  v4 = band[15];

  v6 = band[16] + v4;

  v7 = *band + band[31];

  v192 = 0;

  v8 = (float)(v4 - band[16]) * sb_1m[15];

  v9 = (float)(*band - band[31]) * sb_1m[0];

  v10 = v6 + v7;

  v11 = v7 - v6;

  v12 = v8 + v9;

  v13 = v9 - v8;

  v14 = band[14];

  v234 = v10;

  v201 = v12;

  v15 = band[17] + v14;

  v185 = v13 * sb_2m[0];

  v16 = band[1];

  v204 = v11 * sb_2m[0];

  v17 = v16 + band[30];

  v18 = (float)(v14 - band[17]) * sb_1m[14];

  v19 = (float)(v16 - band[30]) * sb_1m[1];

  v20 = v15 + v17;

  v21 = v17 - v15;

  v22 = v18 + v19;

  v23 = v19 - v18;

  v24 = band[13];

  v237 = v20;

  v199 = v22;

  v25 = band[18] + v24;

  v183 = v23 * sb_2m[1];

  v26 = band[2];

  v213 = v21 * sb_2m[1];

  v27 = v26 + band[29];

  v28 = (float)(v24 - band[18]) * sb_1m[13];

  v29 = v25 + v27;

  v30 = (float)(v26 - band[29]) * sb_1m[2];

  v31 = v27 - v25;

  v32 = v28 + v30;

  v241 = v29;

  v33 = v30 - v28;

  v34 = band[12];

  v219 = v31 * sb_2m[2];

  v222 = v32;

  v193 = v33 * sb_2m[2];

  v35 = band[19] + v34;

  v36 = band[3] + band[28];

  v37 = v35 + v36;

  v38 = (float)(band[3] - band[28]) * sb_1m[3];

  v39 = (float)(v34 - band[19]) * sb_1m[12];

  v40 = (float)(v36 - v35) * sb_2m[3];

  v245 = v37;

  v41 = v39 + v38;

  v42 = v38 - v39;

  v226 = v40;

  v43 = band[11];

  v196 = v41;

  v44 = band[20] + v43;

  v216 = v42 * sb_2m[3];

  v45 = band[4];

  v46 = v45 - band[27];

  v47 = v45 + band[27];

  v48 = (float)(v43 - band[20]) * sb_1m[11];

  v49 = v46 * sb_1m[4];

  v50 = v44 + v47;

  v51 = v47 - v44;

  v52 = v48 + v49;

  v53 = v49 - v48;

  v54 = band[10];

  v174 = v50;

  v170 = v52;

  v55 = band[21] + v54;

  v168 = v53 * sb_2m[4];

  v56 = band[5];

  v172 = v51 * sb_2m[4];

  v57 = v56 + band[26];

  v58 = (float)(v54 - band[21]) * sb_1m[10];

  v59 = (float)(v56 - band[26]) * sb_1m[5];

  v60 = v55 + v57;

  v61 = v57 - v55;

  v62 = v58 + v59;

  v63 = v59 - v58;

  v64 = band[9];

  v230 = v60;

  v178 = v62;

  v176 = v63 * sb_2m[5];

  v65 = band[22] + v64;

  v66 = band[6];

  v180 = v61 * sb_2m[5];

  v67 = v66 + band[25];

  v68 = (float)(v64 - band[22]) * sb_1m[9];

  v69 = v65 + v67;

  v70 = (float)(v66 - band[25]) * sb_1m[6];

  v71 = v67 - v65;

  v72 = v68 + v70;

  v187 = v71 * sb_2m[6];

  v73 = (float)(v70 - v68) * sb_2m[6];

  v207 = v72;

  v74 = band[8];

  v75 = band[7];

  v76 = band[23] + v74;

  v210 = v73;

  v77 = v75 + band[24];

  v78 = (float)(v74 - band[23]) * sb_1m[8];

  v79 = v76 + v77;

  v80 = (float)(v75 - band[24]) * sb_1m[7];

  v81 = v78 + v80;

  v82 = (float)(v77 - v76) * sb_2m[7];

  v190 = v79 + v234;

  v83 = (float)(v80 - v78) * sb_2m[7];

  v235 = (float)(v234 - v79) * sb_3m[0];

  v166 = v204 + v82;

  v205 = (float)(v204 - v82) * sb_3m[0];

  v164 = v201 + v81;

  v202 = (float)(v201 - v81) * sb_3m[0];

  v160 = v185 + v83;

  v186 = (float)(v185 - v83) * sb_3m[0];

  v84 = v230;

  v85 = v237 + v69;

  v238 = (float)(v237 - v69) * sb_3m[1];

  v86 = v187 + v213;

  v214 = (float)(v213 - v187) * sb_3m[1];

  v162 = v207 + v199;

  v200 = (float)(v199 - v207) * sb_3m[1];

  v158 = v210 + v183;

  v184 = (float)(v183 - v210) * sb_3m[1];

  v231 = v230 + v241;

  v87 = v241 - v84;

  v88 = v168;

  v242 = v87 * sb_3m[2];

  v211 = v180 + v219;

  v220 = (float)(v219 - v180) * sb_3m[2];

  v188 = v178 + v222;

  v223 = (float)(v222 - v178) * sb_3m[2];

  v208 = v176 + v193;

  v194 = (float)(v193 - v176) * sb_3m[2];

  v89 = v174 + v245;

  v246 = (float)(v245 - v174) * sb_3m[3];

  v90 = v172 + v226;

  v227 = (float)(v226 - v172) * sb_3m[3];

  v91 = v170 + v196;

  v197 = (float)(v196 - v170) * sb_3m[3];

  v169 = v168 + v216;

  v217 = (float)(v216 - v88) * sb_3m[3];

  v92 = v190 - v89;

  v93 = v89 + v190;

  v94 = (float)(v85 - v231) * sb_4m[1];

  v232 = v231 + v85;

  v95 = v92 * sb_4m[0];

  v96 = (float)(v95 - v94) * 0.7071067811865001f;

  v173 = v96;

  v181 = (float)(v96 + v95) + v94;

  v97 = (float)(v93 - v232);

  v233 = v232 + v93;

  *(float *)&v97 = v97 * 0.7071067811865001f;

  v182 = LODWORD(v97);

  v98 = (float)(v235 - v246) * sb_4m[0];

  v99 = (float)(v238 - v242) * sb_4m[1];

  v247 = v246 + v235;

  v243 = v242 + v238;

  *(float *)&v97 = (float)(v98 - v99) * 0.7071067811865001f;

  v236 = *(float *)&v97;

  v239 = (float)(*(float *)&v97 + v98) + v99;

  v100 = (float)(v247 - v243) * 0.7071067811865001f;

  v244 = (float)(v239 + v247) + v243;

  *(float *)&v100 = v100;

  v101 = (float)(v86 - v211) * sb_4m[1];

  v240 = v239 + *(float *)&v100;

  v171 = *(float *)&v100 + v236;

  v102 = v166 - v90;

  v103 = v90 + v166;

  v104 = v102 * sb_4m[0];

  *(float *)&v100 = (float)(v104 - v101) * 0.7071067811865001f;

  v191 = *(float *)&v100;

  v179 = (float)(*(float *)&v100 + v104) + v101;

  *(float *)&v100 = v103 - (float)(v211 + v86);

  v212 = (float)(v211 + v86) + v103;

  *(float *)&v100 = *(float *)&v100 * 0.7071067811865001f;

  v248 = *(float *)&v100;

  v105 = (float)(v205 - v227) * sb_4m[0];

  v228 = v227 + v205;

  v106 = (float)(v214 - v220) * sb_4m[1];

  v221 = v220 + v214;

  v107 = (float)(v105 - v106) * 0.7071067811865001f;

  v175 = v107;

  v108 = v107 + v105;

  v109 = v164 - v91;

  v110 = v91 + v164;

  v111 = v108 + v106;

  v112 = v109 * sb_4m[0];

  *(float *)&v100 = (float)(v228 - v221) * 0.7071067811865001f;

  v206 = (float)(v111 + v228) + v221;

  v177 = v111 + *(float *)&v100;

  v167 = *(float *)&v100 + v107;

  v113 = v188 + v162;

  v114 = (float)(v162 - v188) * sb_4m[1];

  *(float *)&v100 = (float)(v112 - v114) * 0.7071067811865001f;

  v163 = *(float *)&v100;

  v229 = (float)(*(float *)&v100 + v112) + v114;

  v115 = (float)(v200 - v223) * sb_4m[1];

  v224 = v223 + v200;

  v189 = v113 + v110;

  *(float *)&v100 = (float)(v110 - v113) * 0.7071067811865001f;

  v215 = *(float *)&v100;

  v116 = (float)(v202 - v197) * sb_4m[0];

  v198 = v197 + v202;

  v117 = (float)(v116 - v115) * 0.7071067811865001f;

  v165 = v117;

  v118 = (float)(v117 + v116) + v115;

  v119 = (float)(v198 - v224) * 0.7071067811865001f;

  *(float *)&v100 = v118 + v198;

  v120 = v118 + v119;

  v121 = v119 + v117;

  v203 = v120;

  v122 = (float)(v160 - v169) * sb_4m[0];

  v225 = *(float *)&v100 + v224;

  v123 = (float)(v158 - v208) * sb_4m[1];

  v124 = v169 + v160;

  v125 = v208 + v158;

  v126 = ((unsigned char)this->sb_buf_ofs[channel] - 1) & 0xF;

  v161 = 16 * (v126 & 1);

  v127 = &this->sb_buf[channel][v126 + v161];

  v128 = (float)(v122 - v123) * 0.7071067811865001f;

  v159 = (float)(v128 + v122) + v123;

  v209 = v125 + v124;

  *(float *)&v100 = (float)(v124 - v125) * 0.7071067811865001f;

  v129 = (float)(v186 - v217) * sb_4m[0];

  v218 = v217 + v186;

  v130 = (float)(v184 - v194) * sb_4m[1];

  this->sb_buf_ofs[channel] = v126;

  v195 = v194 + v184;

  v131 = (float)(v129 - v130) * 0.7071067811865001f;

  v132 = (float)(v131 + v129) + v130;

  v133 = (float)(v218 - v195) * 0.7071067811865001f;

  *(_DWORD *)v127 = v182;

  v134 = v133 + v132;

  v135 = v133 + v131;

  v136 = (float)(v132 + v218) + v195;

  v127[64] = v167 + v248;

  v127[128] = v171;

  v127[192] = v167 + v191;

  v127[256] = v173;

  v127[320] = v175 + v191;

  v127[384] = v236;

  v127[448] = v175;

  v137 = v135 + *(float *)&v100;

  v138 = v135 + v128;

  v127[480] = v131;

  v127[32] = v215 + v137;

  v127[96] = v121 + v137;

  v127[160] = v121 + v138;

  v127[288] = v163 + (float)(v131 + v128);

  v127[224] = v163 + v138;

  v127[352] = v165 + (float)(v131 + v128);

  v127[416] = v131 + v165;

  v139 = (float *)((char *)v127 + 64 - ((v126 & 1) << 7));

  v139[512] = v233;

  *(_DWORD *)v139 = v182;

  v139[64] = v177 + v248;

  v139[128] = v240;

  v139[256] = v181;

  v139[192] = v177 + v179;

  v139[320] = v206 + v179;

  v139[448] = v206 + v212;

  v140 = v134 + *(float *)&v100;

  v141 = v134 + v159;

  v139[384] = v244;

  v139[32] = v215 + v140;

  v139[96] = v203 + v140;

  v139[160] = v203 + v141;

  v139[288] = v229 + (float)(v136 + v159);

  v139[224] = v229 + v141;

  v139[352] = v225 + (float)(v136 + v159);

  v142 = v136 + v209;

  v139[416] = v225 + v142;

  v139[480] = v189 + v142;

  v143 = &this->sb_buf[0][16 * ((v126 & 1) + 34 * channel)];

  v144 = (float *)((char *)&sb_window[16] - 4 * this->sb_buf_ofs[channel]);

  for ( i = 0; i < 16; ++i )

  {

    v146 = (float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)(v144[1] * v143[1]) + (float)(*v144 * *v143)) + (float)(v144[2] * v143[2]))

                                                                                                 + (float)(v144[3] * v143[3]))

                                                                                         + (float)(v144[4] * v143[4]))

                                                                                 + (float)(v144[5] * v143[5]))

                                                                         + (float)(v144[6] * v143[6]))

                                                                 + (float)(v144[7] * v143[7]))

                                                         + (float)(v144[8] * v143[8]))

                                                 + (float)(v144[9] * v143[9]))

                                         + (float)(v144[10] * v143[10]))

                                 + (float)(v144[11] * v143[11]))

                         + (float)(v144[12] * v143[12]))

                 + (float)(v144[13] * v143[13]))

         + (float)(v144[14] * v143[14]);

    v147 = v144[15] * v143[15];

    v144 += 32;

    v143 += 32;

    samp[i] = (int)(float)(v146 + v147);

  }

  if ( v161 )

  {

    v148 = (float)((float)((float)((float)((float)((float)(v144[2] * v143[2]) + (float)(*v144 * *v143))

                                         + (float)(v144[4] * v143[4]))

                                 + (float)(v144[6] * v143[6]))

                         + (float)(v144[8] * v143[8]))

                 + (float)(v144[10] * v143[10]))

         + (float)(v144[12] * v143[12]);

    v149 = v144[14] * v143[14];

  }

  else

  {

    v148 = (float)((float)((float)((float)((float)((float)(v144[3] * v143[3]) + (float)(v144[1] * v143[1]))

                                         + (float)(v144[5] * v143[5]))

                                 + (float)(v144[7] * v143[7]))

                         + (float)(v144[9] * v143[9]))

                 + (float)(v144[11] * v143[11]))

         + (float)(v144[13] * v143[13]);

    v149 = v144[15] * v143[15];

  }

  v150 = 17;

  samp[16] = (int)(float)(v148 + v149);

  do

  {

    v151 = v144[32];

    v144 += 32;

    v152 = v151 * *(v143 - 32);

    v143 -= 32;

    samp[v150++] = (int)(float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)((float)(v144[1] * v143[1]) + v152) + (float)(v144[2] * v143[2])) + (float)(v144[3] * v143[3])) + (float)(v144[4] * v143[4])) + (float)(v144[5] * v143[5])) + (float)(v144[6] * v143[6]))

                                                                                              + (float)(v144[7] * v143[7]))

                                                                                      + (float)(v144[8] * v143[8]))

                                                                              + (float)(v144[9] * v143[9]))

                                                                      + (float)(v144[10] * v143[10]))

                                                              + (float)(v144[11] * v143[11]))

                                                      + (float)(v144[12] * v143[12]))

                                              + (float)(v144[13] * v143[13]))

                                      + (float)(v144[14] * v143[14]))

                              + (float)(v144[15] * v143[15]));

  }

  while ( v150 < 32 );

  v153 = samp;

  for ( j = 0; j < 32; ++j )

  {

    if ( *v153 < 0x8000 )

    {

      if ( *v153 >= -32768 )

      {

        v155 = *v153;

        v156 = v192;

      }

      else

      {

        v155 = (short)0x8000;


        v156 = ++v192;

      }

    }

    else

    {

      v155 = 0x7FFF;

      v156 = ++v192;

    }

    sample[j] = v155;

    ++v153;

  }

  return v156;

}
