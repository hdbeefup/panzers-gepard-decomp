// 3dengine/mesh.h
// SMesh — 3D mesh class
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_MESH_H
#define DENGINE3_MESH_H

#include <d3d9.h>
#include <d3dx9.h>

struct SGepard;

struct SMeshMaterial {
    int TextureIndex;
    int TextureIndex2;
    int NumFaces;
    int StartVertex;
    int NumVertices;
    int Flags;
};

struct SMesh {
    virtual ~SMesh();
    virtual void AddRef();
    virtual void Release();

    unsigned int RefCount;
    IDirect3DDevice9* lpD3DDev;
    SGepard* Gepard;
    IDirect3DVertexBuffer9* lpVertexBuffer;
    void* lpVertices;
    unsigned int NumVertices;
    unsigned int VertexFormat;
    int VertexSize;
    int OffsetXYZ;
    int OffsetNormal;
    int OffsetDiffuse;
    int OffsetSpecular;
    int OffsetTexture1;
    int OffsetTexture2;
    int OffsetTexture3;
    IDirect3DIndexBuffer9* lpIndexBuffer[3];
    unsigned short* lpIndices[3];
    unsigned int NumIndices[3];
    SMeshMaterial* lpMaterials;
    unsigned int NumMaterials;

    // True position-component AABB walked from the locked vertex buffer at
    // load time (group.cpp:LoadNew4DFile). Used by the editor object picker
    // to project a tight screen rect that matches what gets drawn, instead
    // of the often-much-smaller authored Bound[8] OBB on SMeshProp.
    D3DXVECTOR3 VertexAABBMin;
    D3DXVECTOR3 VertexAABBMax;
    unsigned int VertexAABBValid;   // 0/1; uint for natural alignment

    // Default for derived classes (SParcel/SParcel2) that init manually.
    // Zero VertexAABBValid so the picker doesn't read garbage AABB fields.
    SMesh() : VertexAABBMin(0, 0, 0), VertexAABBMax(0, 0, 0), VertexAABBValid(0) {}
    SMesh(SGepard* gepard, unsigned int vertex_type);
    unsigned short* CreateIndexBuffer(int which, int numindices);
    SMeshMaterial* CreateMaterialBuffer(int nummaterials);
    void CreateVertexBuffer(unsigned int numvertices);
    void Draw(int drawtype);
    void Draw(unsigned int drawtype, int index, float uOffset);
    unsigned int GetNumVertices();
    void LockIndexBuffer(int which);
    void LockVertexBuffer();
    void UnlockIndexBuffer(int which);
    void UnlockVertexBuffer();
};

#endif // DENGINE3_MESH_H
