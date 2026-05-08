#ifndef DX9_COMPAT_H
#define DX9_COMPAT_H

// DirectX 9 Compatibility Layer for x64 migration
// MUST be included before any d3d8/d3dx8 headers
#define _D3D8_H_              // Block d3d8.h from being included
#define _D3D8TYPES_H_         // Block d3d8types.h
#define _D3D8CAPS_H_          // Block d3d8caps.h
#define __D3DX8_H__           // Block d3dx8.h master header
#define __D3DX8MATH_H__       // Block d3dx8math.h
#define __D3DX8CORE_H__       // Block d3dx8core.h
#define __D3DX8TEX_H__        // Block d3dx8tex.h
#define __D3DX8MESH_H__       // Block d3dx8mesh.h
#define __D3DX8SHAPE_H__      // Block d3dx8shape.h
#define __D3DX8EFFECT_H__     // Block d3dx8effect.h

#include <d3d9.h>
#include "../../../DirectX/Include/d3dx9.h"

#ifndef D3DENUM_NO_WHQL_LEVEL
#define D3DENUM_NO_WHQL_LEVEL 0x00000002L
#endif

#ifndef FullScreen_PresentationInterval
#define FullScreen_PresentationInterval PresentationInterval
#endif

#ifndef D3DSWAPEFFECT_COPY
#define D3DSWAPEFFECT_COPY D3DSWAPEFFECT_DISCARD
#endif

#ifndef D3DSWAPEFFECT_COPY_VSYNC
#define D3DSWAPEFFECT_COPY_VSYNC D3DSWAPEFFECT_COPY
#endif

#ifndef D3DFMT_W11V11U10
#define D3DFMT_W11V11U10 D3DFMT_A16B16G16R16F
#endif

#ifndef D3DTEXF_FLATCUBIC
#define D3DTEXF_FLATCUBIC ((D3DTEXTUREFILTERTYPE)0x7FFFFE01)
#endif

#ifndef D3DTEXF_GAUSSIANCUBIC
#define D3DTEXF_GAUSSIANCUBIC ((D3DTEXTUREFILTERTYPE)0x7FFFFE02)
#endif

#ifndef D3DRS_SOFTWAREVERTEXPROCESSING
#define D3DRS_SOFTWAREVERTEXPROCESSING ((D3DRENDERSTATETYPE)0xFFFFFFFF)
#endif

#ifndef D3DRS_LINEPATTERN
#define D3DRS_LINEPATTERN ((D3DRENDERSTATETYPE)0xFFFFFFFE)
#endif

#ifndef D3DRS_ZVISIBLE
#define D3DRS_ZVISIBLE ((D3DRENDERSTATETYPE)0xFFFFFFFD)
#endif

#ifndef D3DRS_EDGEANTIALIAS
#define D3DRS_EDGEANTIALIAS ((D3DRENDERSTATETYPE)0xFFFFFFFC)
#endif

#ifndef D3DRS_PATCHSEGMENTS
#define D3DRS_PATCHSEGMENTS ((D3DRENDERSTATETYPE)0xFFFFFFFB)
#endif

#ifndef D3DTSS_ADDRESSU
#define D3DTSS_ADDRESSU ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF01)
#endif

#ifndef D3DTSS_ADDRESSV
#define D3DTSS_ADDRESSV ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF02)
#endif

#ifndef D3DTSS_BORDERCOLOR
#define D3DTSS_BORDERCOLOR ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF03)
#endif

#ifndef D3DTSS_MAGFILTER
#define D3DTSS_MAGFILTER ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF04)
#endif

#ifndef D3DTSS_MINFILTER
#define D3DTSS_MINFILTER ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF05)
#endif

#ifndef D3DTSS_MIPFILTER
#define D3DTSS_MIPFILTER ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF06)
#endif

#ifndef D3DTSS_MIPMAPLODBIAS
#define D3DTSS_MIPMAPLODBIAS ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF07)
#endif

#ifndef D3DTSS_MAXMIPLEVEL
#define D3DTSS_MAXMIPLEVEL ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF08)
#endif

#ifndef D3DTSS_MAXANISOTROPY
#define D3DTSS_MAXANISOTROPY ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF09)
#endif

#ifndef D3DTSS_ADDRESSW
#define D3DTSS_ADDRESSW ((D3DTEXTURESTAGESTATETYPE)0x7FFFFF0A)
#endif

#if !defined(WW3D_DX8_INTERFACE_ALIASES)
#define WW3D_DX8_INTERFACE_ALIASES 1
typedef IDirect3DDevice9 IDirect3DDevice8;
typedef IDirect3DVertexBuffer9 IDirect3DVertexBuffer8;
typedef IDirect3DIndexBuffer9 IDirect3DIndexBuffer8;
typedef IDirect3DTexture9 IDirect3DTexture8;
typedef IDirect3DSurface9 IDirect3DSurface8;
typedef IDirect3DCubeTexture9 IDirect3DCubeTexture8;
typedef IDirect3DVolumeTexture9 IDirect3DVolumeTexture8;
typedef IDirect3D9 IDirect3D8;
typedef D3DADAPTER_IDENTIFIER9 D3DADAPTER_IDENTIFIER8;
typedef IDirect3DDevice8 *LPDIRECT3DDEVICE8;
typedef IDirect3DVertexBuffer8 *LPDIRECT3DVERTEXBUFFER8;
typedef IDirect3DIndexBuffer8 *LPDIRECT3DINDEXBUFFER8;
typedef IDirect3DTexture8 *LPDIRECT3DTEXTURE8;
typedef IDirect3DSurface8 *LPDIRECT3DSURFACE8;
#endif

