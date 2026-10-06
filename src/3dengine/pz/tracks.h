// src/3dengine/pz/tracks.h
// HD animation tracks (keyframe controllers) of .4d SSQS sequences and .anim
// (CANM) files. OWNER: agent A.
//
// RTTI: SITrackPosition / SITrackRotation bases; STrackPositionBezier
// (vftable 0x80cd70), STrackPositionIndependent (0x80cda0), STrackPositionPacked
// (0x80cdac), STrackRotationTCB (0x80cd88), STrackRotationEuler (0x80cd94),
// STrackRotationEulerPacked (0x80cdb8). Slot +0x00 is the scalar deleting
// dtor, slot +0x04 evaluates the track at a time (seconds).
//
// Out-of-range-type ("ORT") bytes after each key count: 0 = hold the end key,
// 1/2 = repeat (loop), 3 = ping-pong. They select how a time before the first
// or after the last key is folded back into the key range.

#ifndef PZ_TRACKS_H
#define PZ_TRACKS_H

#include "pzcommon.h"

struct SStream;

namespace pz {

// Float key track (HD 0x10 bytes: count, ortBefore, ortAfter, keys). Keys are
// 16 bytes {time, value, inTangent, outTangent} (CZBX/CZBY/CZBZ sub-
// controllers, CFOV, LINT). Loader 0x671100, evaluator 0x6746d0 (Hermite).
struct SKeyTrackFloat {
    int    Count;      // +0x00
    int    OrtBefore;  // +0x04
    int    OrtAfter;   // +0x08
    float* Keys;       // +0x0c  Count * 4 floats

    void  Load(SStream* is);       // 0x671100
    float Evaluate(float t) const; // 0x6746d0
    ~SKeyTrackFloat();
};
static_assert(sizeof(SKeyTrackFloat) == 0x10, "HD operator new(0x10)");

// Boolean key track (VISI; HD 0x10: count, ortBefore, ortAfter, keys of 8
// bytes {time, value}). Loader 0x671090, evaluator 0x674520.
struct STrackBool {
    int    Count;
    int    OrtBefore;
    int    OrtAfter;
    float* Keys;       // Count * {time, float value}

    void Load(SStream* is);        // 0x671090
    bool Evaluate(float t) const;  // 0x674520
    ~STrackBool();
};
static_assert(sizeof(STrackBool) == 0x10, "HD operator new(0x10)");

// Link track (CLNK; HD 8 bytes: count, keys of 0x54 bytes {3x4 matrix,
// position, quaternion, time +0x4c, node +0x50}). Loader 0x6711c0, lookup
// 0x674960.
struct STrackLink {
    int            Count;
    unsigned char* Keys;

    void Load(SStream* is);                                  // 0x6711c0
    void Evaluate(float t, int* node, float* channel) const; // 0x674960 (channel: 0x4c bytes)
    ~STrackLink();
};
static_assert(sizeof(STrackLink) == 8, "HD operator new(8)");

// Camera-change track (CCHG; HD 8 bytes: count, 8-byte keys). 0x671170.
struct STrackCameraChange {
    int    Count;
    float* Keys;

    void Load(SStream* is);  // 0x671170
    ~STrackCameraChange();
};
static_assert(sizeof(STrackCameraChange) == 8, "HD operator new(8)");

// ---- position tracks (SITrackPosition) ----
struct SITrackPosition {
    virtual ~SITrackPosition() {}
    virtual float* Evaluate(float* out3, float t) = 0;   // +0x04
};

struct STrackPositionBezier : SITrackPosition {          // CPOS, 0x14 bytes
    int    Count;      // +0x04
    int    OrtBefore;  // +0x08
    int    OrtAfter;   // +0x0c
    float* Keys;       // +0x10  Count * 10 floats {t, v3, in3, out3}

    explicit STrackPositionBezier(SStream* is);          // 0x6712a0
    ~STrackPositionBezier() override;                    // 0x672690
    float* Evaluate(float* out3, float t) override;      // 0x674e50
};

struct STrackPositionIndependent : SITrackPosition {     // CXYZ, 0x10 bytes
    SKeyTrackFloat* X;    // +0x04 CZBX
    SKeyTrackFloat* Y;    // +0x08 CZBY
    SKeyTrackFloat* Z;    // +0x0c CZBZ

    explicit STrackPositionIndependent(SStream* is);     // 0x671420
    ~STrackPositionIndependent() override;               // 0x6726e0
    float* Evaluate(float* out3, float t) override;      // 0x675170
};

struct STrackPositionPacked : SITrackPosition {          // CPSP, 0x20 bytes
    int    Count;      // +0x04
    float  Start;      // +0x08
    float  End;        // +0x0c
    float  Rate;       // +0x10 (Count - 1) / (End - Start)
    int    OrtBefore;  // +0x14
    int    OrtAfter;   // +0x18
    float* Keys;       // +0x1c Count * vec3

    explicit STrackPositionPacked(SStream* is);          // 0x6715b0
    ~STrackPositionPacked() override;                    // 0x672710
    float* Evaluate(float* out3, float t) override;      // 0x675220
};

// ---- rotation tracks (SITrackRotation): quaternion {x, y, z, w} ----
struct SITrackRotation {
    virtual ~SITrackRotation() {}
    virtual float* Evaluate(float* out4, float t) = 0;   // +0x04
};

struct STrackRotationTCB : SITrackRotation {             // CROT, 0x14 bytes
    int    Count;      // +0x04
    int    OrtBefore;  // +0x08
    int    OrtAfter;   // +0x0c
    float* Keys;       // +0x10 Count * 0x58 bytes (22 floats)

    explicit STrackRotationTCB(SStream* is);             // 0x671980
    ~STrackRotationTCB() override;                       // 0x6727e0
    float* Evaluate(float* out4, float t) override;      // 0x675d40
};

struct STrackRotationEuler : SITrackRotation {           // CEUL, 0x10 bytes
    SKeyTrackFloat* X;
    SKeyTrackFloat* Y;
    SKeyTrackFloat* Z;

    explicit STrackRotationEuler(SStream* is);           // 0x6716d0
    ~STrackRotationEuler() override;                     // 0x672760
    float* Evaluate(float* out4, float t) override;      // 0x675470
};

struct STrackRotationEulerPacked : SITrackRotation {     // CEUP, 0x20 bytes
    int    Count;
    float  Start;
    float  End;
    float  Rate;
    int    OrtBefore;
    int    OrtAfter;
    float* Keys;       // Count * euler vec3 (radians)

    explicit STrackRotationEulerPacked(SStream* is);     // 0x671860
    ~STrackRotationEulerPacked() override;               // 0x672790
    float* Evaluate(float* out4, float t) override;      // 0x675710
};

// Shared maths of the track evaluators.
float* QuatSlerp(float* out4, const float* a4, const float* b4, float t);   // 0x676560
float* QuatFromEuler(float* out4, float x, float y, float z);              // 0x672c90

} // namespace pz

#endif // PZ_TRACKS_H
