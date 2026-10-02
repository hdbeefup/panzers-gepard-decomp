// 3dengine/iobject.h
// SIObject — base interface for 3D objects
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_IOBJECT_H
#define DENGINE3_IOBJECT_H

#include <cstdio>
#include <d3dx9.h>

struct SString;
template <class T> struct SDArray;

// SIObject — abstract base for renderable objects
// Vtable reconstructed from PDB SIObject_vtbl (46 slots, exact order)
struct SIObject {
    virtual void AddRef() = 0;                                                                  // 0
    virtual void Release() = 0;                                                                 // 1
    virtual void GetPosition(float *x, float *y, float *z) = 0;                                // 2
    virtual void SetPosition(float x, float y, float z) = 0;                                   // 3
    virtual void SetRotation(float xrot, float yrot, float zrot) = 0;                          // 4
    virtual void SetRotation2(float xrot, float yrot, float zrot) = 0;                         // 5
    virtual void SetRelativeRotation(float a, float b, float c, float d) = 0;                  // 6
    virtual void SetScale(float scale) = 0;                                                     // 7
    virtual void SetMatrix(D3DXMATRIX mat) = 0;                                                 // 8
    virtual void Show(bool visible) = 0;                                                        // 9
    virtual void SaveStatus() = 0;                                                              // 10
    virtual void LogTransforms(FILE *f) = 0;                                                    // 11
    virtual int GetMeshIndex(char *meshName) = 0;                                               // 12
    virtual int GetMeshIndex2(char *meshName) = 0;                                              // 13
    virtual const char* GetMeshName(int index) = 0;                                             // 14
    virtual SDArray<SString>* GetMeshNames(SDArray<SString> *result) = 0;                       // 15
    virtual void SetMeshProperties(int idx, float a, float b, float c, float d, float e, float f) = 0; // 16
    virtual void SetMeshPosition(int idx, float x, float y, float z) = 0;                      // 17
    virtual void SetMeshRotation(int idx, float x, float y, float z) = 0;                      // 18
    virtual void SetMeshRotation2(int idx, float x, float y) = 0;                              // 19
    virtual void SetMeshUVTransformActive(int idx, bool active) = 0;                            // 20
    virtual void SetMeshUVTransformValue(int idx, float value) = 0;                             // 21
    virtual void GetMeshProperties(int idx, float *x, float *y, float *z, float *hx, float *hy, float *hz) = 0; // 22
    virtual void GetMeshProperties(int idx, float *x, float *y, float *z) = 0;                 // 23
    virtual void GetMeshProperties(int idx, D3DXMATRIX *mat) = 0;                               // 24
    virtual void GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z, float *hx, float *hy, float *hz) = 0; // 25
    virtual void GetMeshInterpolatedProperties(int idx, float *x, float *y, float *z) = 0;     // 26
    virtual void GetMeshInterpolatedProperties(int idx, D3DXMATRIX *mat) = 0;                   // 27
    virtual void SetMeshVisible(int idx, bool visible) = 0;                                     // 28
    virtual void SetMeshCanCastShadows(int idx, bool canCast) = 0;                              // 29
    virtual void SetFlags(unsigned int flags) = 0;                                              // 30
    virtual void ComputeTransformedBounding(float *a, float *b, float *c, float *d, float *e, float *f, float g, float h) = 0; // 31
    virtual void SetAmbient(float alpha) = 0;                                                   // 32
    virtual void SetColorizeProperties(int a, float b, float c) = 0;                           // 33
    virtual void SetTreeParams(float a, float b) = 0;                                          // 34
    virtual void GetScreenPos(float *x, float *y, float *z) = 0;                               // 35
    virtual float GetClickDistanceSquare(float x, float y) = 0;                                // 36
    virtual bool CheckClickBox(float a, float b, float c, float d) = 0;                        // 37
    virtual void GetInterpolatedPosition(float *x, float *y, float *z) = 0;                    // 38
    virtual void SetVisClass(int vis) = 0;                                                      // 39
    virtual void SetSelection(int sel) = 0;                                                     // 40
    virtual void* SetEffect(void *effect) = 0;                                                  // 41
    virtual void ClearEffect(void *effect) = 0;                                                 // 42
    virtual void GetInternalIndex(bool *a, int *b) = 0;                                         // 43
    virtual bool GetActivationState() = 0;                                                      // 44
    virtual void SetActivationState(bool state) = 0;                                            // 45
};

#endif // DENGINE3_IOBJECT_H
