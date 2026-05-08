///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx8.h
//  Content:    D3DX utility library
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DX8_H__
#define __D3DX8_H__

// Redirect D3DX8 to D3DX9 (migration from DirectX 8 to DirectX 9)
#include "d3d9.h"
#include "d3dx9.h"
#include <limits.h>

#ifndef D3DXINLINE
#ifdef _MSC_VER
  #if (_MSC_VER >= 1200)
  #define D3DXINLINE __forceinline
  #else
  #define D3DXINLINE __inline
  #endif
#else
  #ifdef __cplusplus
  #define D3DXINLINE inline
  #else
  #define D3DXINLINE
  #endif
#endif
#endif


#define D3DX_DEFAULT ULONG_MAX
#define D3DX_DEFAULT_FLOAT FLT_MAX

// Note: d3dx8math.h, d3dx8core.h, d3dx8tex.h, etc. now redirect to d3dx9 headers
#include "d3dx8math.h"
#include "d3dx8core.h"
#include "d3dx8tex.h"
#include "d3dx8mesh.h"
#include "d3dx8shape.h"
#include "d3dx8effect.h"


#endif //__D3DX8_H__