/* BaseType.h already restores the engine bit helpers; only define BitTest if it is still unavailable. */
#ifndef BitTest
#define BitTest( x, i ) ( ( (x) & (i) ) != 0 )
#endif

/* Apply these call-level shims only for D3D9-based builds. */
#if !defined(WW3D_DX9_METHOD_SHIMS)
#define WW3D_DX9_METHOD_SHIMS 1
#define SetStreamSource(StreamNumber, pStreamData, Stride) SetStreamSource(StreamNumber, pStreamData, 0, Stride)
#define SetIndices(pIndexData, BaseVertexIndex) SetIndices(pIndexData)
#define GetAdapterModeCount(Adapter) GetAdapterModeCount(Adapter, D3DFMT_X8R8G8B8)
#define EnumAdapterModes(Adapter, Mode, pMode) EnumAdapterModes(Adapter, D3DFMT_X8R8G8B8, Mode, pMode)
#define CreateImageSurface(Width, Height, Format, ppSurface) CreateOffscreenPlainSurface(Width, Height, Format, D3DPOOL_SYSTEMMEM, ppSurface, NULL)
#define GetFrontBuffer(pDestSurface) GetFrontBufferData(0, pDestSurface)
#define GetRenderTarget(ppRenderTarget) GetRenderTarget(0, ppRenderTarget)
#define CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture) CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, NULL)
#define CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer) CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, NULL)
#define CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer) CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, NULL)

// D3D8-style vertex declaration token shims (only used as placeholder tokens in legacy paths)
#define D3DVSD_STREAM(_stream) 0
#define D3DVSD_REG(_reg, _type) 0
#define D3DVSD_END() 0
#define D3DVSDT_FLOAT1 0
#define D3DVSDT_FLOAT2 0
#define D3DVSDT_FLOAT3 0
#define D3DVSDT_FLOAT4 0
#define D3DVSDT_D3DCOLOR 0
// Shader compatibility
inline HRESULT D3D9_SetVertexShader(IDirect3DDevice9* device, UINT_PTR handle) {
    // On 64-bit Windows, FVF codes are small bitmask values (< 0x10000).
    // Real IDirect3DVertexShader9* pointers are always >= 64 KB.
    if (handle > 0xFFFF) {
        // Compiled vertex shader pointer - clear FVF and activate the shader.
        device->SetFVF(0);
        return device->SetVertexShader(reinterpret_cast<IDirect3DVertexShader9*>(handle));
    }
    // FVF code - D3D8 compatible fixed-function path.
    device->SetVertexShader(NULL);
    return device->SetFVF(static_cast<DWORD>(handle));
}
inline HRESULT D3D9_SetPixelShader(IDirect3DDevice9* device, UINT_PTR handle) {
    return device->SetPixelShader(reinterpret_cast<IDirect3DPixelShader9*>(handle));
}
// Shader constants
inline HRESULT D3D9_SetVertexShaderConstant(IDirect3DDevice9* device, UINT reg, const void* data, UINT count) {
    return device->SetVertexShaderConstantF(reg, static_cast<const float*>(data), count);
}
inline HRESULT D3D9_SetPixelShaderConstant(IDirect3DDevice9* device, UINT reg, const void* data, UINT count) {
    return device->SetPixelShaderConstantF(reg, static_cast<const float*>(data), count);
}
// D3D8-style CreateVertexShader compatibility (4 params: declaration, bytecode, output handle, usage)
// D3D9 only needs bytecode and output handle - we ignore the declaration tokens
inline HRESULT D3D9_CreateVertexShader_Compat(IDirect3DDevice9* device, const DWORD* /*pDeclaration*/, 
                                       const DWORD* pFunction, UINT_PTR* pHandle, DWORD Usage) {
    IDirect3DVertexShader9* pShader = NULL;
    // Call the real D3D9 CreateVertexShader (2 params)
    typedef HRESULT (STDMETHODCALLTYPE IDirect3DDevice9::*CreateVertexShader_t)(CONST DWORD*, IDirect3DVertexShader9**);
    CreateVertexShader_t realMethod = &IDirect3DDevice9::CreateVertexShader;
    HRESULT hr = (device->*realMethod)(pFunction, &pShader);
    if (SUCCEEDED(hr)) {
        *pHandle = reinterpret_cast<UINT_PTR>(pShader);
    }
    return hr;
}
// D3D8-style CreatePixelShader compatibility (2 params: bytecode, output handle)
inline HRESULT D3D9_CreatePixelShader_Compat(IDirect3DDevice9* device, const DWORD* pFunction, UINT_PTR* pHandle) {
    IDirect3DPixelShader9* pShader = NULL;
    // Call the real D3D9 CreatePixelShader
    typedef HRESULT (STDMETHODCALLTYPE IDirect3DDevice9::*CreatePixelShader_t)(CONST DWORD*, IDirect3DPixelShader9**);
    CreatePixelShader_t realMethod = &IDirect3DDevice9::CreatePixelShader;
    HRESULT hr = (device->*realMethod)(pFunction, &pShader);
    if (SUCCEEDED(hr)) {
        *pHandle = reinterpret_cast<UINT_PTR>(pShader);
    }
    return hr;
}
// CopyRects compatibility
// D3D8 CopyRects worked between any pool combination (SCRATCH, SYSTEMMEM, MANAGED, DEFAULT).
// D3D9 StretchRect only works between D3DPOOL_DEFAULT render-target surfaces, so it fails when
// copying from D3DPOOL_SCRATCH (CreateImageSurface) to D3DPOOL_MANAGED texture levels —
// which is exactly the path used by the font atlas in Render2DSentenceClass::Build_Textures.
// D3DXLoadSurfaceFromSurface handles all pool combinations correctly.
inline HRESULT D3D9_CopyRects(IDirect3DDevice9* /*device*/, IDirect3DSurface9* pSrc, CONST RECT* pSrcRects, UINT cRects, IDirect3DSurface9* pDst, CONST POINT* pDstPoints) {
    if (cRects == 0 || pSrcRects == NULL) {
        return D3DXLoadSurfaceFromSurface(pDst, NULL, NULL, pSrc, NULL, NULL, D3DX_FILTER_NONE, 0);
    }
    for (UINT i = 0; i < cRects; i++) {
        RECT dstRect = pSrcRects[i];
        if (pDstPoints) {
            dstRect.left = pDstPoints[i].x; dstRect.top = pDstPoints[i].y;
            dstRect.right = dstRect.left + (pSrcRects[i].right - pSrcRects[i].left);
            dstRect.bottom = dstRect.top + (pSrcRects[i].bottom - pSrcRects[i].top);
        }
        HRESULT hr = D3DXLoadSurfaceFromSurface(pDst, NULL, &dstRect, pSrc, NULL, &pSrcRects[i], D3DX_FILTER_NONE, 0);
        if (FAILED(hr)) return hr;
    }
    return D3D_OK;
}

