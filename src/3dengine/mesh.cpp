// 3dengine/mesh.cpp
// Mesh rendering
// Decompiled from: gameSplit/smesh.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>

#include "mesh.h"
#include "gepard.h"
#include "terrain.h"
#include "logger.h"

// Classes: SMesh
// Function count: 15

//----- (0044C7D0) --------------------------------------------------------

SMesh::SMesh(SGepard *gepard, unsigned int vertex_type)
{
    this->RefCount = 1;
    this->Gepard = gepard;
    this->lpD3DDev = gepard->lpD3DDev;
    this->lpVertexBuffer = 0;
    this->lpVertices = 0;
    this->NumVertices = 0;
    this->VertexFormat = 2;
    this->OffsetXYZ = 0;
    this->VertexSize = 12;
    this->OffsetNormal = -1;
    this->OffsetDiffuse = -1;
    this->OffsetSpecular = -1;
    this->OffsetTexture1 = -1;

    unsigned int v4 = 2;
    int v5 = 12;
    unsigned int v3 = vertex_type;

    if ( (vertex_type & 0x10) != 0 )
    {
        v4 = 18;
        this->OffsetNormal = 12;
        v3 = vertex_type & 0xFFFFFFEF;
        this->VertexFormat = 18;
        this->VertexSize = 24;
        v5 = 24;
    }
    if ( (v3 & 0x40) != 0 )
    {
        v4 |= 0x40u;
        this->OffsetDiffuse = v5;
        v3 &= ~0x40u;
        this->VertexFormat = v4;
        v5 += 4;
        this->VertexSize = v5;
    }
    if ( (v3 & 0x80u) != 0 )
    {
        v4 |= 0x80u;
        this->OffsetSpecular = v5;
        v3 &= ~0x80u;
        this->VertexFormat = v4;
        v5 += 4;
        this->VertexSize = v5;
    }
    if ( (v3 & 0xF) == 1 )
    {
        v4 |= 0x100u;
        this->OffsetTexture1 = v5;
        v3 &= 0xFFFFFFF0;
        this->VertexFormat = v4;
        v5 += 8;
        this->VertexSize = v5;
    }
    if ( (v3 & 0xF) == 2 )
    {
        this->OffsetTexture1 = v5;
        v3 &= 0xFFFFFFF0;
        this->OffsetTexture2 = v5 + 8;
        v4 |= 0x200u;
        v5 += 16;
        this->VertexFormat = v4;
        this->VertexSize = v5;
    }
    if ( (v3 & 0xF) == 3 )
    {
        this->OffsetTexture1 = v5;
        this->OffsetTexture2 = v5 + 8;
        this->VertexFormat = v4 | 0x300;
        v3 &= 0xFFFFFFF0;
        this->OffsetTexture3 = v5 + 16;
        this->VertexSize = v5 + 24;
    }
    if ( v3 )
        Logger.g->Panic("Fatal error in SMesh constructor: invalid vertex type! Exiting...");

    this->lpIndexBuffer[0] = 0;
    this->lpIndices[0] = 0;
    this->NumIndices[0] = 0;
    this->lpIndexBuffer[1] = 0;
    this->lpIndices[1] = 0;
    this->NumIndices[1] = 0;
    this->lpIndexBuffer[2] = 0;
    this->lpIndices[2] = 0;
    this->NumIndices[2] = 0;
    this->lpMaterials = 0;
    this->NumMaterials = 0;
    this->VertexAABBMin = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    this->VertexAABBMax = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
    this->VertexAABBValid = 0;
}

//----- (0044C940) --------------------------------------------------------

SMesh::~SMesh()
{
    if ( this->RefCount != 0 )
        Logger.g->Panic("SMesh::~SMesh: Object deleted instead of release");
    if ( this->lpVertices )
        Logger.g->Panic("SMesh::~SMesh: Locked mesh is deleted");

    if ( this->lpVertexBuffer )
    {
        this->lpVertexBuffer->Release();
        this->lpVertexBuffer = 0;
    }

    for ( int i = 0; i < 3; ++i )
    {
        if ( this->lpIndexBuffer[i] )
        {
            this->lpIndexBuffer[i]->Release();
            this->lpIndexBuffer[i] = 0;
        }
    }

    SMeshMaterial *lpMaterials = this->lpMaterials;
    if ( lpMaterials )
    {
        for ( unsigned int i = 0; i < this->NumMaterials; ++i )
        {
            this->Gepard->ReleaseTexture(this->lpMaterials[i].TextureIndex, 0);
            this->Gepard->ReleaseTexture(this->lpMaterials[i].TextureIndex2, 0);
        }
        delete lpMaterials;
    }
}

