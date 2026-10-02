// 3dengine/terrain.h
// STerrain — terrain rendering
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_TERRAIN_H
#define DENGINE3_TERRAIN_H

#include "iplane.h"
#include "mesh.h"
#include "string2.h"

#include <d3d9.h>
#include <d3dx9.h>

struct SGepard;
struct SParcel;
struct SParcel2;

struct STile {
    unsigned char Level;
    unsigned char Variation;
};

struct SParcelInfo {
    SParcel* Mesh;
    SParcel2* Mesh2;
    SMesh* BlockMapMesh;
    bool Visible;
    float LowPoint;
};

struct SMaterial : D3DMATERIAL9 {
    SMaterial();
    void SetDiffuse(float r, float g, float b, float a) {
        Diffuse.r = r; Diffuse.g = g; Diffuse.b = b; Diffuse.a = a;
    }
    void SetAmbient(float r, float g, float b, float a) {
        Ambient.r = r; Ambient.g = g; Ambient.b = b; Ambient.a = a;
    }
    void SetEmissive(float r, float g, float b, float a) {
        Emissive.r = r; Emissive.g = g; Emissive.b = b; Emissive.a = a;
    }
    void SetSpecular(float r, float g, float b, float a) {
        Specular.r = r; Specular.g = g; Specular.b = b; Specular.a = a;
    }
    void SetSpecular(float r, float g, float b, float a, float power) {
        Specular.r = r; Specular.g = g; Specular.b = b; Specular.a = a; Power = power;
    }
    void GetAmbient(float *r, float *g, float *b) {
        *r = Ambient.r; *g = Ambient.g; *b = Ambient.b;
    }
    void GetDiffuse(float *r, float *g, float *b) {
        *r = Diffuse.r; *g = Diffuse.g; *b = Diffuse.b;
    }
    void GetEmissive(float *r, float *g, float *b) {
        *r = Emissive.r; *g = Emissive.g; *b = Emissive.b;
    }
    void GetSpecular(float *r, float *g, float *b) {
        *r = Specular.r; *g = Specular.g; *b = Specular.b;
    }
};

struct STerrain : SIPlane {
    IDirect3DDevice9* lpD3DDev;
    SGepard* Gepard;
    int XSize;
    int ZSize;
    int Stride;
    int NumTiles;
    int NumVertices;
    float* HeightMap;
    float* OldHeightMap;
    unsigned char (*BlendMap)[9];
    unsigned int* ColorMap;
    D3DXVECTOR3* NormalMap;
    unsigned char* ShadowMap;
    unsigned char* WaterMap;
    int XParcels;
    int ZParcels;
    int NumParcels;
    SParcelInfo* Parcels;
    STile* TileMap;
    int TileTextures[10];
    int Variations[10];
    SString TilePrefix;
    int ViewType;
    D3DXMATRIX IdentityMatrix;
    D3DXMATRIX WireframeLiftupMatrix;
    SMaterial WireframeMaterial;
    SMaterial Wireframe2Material;
    int SketchTexture;
    bool CompactMode;
    SMesh* LimitMesh;
    int LimitTexture;
    unsigned char* VisMap;
    unsigned char* VisMap2;
    int FogMode;
    bool GodMode;
    bool BlockMapMode;
    unsigned char* BlockMap;
    unsigned char* CurrentBlockMap;

    // Constructor / destructor
    STerrain(SGepard *gepard, int xsize, int zsize);
    ~STerrain();

    // Non-virtual methods
    void Acquire(float **heightmap, float **oldheightmap, unsigned char (**blendmap)[9],
                 STile **tilemap, unsigned char **watermap);
    bool IsParcelVisible(float x, float z);
    double GetHeight(int x, int z);
    double GetHeight(float x, float z);
    double GetHeightTriangular(float x, float z);
    double GetOldHeight(int x, int z);
    double GetOldHeight(float x, float z);
    char IsVisible(float x, float z, int visclass);
    void CalculateVisibleParcels(D3DXMATRIX *CameraMatrix, D3DXMATRIX *ProjectionMatrix);
    void ComputeShadows();
    void Draw();
    void DrawLighting(int a2);
    void DrawLimits();
    void DrawShadow();
    void DrawShadow2();
    void EnableGodMode(bool enable);
    bool GetBlockMapMode();
    bool GetCompactMode();
    D3DXVECTOR3 *GetEdgeNormal(D3DXVECTOR3 *result, float x, float z);
    D3DXVECTOR3 *GetNormal(D3DXVECTOR3 *result, int x, int z);
    void InitLimits();
    void InitTileVariations(const char *tileset);
    void InitTileset2();
    void LoadSketchTexture(const char *filename);
    void RaiseHeight(int x, int z, float height);
    void ReleaseTileset();
    void SetBlockMapAddress(unsigned char *blockmap, unsigned char *currentblockmap);
    void SetBlockMapMode(bool enable);
    void SetCompactMode(bool enable);
    void SetViewType(int view_type);
    void Update(int x0, int z0, int x1, int z1);
    void UpdateBlockMap(int parcel);
    void UpdateColorMap(int x0, int z0, int x1, int z1);
    void UpdateLimits();
    void UpdateVisMap(unsigned char *vismap, unsigned char *vismap2, int fogmode);
};

#endif // DENGINE3_TERRAIN_H
