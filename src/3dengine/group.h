// 3dengine/group.h
// SGroup — 3D object group
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_GROUP_H
#define DENGINE3_GROUP_H

#include "iobject.h"
#include "chain.h"
#include "darray.h"
#include "string2.h"
#include "igepard.h"  // for SDrawType

#include <d3d9.h>
#include <d3dx9.h>

struct SGepard;
struct SMesh;
struct SShadowMask2;

template <class T>
struct SPtr {
    T *ptr;
};

struct D3DXQUATERNION_s {
    float x;
    float y;
    float z;
    float w;
};

struct SMatrix4 {
    union {
        struct {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
        float m[4][4];
    };

    SMatrix4 operator*(const SMatrix4 &mat) const;
    SMatrix4& operator*=(const SMatrix4 &mat);
};

struct STransformation {
    D3DXVECTOR3 Translation;
    float Scale;
    D3DXQUATERNION Rotation;

    STransformation();
    STransformation(STransformation *trans0, STransformation *trans1, float interpolation);
    void Identity();
    void LogToFile(FILE *logfile);
    SMatrix4* GetMatrix(SMatrix4 *result);
    void GetMatrix(D3DXMATRIX *matrix);
};

struct SColorizeConstants {
    float R;
    float G;
    float B;
    float dummy0;
    float SM;
    float VM;
    float dummy1;
    float dummy2;
};

struct SMeshProp {
    SString MeshName;
    int Parent;
    D3DXMATRIX OriginMatrix;
    D3DXMATRIX WorldMatrix;
    STransformation Transformation;
    STransformation LastTransformation;
    bool HasUVTranslate;
    float UVTranslate;
    float LastUVTranslate;
    bool Visible;
    bool CanCastShadows;
    SPtr<SMesh> Mesh;
    D3DXVECTOR3 Bound[8];
};

struct SShadowMask;
struct SBitmap;

struct SShadowMask2 {
    int Width;
    int Height;
    int Pitch;
    int Size;
    unsigned char *Data;

    SShadowMask2(const SShadowMask *source);
    SShadowMask2(SStream *is);
    ~SShadowMask2();

    void MakeBitmap(SBitmap *destination);
    void Save(SStream *is);
};

struct SGroup : SIObject {
    struct SEffects {
        SEffects* prev;
        SEffects* next;
        void* effect;
    };
    unsigned int RefCount;
    IDirect3DDevice9* lpD3DDev;
    SGepard* Gepard;
    SDArray<SMeshProp> MeshArray;
    STransformation Transformation;
    STransformation LastTransformation;
    bool Recalculate;
    int LastRecalculated;
    SDrawType DrawType;
    bool ambientMode;
    D3DCOLORVALUE ambientColor;
    int Type;
    bool Visible;
    int GepardIndex;
    unsigned int Flags;
    bool IsColorized;
    SColorizeConstants ColorizeConstants;
    float treeBendParams[4];
    float treeBendAmplitude;
    float treeBendFreqX;
    float treeBendFreqZ;
    float treeBendPhaseX;
    float treeBendPhaseZ;
    float treeWobbleParams[4];
    float treeWobbleAmplitude;
    SChain<SEffects> Effects;
    bool NeedNewShadow;
    SShadowMask2* ShadowMask2;
    float ShadowMaskUMin;
    float ShadowMaskUMax;
    float ShadowMaskVMin;
    float ShadowMaskVMax;
    int ShadowTexture;
    SMesh* ShadowMesh;
    int SMX0;
    int SMZ0;
    int SMX1;
    int SMZ1;
    int VisClass;
    int Selection;
    bool Activated;

    // Constructors/destructor
    SGroup(const SGroup *src, bool dynamic, int gepard_index);
    SGroup(SGepard *gepard, SDrawType drawtype);
    ~SGroup();

    // SIObject overrides
    void AddRef() override;
    void Release() override;
    void GetPosition(float *x, float *y, float *z) override;
    void SetPosition(float x, float y, float z) override;
    void SetRotation(float dir, float tiltx, float tiltz) override;
    void SetRotation2(float pitch, float yaw, float roll) override;
    void SetRelativeRotation(float x, float y, float z, float angle) override;
    void SetScale(float scale) override;
    void SetMatrix(D3DXMATRIX m) override;
    void Show(bool visible) override;
    void SaveStatus() override;
    void LogTransforms(FILE *logfile) override;
    int GetMeshIndex(char *name) override;
    int GetMeshIndex2(char *name) override;
    const char* GetMeshName(int index) override;
    SDArray<SString>* GetMeshNames(SDArray<SString> *result) override;
    void SetMeshProperties(int idx, float x, float y, float z, float dir, float tiltx, float tilty) override;
    void SetMeshPosition(int idx, float x, float y, float z) override;
    void SetMeshRotation(int idx, float dir, float tiltx, float tilty) override;
    void SetMeshRotation2(int idx, float steer, float roll) override;
    void SetMeshUVTransformActive(int idx, bool active) override;
    void SetMeshUVTransformValue(int idx, float value) override;
    void GetMeshProperties(int idx, float *x, float *y, float *z, float *hx, float *hy, float *hz) override;
    void GetMeshProperties(int idx, float *x, float *y, float *z) override;
    void GetMeshProperties(int idx, D3DXMATRIX *mat) override;
    void GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z, float *hx, float *hy, float *hz) override;
    void GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z) override;
    void GetMeshInterpolatedProperties(int idx, D3DXMATRIX *mat) override;
    void SetMeshVisible(int idx, bool visible) override;
    void SetMeshCanCastShadows(int idx, bool canCast) override;
    void SetFlags(unsigned int flags) override;
    void ComputeTransformedBounding(float *a, float *b, float *c, float *d, float *e, float *f, float g, float h) override;
    void SetAmbient(float alpha) override;
    void SetColorizeProperties(int a, float b, float c) override;
    void SetTreeParams(float a, float b) override;
    void GetScreenPos(float *x, float *y, float *z) override;
    float GetClickDistanceSquare(float x, float y) override;
    bool CheckClickBox(float a, float b, float c, float d) override;
    void GetInterpolatedPosition(float *x, float *y, float *z) override;
    void SetVisClass(int vis) override;
    void SetSelection(int sel) override;
    void* SetEffect(void *effect) override;
    void ClearEffect(void *effect) override;
    void GetInternalIndex(bool *a, int *b) override;
    bool GetActivationState() override;
    void SetActivationState(bool state) override;

    // Non-virtual methods
    void SetAmbient(D3DCOLORVALUE ambient);
    void ClearShadow(bool keepmask);
    void ComputeProjectedShadowBounding(float *umin, float *umax, float *vmin, float *vmax);
    void ComputeShadowBounding(float *xmin, float *xmax, float *zmin, float *zmax);
    void ComputeVisBounding(float *xmin, float *xmax, float *zmin, float *zmax);
    void Draw();
    void DrawShadow();
    void DrawShadow2();
    void DrawShadowHack();
    void DrawShadowHack2();
    void ForceUpdate();
    SMatrix4* GetMeshProperties(SMatrix4 *result, int idx);
    char Load4DFile(const char *textureprefix, const char *filename, float scale);
    void Precalculate();
    void Replace(const SGroup *src);
};

#endif // DENGINE3_GROUP_H
