// mdec/decode.h
// MP3 decoder types and declarations
#ifndef MDEC_DECODE_H
#define MDEC_DECODE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <xmmintrin.h>
#include <string.h>
#include <math.h>

// IDA type aliases
#ifndef _DWORD
typedef unsigned int _DWORD;
#endif
#ifndef _BYTE
typedef unsigned char _BYTE;
#endif
#ifndef _WORD
typedef unsigned short _WORD;
#endif
#ifndef _QWORD
typedef unsigned long long _QWORD;
#endif

// IDA byte/word extraction macros
#ifndef BYTE1
#define BYTE1(x)    (*(((_BYTE*)&(x)) + 1))
#define BYTE2(x)    (*(((_BYTE*)&(x)) + 2))
#define BYTE3(x)    (*(((_BYTE*)&(x)) + 3))
#endif
#ifndef LODWORD
#define LODWORD(x)  (*((_DWORD*)&(x)))
#define HIDWORD(x)  (*(((_DWORD*)&(x)) + 1))
#endif
#ifndef SLODWORD
#define SLODWORD(x) (*((int*)&(x)))
#endif

// IDA 128-bit type
#ifndef _OWORD_DEFINED
#define _OWORD_DEFINED
typedef struct { unsigned long long lo, hi; } _OWORD;
#endif

// IDA type coercion macros
#ifndef COERCE_UNSIGNED_INT
#define COERCE_UNSIGNED_INT(x) (*((unsigned int *)&(x)))
#endif
#ifndef COERCE_FLOAT
#define COERCE_FLOAT(x) (*((float *)&(x)))
#endif
// Reinterpret int bits as float (for IDA *(float*)&dword_XXX pattern)
#ifndef FLOAT_OF
#define FLOAT_OF(x) (*((float *)&(x)))
#endif
// Float negation via sign-bit XOR
inline unsigned int NEGATE_FLOAT_BITS(float f) {
    unsigned int u;
    memcpy(&u, &f, sizeof(u));
    return u ^ 0x80000000u;
}

// IDA qualifiers
#define __cppobj

// IDA sign-bit constant for float negation via XOR
static const unsigned int _xmm = 0x80000000u;

// Helper to cast unsigned int bits to __m128
inline __m128 UINT_TO_M128(unsigned int v) {
    float f;
    memcpy(&f, &v, sizeof(f));
    return _mm_set_ss(f);
}

// Use core headers for SLogger, LoggerGlobal, SStream
#include "logger.h"
#include "stream.h"

// SMpegAudioCallBack — manual vtable dispatch (matches original binary layout)
struct SMpegAudioCallBack_vtbl;

struct SMpegAudioCallBack {
    SMpegAudioCallBack_vtbl *vftable;
    void DataCallback(short *a, short *b, int c, int d, int e);
};

struct SMpegAudioCallBack_vtbl {
    void (__thiscall *DataCallback)(SMpegAudioCallBack *self, short *, short *, int, int, int);
};

inline void SMpegAudioCallBack::DataCallback(short *a, short *b, int c, int d, int e) {
    vftable->DataCallback(this, a, b, c, d, e);
}

// MP3 frame header
struct SMpegAudioFrame {
    int version;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int stereo;
    int size;
    int real_freq;
    int num;
};

// Granule info
struct SGrInfo {
    unsigned int part2_3_length;
    unsigned int big_values;
    unsigned int global_gain;
    unsigned int scalefac_compress;
    unsigned int window_switching_flag;
    unsigned int block_type;
    unsigned int mixed_block_flag;
    unsigned int table_select[3];
    unsigned int subblock_gain[3];
    unsigned int region0_count;
    unsigned int region1_count;
    unsigned int preflag;
    unsigned int scalefac_scale;
    unsigned int count1table_select;
};

// Huffman table entry
struct HuffmanTab {
    int maxval;
    int linbits;
    int ref;
    int tabstart;
    int tabend;
};

// Main MP3 decoder class
struct SMpegAudioDecoder {
    struct ScaleFac {
        int l[22];
        int s[3][13];
    };

    struct SideInfoCh {
        unsigned int scfsi[4];
        SGrInfo gr[2];
    };

    struct SideInfo {
        unsigned int main_data_begin;
        unsigned int private_bits;
        SideInfoCh ch[2];
    };

