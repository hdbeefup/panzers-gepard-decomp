// mdec/decode.cpp
// MP3 frame decoder — main entry points
// Decompiled from: gameSplit/mp3_decoder.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <corecrt_math.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "decode.h"
#include "hdbeefup.h"

// Classes: SMpegAudioDecoder
// Function count: 4

//----- (0046DD20) --------------------------------------------------------

SMpegAudioDecoder::SMpegAudioDecoder(
        SStream *is,

        SMpegAudioCallBack *callback)

{

  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: this=%p is=%p callback=%p", this, is, callback);
  #endif
  if ( !PrecalculateCalled )

    Logger.g->Panic("SMpegAudioDecoder::SMpegAudioDecoder: MpegAudioPrecalculate was not called.");

  this->Callback = callback;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: in_open");
  #endif
  in_open(is);
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: s2_init");
  #endif
  s2_init();
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: SubbandInitialize");
  #endif
  SubbandInitialize();
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: L3_HybridInitialize");
  #endif
  L3_HybridInitialize();

  this->Frame.num = 0;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SMpegAudioDecoder::ctor: done");
  #endif

}



//----- (0046DD80) --------------------------------------------------------

SMpegAudioDecoder::~SMpegAudioDecoder()

{

  in_close();

}



//----- (0046DDC0) --------------------------------------------------------

int SMpegAudioDecoder::DecodeFrame()

{

  int result; // eax

  unsigned int v3; // eax

  int version; // esi

  int sampling_frequency; // ebx

  int v6; // ebx

  int v7; // ecx

  int v8; // eax

  int num; // ecx



  result = in_seeksync();

  if ( result )

  {
    this->Frame.version = in_getbits(1);

    this->Frame.lay = 4 - in_getbits(2);

    this->Frame.error_protection = in_getbits(1) == 0;

    this->Frame.bitrate_index = in_getbits(4);

    this->Frame.sampling_frequency = in_getbits(2);

    this->Frame.padding = in_getbits(1);

    this->Frame.extension = in_getbits(1);

    this->Frame.mode = in_getbits(2);

    this->Frame.mode_ext = in_getbits(2);

    this->Frame.copyright = in_getbits(1);

    this->Frame.original = in_getbits(1);

    v3 = in_getbits(2);

    version = this->Frame.version;

    sampling_frequency = this->Frame.sampling_frequency;

    this->Frame.emphasis = v3;

    v6 = 4 * version + sampling_frequency;

    this->Frame.stereo = (this->Frame.mode != 3) + 1;

    v7 = 144000 * (int)(&(&layer_names[64 * version])[16 * this->Frame.lay])[this->Frame.bitrate_index] / s_freq[v6];

    if ( !version )

      v7 >>= 1;

    v8 = v7 + this->Frame.padding - 4;

    num = this->Frame.num;

    this->Frame.size = v8;

    this->Frame.real_freq = s_freq[v6];

    this->Frame.num = num + 1;

    Logger.g->Log(2, "Current frame: %d", num);

    if ( this->Frame.error_protection )

    {

      in_getbits(16);

      this->Frame.size -= 2;

    }

    if ( this->Frame.lay != 3 )

      Logger.g->Panic("SMpegAudioDecoder::DecodeFrame: Layer type not supported.");

    return L3_DecodeFrame();

  }

  return result;

}



//----- (00482880) --------------------------------------------------------

extern "C" DWORD WINAPI StreamThreadProc(LPVOID param)

{
  SMpegAudioDecoder *lpParameter = (SMpegAudioDecoder *)param;

  int v2; // [esp+0h] [ebp-24h] BYREF

  int *v3; // [esp+14h] [ebp-10h]

  int v4; // [esp+20h] [ebp-4h]



  v3 = &v2;

  v4 = 0;

  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "StreamThreadProc: starting decode loop, decoder=%p", lpParameter);
  #endif
  while ( lpParameter->DecodeFrame() && !StreamThreadShutdown )

    ;

  v4 = -1;

  if ( lpParameter )

  {

    lpParameter->~SMpegAudioDecoder();

    operator delete(lpParameter);

  }

  StreamThreadRunning = 0;

  if ( !StreamThreadShutdown )

    Concert->NextTrack();

  return 0;

}
