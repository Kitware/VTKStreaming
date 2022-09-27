/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCUDADriverAPI.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * This header contains types for functions, enums, datatypes and structs.
 * It allows for limited CUDA-OpenGL interop with support for mapping an
 * OpenGL texture -> CUarray.
 *
 * We're interested in a tiny subset of the CUDA Driver API, as we do not allocate
 * memory through CUDA interface. That's a major chunk of functions we do
 * not need.
 */

#ifndef vtkCUDADriverAPI_h
#define vtkCUDADriverAPI_h

#include "stddef.h"

#define CUDA_VERSION 7050

#if defined(_WIN32) || defined(__CYGWIN__)
#define CUDAAPI __stdcall
#else
#define CUDAAPI
#endif

typedef int CUdevice;

#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64) || defined(__LP64__) ||               \
  defined(__aarch64__)
typedef unsigned long long CUdeviceptr;
#else
typedef unsigned int CUdeviceptr;
#endif
typedef unsigned long long CUtexObject;

typedef struct CUarray_st* CUarray;
typedef struct CUctx_st* CUcontext;
typedef struct CUstream_st* CUstream;
typedef struct CUgraphicsResource_st* CUgraphicsResource;

typedef enum cudaError_enum
{
  CUDA_SUCCESS = 0,
  CUDA_ERROR_NOT_READY = 600
} CUresult;

typedef enum CUgraphicsRegisterFlags_enum
{
  CU_GRAPHICS_REGISTER_FLAGS_NONE = 0,
  CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY = 1,
  CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD = 2,
  CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST = 4,
  CU_GRAPHICS_REGISTER_FLAGS_TEXTURE_GATHER = 8
} CUgraphicsRegisterFlags;

typedef enum CUgraphicsMapResourceFlags_enum
{
  CU_GRAPHICS_MAP_RESOURCE_FLAGS_NONE = 0,
  CU_GRAPHICS_MAP_RESOURCE_FLAGS_READ_ONLY = 1,
  CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD = 2
} CUgraphicsMapResourceFlags;

typedef enum CUGLDeviceList_enum
{
  CU_GL_DEVICE_LIST_ALL = 1,
  CU_GL_DEVICE_LIST_CURRENT_FRAME = 2,
  CU_GL_DEVICE_LIST_NEXT_FRAME = 3,
} CUGLDeviceList;

typedef unsigned int GLenum;
typedef unsigned int GLuint;

#define CU_STREAM_NON_BLOCKING 1
#define CU_EVENT_BLOCKING_SYNC 1
#define CU_EVENT_DISABLE_TIMING 2
#define CU_TRSF_READ_AS_INTEGER 1

// TFN -> Type function
// clang-format off
typedef CUresult CUDAAPI TFN_cuInit(unsigned int Flags);

typedef CUresult CUDAAPI TFN_cuDeviceGet(CUdevice* device, int ordinal);
typedef CUresult CUDAAPI TFN_cuDeviceGetCount(int* count);
typedef CUresult CUDAAPI TFN_cuDeviceGetName(char* name, int len, CUdevice dev);

typedef CUresult CUDAAPI TFN_cuCtxCreate_v2(CUcontext* pctx, unsigned int flags, CUdevice dev);
typedef CUresult CUDAAPI TFN_cuCtxGetApiVersion(CUcontext ctx, unsigned int* version);
typedef CUresult CUDAAPI TFN_cuCtxGetDevice(CUdevice* device);
typedef CUresult CUDAAPI TFN_cuCtxPushCurrent_v2(CUcontext pctx);
typedef CUresult CUDAAPI TFN_cuCtxPopCurrent_v2(CUcontext* pctx);
typedef CUresult CUDAAPI TFN_cuCtxDestroy_v2(CUcontext ctx);

typedef CUresult CUDAAPI TFN_cuGetErrorName(CUresult error, const char** pstr);
typedef CUresult CUDAAPI TFN_cuGetErrorString(CUresult error, const char** pstr);

typedef CUresult CUDAAPI TFN_cuGLGetDevices_v2(unsigned int* pCudaDeviceCount, CUdevice* pCudaDevices, unsigned int cudaDeviceCount, CUGLDeviceList deviceList);

typedef CUresult CUDAAPI TFN_cuGraphicsGLRegisterImage(CUgraphicsResource* pCudaResource, GLuint image, GLenum target, unsigned int Flags);
typedef CUresult CUDAAPI TFN_cuGraphicsUnregisterResource(CUgraphicsResource resource);
typedef CUresult CUDAAPI TFN_cuGraphicsMapResources(unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI TFN_cuGraphicsUnmapResources(unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI TFN_cuGraphicsResourceSetMapFlags(CUgraphicsResource resource, unsigned int  flags);
typedef CUresult CUDAAPI TFN_cuGraphicsSubResourceGetMappedArray(CUarray* pArray, CUgraphicsResource resource, unsigned int arrayIndex, unsigned int mipLevel);
// clang-format on

struct CUDRVFunctions
{
  TFN_cuInit* cuInit;

  TFN_cuDeviceGet* cuDeviceGet;
  TFN_cuDeviceGetCount* cuDeviceGetCount;
  TFN_cuDeviceGetName* cuDeviceGetName;

  TFN_cuCtxCreate_v2* cuCtxCreate_v2;
  TFN_cuCtxGetApiVersion* cuCtxGetApiVersion;
  TFN_cuCtxGetDevice* cuCtxGetDevice;
  TFN_cuCtxPushCurrent_v2* cuCtxPushCurrent_v2;
  TFN_cuCtxPopCurrent_v2* cuCtxPopCurrent_v2;
  TFN_cuCtxDestroy_v2* cuCtxDestroy_v2;

  TFN_cuGetErrorName* cuGetErrorName;
  TFN_cuGetErrorString* cuGetErrorString;

  TFN_cuGLGetDevices_v2* cuGLGetDevices_v2;

  TFN_cuGraphicsGLRegisterImage* cuGraphicsGLRegisterImage;
  TFN_cuGraphicsUnregisterResource* cuGraphicsUnregisterResource;
  TFN_cuGraphicsMapResources* cuGraphicsMapResources;
  TFN_cuGraphicsUnmapResources* cuGraphicsUnmapResources;
  TFN_cuGraphicsResourceSetMapFlags* cuGraphicsResourceSetMapFlags;
  TFN_cuGraphicsSubResourceGetMappedArray* cuGraphicsSubResourceGetMappedArray;
};

#endif
