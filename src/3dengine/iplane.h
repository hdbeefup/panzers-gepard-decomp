// 3dengine/iplane.h
// SIPlane — base interface for terrain/plane
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_IPLANE_H
#define DENGINE3_IPLANE_H

struct STile;

// SIPlane — abstract base for terrain planes
// Vtable reconstructed from PDB SIPlane_vtbl (16 slots, exact order)
struct SIPlane {
    virtual void Acquire(float **heightMap, float **oldHeightMap, unsigned char (**blendMap)[9], STile **tileMap, unsigned char **waterMap) = 0; // 0
    virtual void InitTileVariations(const char *tileset) = 0;           // 1
    virtual void InitTileset2() = 0;                                     // 2
    virtual void ReleaseTileset() = 0;                                   // 3
    virtual void LoadSketchTexture(const char *filename) = 0;           // 4
    virtual void SetViewType(int viewType) = 0;                         // 5
    virtual void ComputeShadows() = 0;                                   // 6
    virtual void UpdateVisMap(unsigned char *a, unsigned char *b, int c) = 0; // 7
    virtual void Update(int x0, int z0, int x1, int z1) = 0;           // 8
    virtual void UpdateColorMap(int x0, int z0, int x1, int z1) = 0;   // 9
    virtual void EnableGodMode(bool enable) = 0;                         // 10
    virtual void SetCompactMode(bool compact) = 0;                      // 11
    virtual bool GetCompactMode() = 0;                                   // 12
    virtual void SetBlockMapAddress(unsigned char *a, unsigned char *b) = 0; // 13
    virtual void SetBlockMapMode(bool mode) = 0;                        // 14
    virtual bool GetBlockMapMode() = 0;                                  // 15
};

#endif // DENGINE3_IPLANE_H