inline const float* D3D9_ShaderConstPtr(const void* data) {
    return static_cast<const float*>(data);
}

template <typename T>
inline const float* D3D9_ShaderConstPtr(const T& data) {
    return reinterpret_cast<const float*>(&data);
}

#define SetVertexShader(VertexShader) SetVertexShader(reinterpret_cast<IDirect3DVertexShader9*>(static_cast<UINT_PTR>(VertexShader)))
#define SetPixelShader(PixelShader) SetPixelShader(reinterpret_cast<IDirect3DPixelShader9*>(static_cast<UINT_PTR>(PixelShader)))
#define SetVertexShaderConstant(Register, Data, Count) SetVertexShaderConstantF(Register, D3D9_ShaderConstPtr(Data), Count)
#define SetPixelShaderConstant(Register, Data, Count) SetPixelShaderConstantF(Register, D3D9_ShaderConstPtr(Data), Count)
inline HRESULT D3D9_DeleteVertexShader(IDirect3DDevice9*, UINT_PTR handle) {
    if (handle) {
        IDirect3DVertexShader9* shader = reinterpret_cast<IDirect3DVertexShader9*>(handle);
        shader->Release();
    }
    return D3D_OK;
}
inline HRESULT D3D9_DeletePixelShader(IDirect3DDevice9*, UINT_PTR handle) {
    if (handle) {
        IDirect3DPixelShader9* shader = reinterpret_cast<IDirect3DPixelShader9*>(handle);
        shader->Release();
    }
    return D3D_OK;
}
#endif
// D3DRS_ZBIAS compatibility
#define D3DRS_ZBIAS D3DRS_DEPTHBIAS

/* D3DX8 → D3DX9 compatibility mappings (math types and utilities) */
/* d3dx9 is included above via the repository-local DirectX include path. */

// Common D3DX8 function redirects to D3DX9
#define D3DXLoadSurfaceFromSurface(pDestSurface, pDestPalette, pDestRect, pSrcSurface, pSrcPalette, pSrcRect, dwFilter, dwColorKey) \
    D3DXLoadSurfaceFromSurface(pDestSurface, pDestPalette, pDestRect, pSrcSurface, pSrcPalette, pSrcRect, dwFilter, dwColorKey)
#define D3DXSaveSurfaceToFileA D3DXSaveSurfaceToFileA
#define D3DXLoadSurfaceFromFileA D3DXLoadSurfaceFromFileA
#define D3DXGetImageInfoFromFileA D3DXGetImageInfoFromFileA

#endif
