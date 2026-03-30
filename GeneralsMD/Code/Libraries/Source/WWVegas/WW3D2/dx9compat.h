#ifndef DX9_COMPAT_H
#define DX9_COMPAT_H
// DirectX 9 Compatibility Layer for x64 migration
#include <d3d9.h>
// Shader compatibility
inline HRESULT D3D9_SetVertexShader(IDirect3DDevice9* device, DWORD handle) {
    return device->SetVertexShader(reinterpret_cast<IDirect3DVertexShader9*>(static_cast<UINT_PTR>(handle)));
}
inline HRESULT D3D9_SetPixelShader(IDirect3DDevice9* device, DWORD handle) {
    return device->SetPixelShader(reinterpret_cast<IDirect3DPixelShader9*>(static_cast<UINT_PTR>(handle)));
}
// Shader constants
inline HRESULT D3D9_SetVertexShaderConstant(IDirect3DDevice9* device, UINT reg, const void* data, UINT count) {
    return device->SetVertexShaderConstantF(reg, static_cast<const float*>(data), count);
}
inline HRESULT D3D9_SetPixelShaderConstant(IDirect3DDevice9* device, UINT reg, const void* data, UINT count) {
    return device->SetPixelShaderConstantF(reg, static_cast<const float*>(data), count);
}
// CopyRects compatibility
inline HRESULT D3D9_CopyRects(IDirect3DDevice9* device, IDirect3DSurface9* pSrc, CONST RECT* pSrcRects, UINT cRects, IDirect3DSurface9* pDst, CONST POINT* pDstPoints) {
    if (cRects == 0 || pSrcRects == NULL) return device->StretchRect(pSrc, NULL, pDst, NULL, D3DTEXF_NONE);
    for (UINT i = 0; i < cRects; i++) {
        RECT dstRect = pSrcRects[i];
        if (pDstPoints) { 
            dstRect.left = pDstPoints[i].x; dstRect.top = pDstPoints[i].y;
            dstRect.right = dstRect.left + (pSrcRects[i].right - pSrcRects[i].left);
            dstRect.bottom = dstRect.top + (pSrcRects[i].bottom - pSrcRects[i].top);
        }
        HRESULT hr = device->StretchRect(pSrc, &pSrcRects[i], pDst, &dstRect, D3DTEXF_NONE);
        if (FAILED(hr)) return hr;
    }
    return D3D_OK;
}
// D3DRS_ZBIAS compatibility
#define D3DRS_ZBIAS D3DRS_DEPTHBIAS
// Adapter identifier
typedef D3DADAPTER_IDENTIFIER9 D3DADAPTER_IDENTIFIER8;
#endif