//----- (0044CB60) --------------------------------------------------------

void SMesh::AddRef()
{
    ++this->RefCount;
}

//----- (0044CB70) --------------------------------------------------------

unsigned short *SMesh::CreateIndexBuffer(int which, int numindices)
{
    char atmstr[200];
    this->NumIndices[which] = numindices;
    HRESULT v4 = this->lpD3DDev->CreateIndexBuffer(
        2 * numindices,
        8u,
        D3DFMT_INDEX16,
        D3DPOOL_MANAGED,
        &this->lpIndexBuffer[which],
        0);
    if ( v4 )
    {
        const char *v6 = DXGetErrorStringA(v4);
        sprintf(atmstr, "%s: %s", "SMesh::CreateIndexBuffer\\IDirect3DDevice9::CreateIndexBuffer", v6);
        Logger.g->Panic(atmstr);
    }
    this->lpIndexBuffer[which]->Lock(0, 0, (void **)&this->lpIndices[which], 0);
    memset(this->lpIndices[which], 0, 2 * this->NumIndices[which]);
    return this->lpIndices[which];
}

//----- (0044CC30) --------------------------------------------------------

SMeshMaterial *SMesh::CreateMaterialBuffer(int nummaterials)
{
    this->NumMaterials = nummaterials;
    SMeshMaterial *v4 = (SMeshMaterial *)operator new[](sizeof(SMeshMaterial) * nummaterials);
    this->lpMaterials = v4;
    memset(v4, 0, sizeof(SMeshMaterial) * this->NumMaterials);
    for ( int i = 0; i < nummaterials; ++i )
    {
        this->lpMaterials[i].TextureIndex = -1;
        this->lpMaterials[i].TextureIndex2 = -1;
    }
    return this->lpMaterials;
}

//----- (0044CCB0) --------------------------------------------------------

void SMesh::CreateVertexBuffer(unsigned int numvertices)
{
    char atmstr[200];
    unsigned int VertexFormat = this->VertexFormat;
    int bufSize = numvertices * this->VertexSize;
    this->NumVertices = numvertices;
    HRESULT v6 = this->lpD3DDev->CreateVertexBuffer(bufSize, 8u, VertexFormat, D3DPOOL_MANAGED, &this->lpVertexBuffer, 0);
    if ( v6 )
    {
        const char *v7 = DXGetErrorStringA(v6);
        sprintf(atmstr, "%s: %s", "SMesh::CreateVertexBuffer\\IDirect3DDevice9::CreateVertexBuffer", v7);
        Logger.g->Panic(atmstr);
    }
    this->lpVertexBuffer->Lock(0, 0, &this->lpVertices, 0);
    memset(this->lpVertices, 0, this->NumVertices * this->VertexSize);
}

//----- (0044CD60) --------------------------------------------------------

void SMesh::Draw(int drawtype)
{
    this->lpD3DDev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
    this->lpD3DDev->SetFVF(this->VertexFormat);
    unsigned int NumVertices = this->NumVertices;
    if ( NumVertices >= 2 )
        this->lpD3DDev->DrawPrimitive(D3DPT_LINESTRIP, 0, NumVertices - 1);
}

//----- (0044CDB0) --------------------------------------------------------