    SMpegAudioFrame Frame;
    SMpegAudioCallBack *Callback;
    int sfreq;
    SideInfo si;
    ScaleFac scalefac[2];
    int is[578];
    float dq[2][576];
    float lr[2][576];
    float hybridOut[32][18];
    float polyPhaseIn[32];
    short pcmsample[2][18][32];
    SStream *in_file;
    unsigned char in_buf[16384];
    int in_p;
    int in_end;
    int in_bits;
    unsigned int in_bitbuf;
    unsigned char s2_buf[4096];
    int s2_p;
    int s2_end;
    int s2_bits;
    unsigned int s2_bitbuf;
    float sb_buf[2][544];
    int sb_buf_ofs[2];
    float hm_buf[2][32][18];

    SMpegAudioDecoder(SStream *is, SMpegAudioCallBack *callback);
    ~SMpegAudioDecoder();

    int DecodeFrame();

    void HuffmanDecode(int h, int *x, int *y, int *v, int *w);
    void L3_Hybrid(float *fs, float *ts, int sb, int ch, int bt);
    void L3_HybridInitialize();
    void in_close();
    BOOL in_fillbuf(int minsize);
    unsigned int in_getbits(int n);
    void in_open(SStream *is);
    int in_seeksync();
    void s2_dropbits(int n);
    int s2_fillbuf(int size);
    void s2_flushbits();
    unsigned int s2_getbits(int n);
    int s2_getpos();
    void s2_init();
    unsigned int s2_prebits(int n);
    void s2_setpos(int pos);
    void L3_Antialias(int gr, int ch);
    int L3_DecodeFrame();
    void L3_DequantizeSample(int gr, int ch);
    void L3_GetMPEG1ScaleFactors(int gr, int ch);
    void L3_GetMPEG2ScaleFactors(int gr, int ch);
    void L3_GetSideInfo();
    void L3_HuffmanDecode(int gr, int ch, int block_end);
    void L3_Stereo(int gr);
    void SubbandInitialize();
    int SubbandSynthesis(float *bandPtr, int channel, short *samples);
};

// Concert interface for NextTrack callback
#include "iconcert.h"
extern SIConcert *Concert;

// Global data externs
extern int PrecalculateCalled;
extern int layer_names[];
extern int s_freq[];
extern unsigned int huf_codes[];
extern unsigned char huf_values[];
extern unsigned char huf_bits[];
extern HuffmanTab huf_tabs[];

// (formerly dword_58B6E4/EC/F0 — now accessed via huf_tabs[h].linbits/tabstart/tabend)

// MDCT/IMDCT window coefficients
extern float hm_s1m[];
extern float hm_s2m[];
extern float hm_1m[];
extern float hm_2m[];
extern float hm_window[][36];

// Hybrid filter cosine tables
extern int dword_58C518[];
// sfb_index aliases (pointers into sfb_index array)
extern int *dword_58C55C;
extern int *dword_58C560;
extern int *dword_58C5B4;
extern int *dword_58C5B8;
extern int *dword_58C5C0;

// IMDCT coefficients — now accessed via hm_1m[], hm_2m[], hm_s1m[], hm_s2m[], hm_cos[], hm_window[][]
// (formerly individual dword_59E4XX variables, which were elements of these arrays)

// Subband/antialias coefficients — now accessed via cs[], ca[], sb_1m[], sb_2m[], sb_3m[], sb_4m[]
// (formerly individual dword_5B2DXX variables, which were elements of these arrays)

// Antialias coefficients
extern float cs[];
extern float ca[];

// Intensity stereo tables
extern float io[];

// Scale factor band index table (different layout from ScaleFac)
struct ScaleFacIndex {
    int l[23];
    int s[14];
};

// Scale factor band table (subset sizes)
struct ScaleFacBandTable {
    int l[5];
    int s[3];
};

// Scale factor tables
extern int slen[][16];
extern ScaleFacBandTable sfb_table;
extern ScaleFacIndex sfb_index[];
extern int nr_of_sfb_block[][3][4];
extern int reorder_table[];

// Subband synthesis tables
extern float sb_1m[];
extern float sb_2m[];
extern float sb_3m[];
extern float sb_4m[];
// (unk_5B2E28 was at &sb_window[16] in the original binary, now accessed directly)

// Precalculated lookup tables
// qword_5A6F30 is a float* pointing to the middle of a backing array,
// allowing negative indexing for negative quantized sample values
extern float *qword_5A6F30;

// Antialias Ci constants
extern float Ci[];

// Subband synthesis dcwin table and window
extern int dcwin[];
extern float sb_window[];

// IMDCT cosine table
extern float hm_cos[];

// Thread control
extern int StreamThreadShutdown;
extern int StreamThreadRunning;

// Standalone functions
void MpegAudioPrecalculate();
unsigned int __stdcall StreamThreadProc(SMpegAudioDecoder *lpParameter);

#endif // MDEC_DECODE_H
