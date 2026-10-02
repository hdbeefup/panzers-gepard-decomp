// 3dengine/parcel.h
// SParcel, SParcel2 — terrain parcel meshes
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_PARCEL_H
#define DENGINE3_PARCEL_H

#include "mesh.h"

#include <d3d9.h>
#include <d3dx9.h>

struct SParcel : SMesh {
    bool BlendVisible[9];

    SParcel(SGepard *gepard, int xbig, int zbig);
    ~SParcel();
    void DrawLayered(int bottom_level, int *textures);
    void DrawSimple();
    void DrawSketch(int x, int z, int width, int height);
    void DrawWireframe(int index);
    void Update(float *heightmap, unsigned char (*blendmap)[9],
                unsigned int *colormap, D3DXVECTOR3 *normalmap,
                int stride);
};

struct SParcel2 : SMesh {
    unsigned short *ParcelVertices;

    SParcel2(SGepard *gepard, int xbig, int zbig, float *heightmap,
             unsigned char (*blendmap)[9], unsigned int *colormap,
             D3DXVECTOR3 *normalmap, int stride, int bottom_level, int *textures);
    ~SParcel2();
    void Update(unsigned int *colormap, unsigned char *vismap, int stride, int mode);
    void UpdateFullBright();
};

#endif // DENGINE3_PARCEL_H