void SMesh::Draw(unsigned int drawtype, int index, float uOffset)
{
    IDirect3DDevice9 *dev = this->lpD3DDev;
    dev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
    dev->SetIndices(this->lpIndexBuffer[index]);
    dev->SetFVF(this->VertexFormat);

    if ( (drawtype & 2) == 0 )
    {
        if ( (drawtype & 8) != 0 )
        {
            dev->SetTextureStageState(0, D3DTSS_COLOROP, 3u);
            unsigned int startIdx = 0;
            for ( unsigned int i = 0; i < this->NumMaterials; ++i )
            {
                this->Gepard->SetTexture(0, this->lpMaterials[i].TextureIndex, 1);
                dev->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST, 0,
                    this->lpMaterials[i].StartVertex,
                    this->lpMaterials[i].NumVertices,
                    startIdx,
                    this->lpMaterials[i].NumFaces);
                this->Gepard->PolyCount += this->lpMaterials[i].NumFaces;
                startIdx += this->lpMaterials[i].NumFaces * 3;
            }
            dev->SetTextureStageState(0, D3DTSS_COLOROP, 4u);
            return;
        }
        if ( (drawtype & 0x10) != 0 )
        {
            dev->SetTextureStageState(0, D3DTSS_COLOROP, 3u);
            unsigned int startIdx = 0;
            for ( unsigned int i = 0; i < this->NumMaterials; ++i )
            {
                this->Gepard->SetTexture(0, this->lpMaterials[i].TextureIndex, 1);
                int TextureAlpha = this->Gepard->GetTextureAlpha(this->lpMaterials[i].TextureIndex);
                if ( TextureAlpha != 2 )
                {
                    int v25 = (int)(TextureAlpha == 1) | 2;
                    if ( (drawtype & 0x20) == 0 )
                        v25 = TextureAlpha == 1;
                    int v26 = v25 | 4;
                    if ( (drawtype & 0x40) == 0 )
                        v26 = v25;
                    dev->SetVertexShader(this->Gepard->depthWriteVertexShaders[v26].Ptr);
                    dev->SetPixelShader(this->Gepard->depthWritePixelShaders[v26].Ptr);
                    dev->DrawIndexedPrimitive(
                        D3DPT_TRIANGLELIST, 0,
                        this->lpMaterials[i].StartVertex,
                        this->lpMaterials[i].NumVertices,
                        startIdx,
                        this->lpMaterials[i].NumFaces);
                    dev->SetVertexShader(0);
                    dev->SetPixelShader(0);
                    this->Gepard->PolyCount += this->lpMaterials[i].NumFaces;
                    startIdx += this->lpMaterials[i].NumFaces * 3;
                }
            }
            dev->SetTextureStageState(0, D3DTSS_COLOROP, 4u);
            return;
        }
        if ( (drawtype & 4) == 0 )
        {
            unsigned int v36 = this->NumIndices[index];
            unsigned int NumVertices = this->NumVertices;
            if ( (drawtype & 1) != 0 )
            {
                dev->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, NumVertices, 0, v36 >> 1);
            }
            else
            {
                dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, NumVertices, 0, v36 / 3);
                this->Gepard->PolyCount += this->NumIndices[index] / 3;
            }
            return;
        }

        // drawtype & 4: alpha blended materials
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
        dev->SetRenderState(D3DRS_SRCBLEND, 5u);
        dev->SetRenderState(D3DRS_DESTBLEND, 6u);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
        dev->SetRenderState(D3DRS_ALPHAREF, 0);
        dev->SetRenderState(D3DRS_ALPHAFUNC, 6u);
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, 3u);
        unsigned int startIdx = 0;
        for ( unsigned int i = 0; i < this->NumMaterials; ++i )
        {
            SMeshMaterial *mat = &this->lpMaterials[i];
            int Flags = mat->Flags;
            if ( Flags == 0 )
            {
                this->Gepard->SetTexture(0, mat->TextureIndex, 0);
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
                dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
                dev->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
            }
            else if ( Flags == 1 )
            {
                this->Gepard->SetTexture(0, mat->TextureIndex, 0);
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
                dev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
                dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            else if ( Flags == 2 )
            {
                this->Gepard->SetTexture(0, mat->TextureIndex, 1);
                dev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2u);
                dev->SetSamplerState(0, D3DSAMP_ADDRESSU, 3u);
                dev->SetSamplerState(0, D3DSAMP_ADDRESSV, 3u);
                if ( this->Gepard->IsTextureOpaque(mat->TextureIndex) )
                    dev->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
                else
                    dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }

            dev->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST, 0,
                mat->StartVertex, mat->NumVertices,
                startIdx, mat->NumFaces);
            this->Gepard->PolyCount += mat->NumFaces;
            startIdx += mat->NumFaces * 3;
        }
        dev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2u);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSU, 1u);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSV, 1u);
        return;
    }

    // drawtype & 2: standard material rendering with shaders
    D3DXMATRIX EnvMatrix;
    memset(&EnvMatrix, 0, sizeof(EnvMatrix));
    EnvMatrix._11 = 0.5f;
    EnvMatrix._22 = -0.5f;
    EnvMatrix._33 = 1.0f;
    EnvMatrix._44 = 1.0f;
    EnvMatrix._41 = 0.5f;
    EnvMatrix._42 = 0.5f;

    bool needSSAA = (drawtype & 0x20) != 0;
    bool hasBump = (drawtype & 0x40) != 0;
    unsigned int startIdx = 0;
    for ( unsigned int i = 0; i < this->NumMaterials; ++i )
    {
        SMeshMaterial *mat = &this->lpMaterials[i];
        int TextureAlpha = this->Gepard->GetTextureAlpha(mat->TextureIndex);
        int matFlags = mat->Flags;
        bool didSSAA = false;
        int shaderIdx;

        if ( (matFlags & 2) != 0 )
        {
            dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
            this->Gepard->SetTexture(0, mat->TextureIndex, 1);
            dev->SetTextureStageState(1u, D3DTSS_TEXCOORDINDEX, 0x10000u);
            dev->SetTextureStageState(1u, D3DTSS_TEXTURETRANSFORMFLAGS, 2u);
            dev->SetTransform(D3DTS_TEXTURE1, &EnvMatrix);
            this->Gepard->SetTexture(1u, mat->TextureIndex2, 0);
            dev->SetTextureStageState(1u, D3DTSS_COLOROP, 7u);
            shaderIdx = 2;
        }
        else if ( (matFlags & 4) != 0 )
        {
            dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
            this->Gepard->SetTexture(0, mat->TextureIndex, 1);
            dev->SetTextureStageState(1u, D3DTSS_TEXCOORDINDEX, 0);
            dev->SetTextureStageState(1u, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
            this->Gepard->SetTexture(1u, mat->TextureIndex2, 0);
            dev->SetTextureStageState(1u, D3DTSS_COLOROP, 7u);
            shaderIdx = 1;
        }
        else
        {
            dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
            int TextureIndex = mat->TextureIndex;
            if ( TextureIndex < 0 )
                TextureIndex = this->Gepard->_gray_texture;
            this->Gepard->SetTexture(0, TextureIndex, 1);
            dev->SetTextureStageState(1u, D3DTSS_COLOROP, 1u);
            if ( TextureAlpha == 1 )
            {
                if ( this->Gepard->PresentationParameters.MultiSampleType )
                {
                    this->Gepard->SetSSAA(1);
                    didSSAA = true;
                }
            }
            shaderIdx = 0;
            if ( TextureAlpha == 3 )
                shaderIdx = 3;
        }

        if ( needSSAA )
        {
            shaderIdx = 4 + 2 * (hasBump ? 1 : 0);
        }
        else if ( hasBump )
        {
            shaderIdx = 5;
        }

        dev->SetVertexShaderConstantF(21u, &uOffset, 1u);
        dev->SetVertexShader(this->Gepard->standardVertexShaders[shaderIdx].Ptr);
        dev->SetPixelShader(this->Gepard->standardPixelShaders[shaderIdx].Ptr);
        dev->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST, 0,
            mat->StartVertex, mat->NumVertices,
            startIdx, mat->NumFaces);
        if ( didSSAA )
            this->Gepard->SetSSAA(0);
        dev->SetVertexShader(0);
        dev->SetPixelShader(0);
        this->Gepard->PolyCount += mat->NumFaces;
        startIdx += mat->NumFaces * 3;
    }

    dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
    dev->SetTextureStageState(1u, D3DTSS_TEXCOORDINDEX, 0);
    dev->SetTextureStageState(1u, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
    dev->SetTextureStageState(1u, D3DTSS_COLOROP, 1u);
}

//----- (0044D610) --------------------------------------------------------

unsigned int SMesh::GetNumVertices()
{
    return this->NumVertices;
}

//----- (0044D620) --------------------------------------------------------

void SMesh::LockIndexBuffer(int which)
{
    this->lpIndexBuffer[which]->Lock(0, 0, (void **)&this->lpIndices[which], 0);
}

//----- (0044D650) --------------------------------------------------------

void SMesh::LockVertexBuffer()
{
    this->lpVertexBuffer->Lock(0, 0, &this->lpVertices, 0);
}

//----- (0044D670) --------------------------------------------------------

void SMesh::Release()
{
    if ( --this->RefCount == 0 )
        delete this;
}

//----- (0044D680) --------------------------------------------------------

void SMesh::UnlockIndexBuffer(int which)
{
    this->lpIndexBuffer[which]->Unlock();
    this->lpIndices[which] = 0;
}

//----- (0044D6B0) --------------------------------------------------------

void SMesh::UnlockVertexBuffer()
{
    this->lpVertexBuffer->Unlock();
    this->lpVertices = 0;
}
