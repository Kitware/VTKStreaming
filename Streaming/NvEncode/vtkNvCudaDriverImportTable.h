/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvCudaDriverImportTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkNvCudaDriverImportTable_h
#define vtkNvCudaDriverImportTable_h

#include "vtkNvDynamicLoader.h"
#include "vtkStreamingNvEncodeModule.h"

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
typedef struct CUevent_st* CUevent;
typedef struct CUfunc_st* CUfunction;
typedef struct CUmod_st* CUmodule;
typedef struct CUmipmappedArray_st* CUmipmappedArray;
typedef struct CUgraphicsResource_st* CUgraphicsResource;
typedef struct CUextMemory_st* CUexternalMemory;
typedef struct CUextSemaphore_st* CUexternalSemaphore;

typedef struct CUlinkState_st* CUlinkState;

typedef enum cudaError_enum
{
  CUDA_SUCCESS = 0,
  CUDA_ERROR_NOT_READY = 600
} CUresult;

/**
 * Device properties (subset)
 */
typedef enum CUdevice_attribute_enum
{
  CU_DEVICE_ATTRIBUTE_CLOCK_RATE = 13,
  CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT = 14,
  CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = 16,
  CU_DEVICE_ATTRIBUTE_INTEGRATED = 18,
  CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY = 19,
  CU_DEVICE_ATTRIBUTE_COMPUTE_MODE = 20,
  CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS = 31,
  CU_DEVICE_ATTRIBUTE_PCI_BUS_ID = 33,
  CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 34,
  CU_DEVICE_ATTRIBUTE_TCC_DRIVER = 35,
  CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE = 36,
  CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH = 37,
  CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT = 40,
  CU_DEVICE_ATTRIBUTE_UNIFIED_ADDRESSING = 41,
  CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID = 50,
  CU_DEVICE_ATTRIBUTE_TEXTURE_PITCH_ALIGNMENT = 51,
  CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 75,
  CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 76,
  CU_DEVICE_ATTRIBUTE_MANAGED_MEMORY = 83,
  CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD = 84,
  CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD_GROUP_ID = 85,
} CUdevice_attribute;

typedef enum CUarray_format_enum
{
  CU_AD_FORMAT_UNSIGNED_INT8 = 0x01,
  CU_AD_FORMAT_UNSIGNED_INT16 = 0x02,
  CU_AD_FORMAT_UNSIGNED_INT32 = 0x03,
  CU_AD_FORMAT_SIGNED_INT8 = 0x08,
  CU_AD_FORMAT_SIGNED_INT16 = 0x09,
  CU_AD_FORMAT_SIGNED_INT32 = 0x0a,
  CU_AD_FORMAT_HALF = 0x10,
  CU_AD_FORMAT_FLOAT = 0x20
} CUarray_format;

typedef enum CUmemorytype_enum
{
  CU_MEMORYTYPE_HOST = 1,
  CU_MEMORYTYPE_DEVICE = 2,
  CU_MEMORYTYPE_ARRAY = 3
} CUmemorytype;

typedef enum CUlimit_enum
{
  CU_LIMIT_STACK_SIZE = 0,
  CU_LIMIT_PRINTF_FIFO_SIZE = 1,
  CU_LIMIT_MALLOC_HEAP_SIZE = 2,
  CU_LIMIT_DEV_RUNTIME_SYNC_DEPTH = 3,
  CU_LIMIT_DEV_RUNTIME_PENDING_LAUNCH_COUNT = 4
} CUlimit;

typedef enum CUresourcetype_enum
{
  CU_RESOURCE_TYPE_ARRAY = 0x00,
  CU_RESOURCE_TYPE_MIPMAPPED_ARRAY = 0x01,
  CU_RESOURCE_TYPE_LINEAR = 0x02,
  CU_RESOURCE_TYPE_PITCH2D = 0x03
} CUresourcetype;

typedef enum CUaddress_mode_enum
{
  CU_TR_ADDRESS_MODE_WRAP = 0,
  CU_TR_ADDRESS_MODE_CLAMP = 1,
  CU_TR_ADDRESS_MODE_MIRROR = 2,
  CU_TR_ADDRESS_MODE_BORDER = 3
} CUaddress_mode;

typedef enum CUfilter_mode_enum
{
  CU_TR_FILTER_MODE_POINT = 0,
  CU_TR_FILTER_MODE_LINEAR = 1
} CUfilter_mode;

typedef enum CUgraphicsRegisterFlags_enum
{
  CU_GRAPHICS_REGISTER_FLAGS_NONE = 0,
  CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY = 1,
  CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD = 2,
  CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST = 4,
  CU_GRAPHICS_REGISTER_FLAGS_TEXTURE_GATHER = 8
} CUgraphicsRegisterFlags;

typedef enum CUexternalMemoryHandleType_enum
{
  CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD = 1,
  CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32 = 2,
  CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT = 3,
  CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP = 4,
  CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE = 5,
} CUexternalMemoryHandleType;

typedef enum CUexternalSemaphoreHandleType_enum
{
  CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD = 1,
  CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32 = 2,
  CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT = 3,
  CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE = 4
} CUexternalSemaphoreHandleType;

typedef enum CUjit_option_enum
{
  CU_JIT_MAX_REGISTERS = 0,
  CU_JIT_THREADS_PER_BLOCK = 1,
  CU_JIT_WALL_TIME = 2,
  CU_JIT_INFO_LOG_BUFFER = 3,
  CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES = 4,
  CU_JIT_ERROR_LOG_BUFFER = 5,
  CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES = 6,
  CU_JIT_OPTIMIZATION_LEVEL = 7,
  CU_JIT_TARGET_FROM_CUCONTEXT = 8,
  CU_JIT_TARGET = 9,
  CU_JIT_FALLBACK_STRATEGY = 10,
  CU_JIT_GENERATE_DEBUG_INFO = 11,
  CU_JIT_LOG_VERBOSE = 12,
  CU_JIT_GENERATE_LINE_INFO = 13,
  CU_JIT_CACHE_MODE = 14,
  CU_JIT_NEW_SM3X_OPT = 15,
  CU_JIT_FAST_COMPILE = 16,
  CU_JIT_GLOBAL_SYMBOL_NAMES = 17,
  CU_JIT_GLOBAL_SYMBOL_ADDRESSES = 18,
  CU_JIT_GLOBAL_SYMBOL_COUNT = 19,
  CU_JIT_NUM_OPTIONS
} CUjit_option;

typedef enum CUjitInputType_enum
{
  CU_JIT_INPUT_CUBIN = 0,
  CU_JIT_INPUT_PTX = 1,
  CU_JIT_INPUT_FATBINARY = 2,
  CU_JIT_INPUT_OBJECT = 3,
  CU_JIT_INPUT_LIBRARY = 4,
  CU_JIT_NUM_INPUT_TYPES
} CUjitInputType;

#ifndef CU_UUID_HAS_BEEN_DEFINED
#define CU_UUID_HAS_BEEN_DEFINED
typedef struct CUuuid_st
{
  char bytes[16];
} CUuuid;
#endif

typedef struct CUDA_MEMCPY2D_st
{
  size_t srcXInBytes;
  size_t srcY;
  CUmemorytype srcMemoryType;
  const void* srcHost;
  CUdeviceptr srcDevice;
  CUarray srcArray;
  size_t srcPitch;

  size_t dstXInBytes;
  size_t dstY;
  CUmemorytype dstMemoryType;
  void* dstHost;
  CUdeviceptr dstDevice;
  CUarray dstArray;
  size_t dstPitch;

  size_t WidthInBytes;
  size_t Height;
} CUDA_MEMCPY2D;

typedef struct CUDA_RESOURCE_DESC_st
{
  CUresourcetype resType;
  union
  {
    struct
    {
      CUarray hArray;
    } array;
    struct
    {
      CUmipmappedArray hMipmappedArray;
    } mipmap;
    struct
    {
      CUdeviceptr devPtr;
      CUarray_format format;
      unsigned int numChannels;
      size_t sizeInBytes;
    } linear;
    struct
    {
      CUdeviceptr devPtr;
      CUarray_format format;
      unsigned int numChannels;
      size_t width;
      size_t height;
      size_t pitchInBytes;
    } pitch2D;
    struct
    {
      int reserved[32];
    } reserved;
  } res;
  unsigned int flags;
} CUDA_RESOURCE_DESC;

typedef struct CUDA_TEXTURE_DESC_st
{
  CUaddress_mode addressMode[3];
  CUfilter_mode filterMode;
  unsigned int flags;
  unsigned int maxAnisotropy;
  CUfilter_mode mipmapFilterMode;
  float mipmapLevelBias;
  float minMipmapLevelClamp;
  float maxMipmapLevelClamp;
  float borderColor[4];
  int reserved[12];
} CUDA_TEXTURE_DESC;

/* Unused type */
typedef struct CUDA_RESOURCE_VIEW_DESC_st CUDA_RESOURCE_VIEW_DESC;

typedef unsigned int GLenum;
typedef unsigned int GLuint;

typedef enum CUGLDeviceList_enum
{
  CU_GL_DEVICE_LIST_ALL = 1,
  CU_GL_DEVICE_LIST_CURRENT_FRAME = 2,
  CU_GL_DEVICE_LIST_NEXT_FRAME = 3,
} CUGLDeviceList;

typedef struct CUDA_EXTERNAL_MEMORY_HANDLE_DESC_st
{
  CUexternalMemoryHandleType type;
  union
  {
    int fd;
    struct
    {
      void* handle;
      const void* name;
    } win32;
  } handle;
  unsigned long long size;
  unsigned int flags;
  unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_HANDLE_DESC;

typedef struct CUDA_EXTERNAL_MEMORY_BUFFER_DESC_st
{
  unsigned long long offset;
  unsigned long long size;
  unsigned int flags;
  unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_BUFFER_DESC;

typedef struct CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC_st
{
  CUexternalSemaphoreHandleType type;
  union
  {
    int fd;
    struct
    {
      void* handle;
      const void* name;
    } win32;
  } handle;
  unsigned int flags;
  unsigned int reserved[16];
} CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC;

typedef struct CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_st
{
  struct
  {
    struct
    {
      unsigned long long value;
    } fence;
    unsigned int reserved[16];
  } params;
  unsigned int flags;
  unsigned int reserved[16];
} CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS;

typedef CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS;

typedef struct CUDA_ARRAY3D_DESCRIPTOR_st
{
  size_t Width;
  size_t Height;
  size_t Depth;

  CUarray_format Format;
  unsigned int NumChannels;
  unsigned int Flags;
} CUDA_ARRAY3D_DESCRIPTOR;

typedef struct CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC_st
{
  unsigned long long offset;
  CUDA_ARRAY3D_DESCRIPTOR arrayDesc;
  unsigned int numLevels;
  unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC;

#define CU_STREAM_NON_BLOCKING 1
#define CU_EVENT_BLOCKING_SYNC 1
#define CU_EVENT_DISABLE_TIMING 2
#define CU_TRSF_READ_AS_INTEGER 1

typedef void CUDAAPI CUstreamCallback(CUstream hStream, CUresult status, void* userdata);

typedef CUresult CUDAAPI PFN_cuInit(unsigned int Flags);
typedef CUresult CUDAAPI PFN_cuDeviceGetCount(int* count);
typedef CUresult CUDAAPI PFN_cuDeviceGet(CUdevice* device, int ordinal);
typedef CUresult CUDAAPI PFN_cuDeviceGetAttribute(int* pi, CUdevice_attribute attrib, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuDeviceGetName(char* name, int len, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuDeviceGetUuid(CUuuid* uuid, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuDeviceComputeCapability(int* major, int* minor, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuCtxCreate(CUcontext* pctx, unsigned int flags, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuCtxSetLimit(CUlimit limit, size_t value);
typedef CUresult CUDAAPI PFN_cuCtxPushCurrent(CUcontext pctx);
typedef CUresult CUDAAPI PFN_cuCtxPopCurrent(CUcontext* pctx);
typedef CUresult CUDAAPI PFN_cuCtxDestroy(CUcontext ctx);
typedef CUresult CUDAAPI PFN_cuMemAlloc(CUdeviceptr* dptr, size_t bytesize);
typedef CUresult CUDAAPI PFN_cuMemAllocPitch(CUdeviceptr* dptr, size_t* pPitch, size_t WidthInBytes,
  size_t Height, unsigned int ElementSizeBytes);
typedef CUresult CUDAAPI PFN_cuMemAllocManaged(
  CUdeviceptr* dptr, size_t bytesize, unsigned int flags);
typedef CUresult CUDAAPI PFN_cuMemsetD8Async(
  CUdeviceptr dstDevice, unsigned char uc, size_t N, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuMemFree(CUdeviceptr dptr);
typedef CUresult CUDAAPI PFN_cuMemcpy(CUdeviceptr dst, CUdeviceptr src, size_t bytesize);
typedef CUresult CUDAAPI PFN_cuMemcpyAsync(
  CUdeviceptr dst, CUdeviceptr src, size_t bytesize, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuMemcpy2D(const CUDA_MEMCPY2D* pcopy);
typedef CUresult CUDAAPI PFN_cuMemcpy2DAsync(const CUDA_MEMCPY2D* pcopy, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuMemcpyHtoD(
  CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);
typedef CUresult CUDAAPI PFN_cuMemcpyHtoDAsync(
  CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuMemcpyDtoH(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult CUDAAPI PFN_cuMemcpyDtoHAsync(
  void* dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuMemcpyDtoD(
  CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult CUDAAPI PFN_cuMemcpyDtoDAsync(
  CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuGetErrorName(CUresult error, const char** pstr);
typedef CUresult CUDAAPI PFN_cuGetErrorString(CUresult error, const char** pstr);
typedef CUresult CUDAAPI PFN_cuCtxGetDevice(CUdevice* device);

typedef CUresult CUDAAPI PFN_cuDevicePrimaryCtxRetain(CUcontext* pctx, CUdevice dev);
typedef CUresult CUDAAPI PFN_cuDevicePrimaryCtxRelease(CUdevice dev);
typedef CUresult CUDAAPI PFN_cuDevicePrimaryCtxSetFlags(CUdevice dev, unsigned int flags);
typedef CUresult CUDAAPI PFN_cuDevicePrimaryCtxGetState(
  CUdevice dev, unsigned int* flags, int* active);
typedef CUresult CUDAAPI PFN_cuDevicePrimaryCtxReset(CUdevice dev);

typedef CUresult CUDAAPI PFN_cuStreamCreate(CUstream* phStream, unsigned int flags);
typedef CUresult CUDAAPI PFN_cuStreamQuery(CUstream hStream);
typedef CUresult CUDAAPI PFN_cuStreamSynchronize(CUstream hStream);
typedef CUresult CUDAAPI PFN_cuStreamDestroy(CUstream hStream);
typedef CUresult CUDAAPI PFN_cuStreamAddCallback(
  CUstream hStream, CUstreamCallback* callback, void* userdata, unsigned int flags);
typedef CUresult CUDAAPI PFN_cuEventCreate(CUevent* phEvent, unsigned int flags);
typedef CUresult CUDAAPI PFN_cuEventDestroy(CUevent hEvent);
typedef CUresult CUDAAPI PFN_cuEventSynchronize(CUevent hEvent);
typedef CUresult CUDAAPI PFN_cuEventQuery(CUevent hEvent);
typedef CUresult CUDAAPI PFN_cuEventRecord(CUevent hEvent, CUstream hStream);

typedef CUresult CUDAAPI PFN_cuLaunchKernel(CUfunction f, unsigned int gridDimX,
  unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY,
  unsigned int blockDimZ, unsigned int sharedMemBytes, CUstream hStream, void** kernelParams,
  void** extra);
typedef CUresult CUDAAPI PFN_cuLinkCreate(
  unsigned int numOptions, CUjit_option* options, void** optionValues, CUlinkState* stateOut);
typedef CUresult CUDAAPI PFN_cuLinkAddData(CUlinkState state, CUjitInputType type, void* data,
  size_t size, const char* name, unsigned int numOptions, CUjit_option* options,
  void** optionValues);
typedef CUresult CUDAAPI PFN_cuLinkComplete(CUlinkState state, void** cubinOut, size_t* sizeOut);
typedef CUresult CUDAAPI PFN_cuLinkDestroy(CUlinkState state);
typedef CUresult CUDAAPI PFN_cuModuleLoadData(CUmodule* module, const void* image);
typedef CUresult CUDAAPI PFN_cuModuleUnload(CUmodule hmod);
typedef CUresult CUDAAPI PFN_cuModuleGetFunction(
  CUfunction* hfunc, CUmodule hmod, const char* name);
typedef CUresult CUDAAPI PFN_cuModuleGetGlobal(
  CUdeviceptr* dptr, size_t* bytes, CUmodule hmod, const char* name);
typedef CUresult CUDAAPI PFN_cuTexObjectCreate(CUtexObject* pTexObject,
  const CUDA_RESOURCE_DESC* pResDesc, const CUDA_TEXTURE_DESC* pTexDesc,
  const CUDA_RESOURCE_VIEW_DESC* pResViewDesc);
typedef CUresult CUDAAPI PFN_cuTexObjectDestroy(CUtexObject texObject);

typedef CUresult CUDAAPI PFN_cuGLGetDevices(unsigned int* pCudaDeviceCount, CUdevice* pCudaDevices,
  unsigned int cudaDeviceCount, CUGLDeviceList deviceList);
typedef CUresult CUDAAPI PFN_cuGraphicsGLRegisterImage(
  CUgraphicsResource* pCudaResource, GLuint image, GLenum target, unsigned int Flags);
typedef CUresult CUDAAPI PFN_cuGraphicsUnregisterResource(CUgraphicsResource resource);
typedef CUresult CUDAAPI PFN_cuGraphicsMapResources(
  unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuGraphicsUnmapResources(
  unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI PFN_cuGraphicsSubResourceGetMappedArray(
  CUarray* pArray, CUgraphicsResource resource, unsigned int arrayIndex, unsigned int mipLevel);

typedef CUresult CUDAAPI PFN_cuImportExternalMemory(
  CUexternalMemory* extMem_out, const CUDA_EXTERNAL_MEMORY_HANDLE_DESC* memHandleDesc);
typedef CUresult CUDAAPI PFN_cuDestroyExternalMemory(CUexternalMemory extMem);
typedef CUresult CUDAAPI PFN_cuExternalMemoryGetMappedBuffer(
  CUdeviceptr* devPtr, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_BUFFER_DESC* bufferDesc);
typedef CUresult CUDAAPI PFN_cuExternalMemoryGetMappedMipmappedArray(CUmipmappedArray* mipmap,
  CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC* mipmapDesc);
typedef CUresult CUDAAPI PFN_cuMipmappedArrayGetLevel(
  CUarray* pLevelArray, CUmipmappedArray hMipmappedArray, unsigned int level);
typedef CUresult CUDAAPI PFN_cuMipmappedArrayDestroy(CUmipmappedArray hMipmappedArray);

typedef CUresult CUDAAPI PFN_cuImportExternalSemaphore(
  CUexternalSemaphore* extSem_out, const CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC* semHandleDesc);
typedef CUresult CUDAAPI PFN_cuDestroyExternalSemaphore(CUexternalSemaphore extSem);
typedef CUresult CUDAAPI PFN_cuSignalExternalSemaphoresAsync(const CUexternalSemaphore* extSemArray,
  const CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS* paramsArray, unsigned int numExtSems,
  CUstream stream);
typedef CUresult CUDAAPI PFN_cuWaitExternalSemaphoresAsync(const CUexternalSemaphore* extSemArray,
  const CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS* paramsArray, unsigned int numExtSems, CUstream stream);

struct CudaDriverFunctionsList
{
  PFN_cuInit* cuInit;
  PFN_cuDeviceGetCount* cuDeviceGetCount;
  PFN_cuDeviceGet* cuDeviceGet;
  PFN_cuDeviceGetAttribute* cuDeviceGetAttribute;
  PFN_cuDeviceGetName* cuDeviceGetName;
  PFN_cuDeviceGetUuid* cuDeviceGetUuid;
  PFN_cuDeviceComputeCapability* cuDeviceComputeCapability;
  PFN_cuCtxCreate* cuCtxCreate;
  PFN_cuCtxSetLimit* cuCtxSetLimit;
  PFN_cuCtxPushCurrent* cuCtxPushCurrent;
  PFN_cuCtxPopCurrent* cuCtxPopCurrent;
  PFN_cuCtxDestroy* cuCtxDestroy;
  PFN_cuMemAlloc* cuMemAlloc;
  PFN_cuMemAllocPitch* cuMemAllocPitch;
  PFN_cuMemAllocManaged* cuMemAllocManaged;
  PFN_cuMemsetD8Async* cuMemsetD8Async;
  PFN_cuMemFree* cuMemFree;
  PFN_cuMemcpy* cuMemcpy;
  PFN_cuMemcpyAsync* cuMemcpyAsync;
  PFN_cuMemcpy2D* cuMemcpy2D;
  PFN_cuMemcpy2DAsync* cuMemcpy2DAsync;
  PFN_cuMemcpyHtoD* cuMemcpyHtoD;
  PFN_cuMemcpyHtoDAsync* cuMemcpyHtoDAsync;
  PFN_cuMemcpyDtoH* cuMemcpyDtoH;
  PFN_cuMemcpyDtoHAsync* cuMemcpyDtoHAsync;
  PFN_cuMemcpyDtoD* cuMemcpyDtoD;
  PFN_cuMemcpyDtoDAsync* cuMemcpyDtoDAsync;
  PFN_cuGetErrorName* cuGetErrorName;
  PFN_cuGetErrorString* cuGetErrorString;
  PFN_cuCtxGetDevice* cuCtxGetDevice;

  PFN_cuDevicePrimaryCtxRetain* cuDevicePrimaryCtxRetain;
  PFN_cuDevicePrimaryCtxRelease* cuDevicePrimaryCtxRelease;
  PFN_cuDevicePrimaryCtxSetFlags* cuDevicePrimaryCtxSetFlags;
  PFN_cuDevicePrimaryCtxGetState* cuDevicePrimaryCtxGetState;
  PFN_cuDevicePrimaryCtxReset* cuDevicePrimaryCtxReset;

  PFN_cuStreamCreate* cuStreamCreate;
  PFN_cuStreamQuery* cuStreamQuery;
  PFN_cuStreamSynchronize* cuStreamSynchronize;
  PFN_cuStreamDestroy* cuStreamDestroy;
  PFN_cuStreamAddCallback* cuStreamAddCallback;
  PFN_cuEventCreate* cuEventCreate;
  PFN_cuEventDestroy* cuEventDestroy;
  PFN_cuEventSynchronize* cuEventSynchronize;
  PFN_cuEventQuery* cuEventQuery;
  PFN_cuEventRecord* cuEventRecord;

  PFN_cuLaunchKernel* cuLaunchKernel;
  PFN_cuLinkCreate* cuLinkCreate;
  PFN_cuLinkAddData* cuLinkAddData;
  PFN_cuLinkComplete* cuLinkComplete;
  PFN_cuLinkDestroy* cuLinkDestroy;
  PFN_cuModuleLoadData* cuModuleLoadData;
  PFN_cuModuleUnload* cuModuleUnload;
  PFN_cuModuleGetFunction* cuModuleGetFunction;
  PFN_cuModuleGetGlobal* cuModuleGetGlobal;
  PFN_cuTexObjectCreate* cuTexObjectCreate;
  PFN_cuTexObjectDestroy* cuTexObjectDestroy;

  PFN_cuGLGetDevices* cuGLGetDevices;
  PFN_cuGraphicsGLRegisterImage* cuGraphicsGLRegisterImage;
  PFN_cuGraphicsUnregisterResource* cuGraphicsUnregisterResource;
  PFN_cuGraphicsMapResources* cuGraphicsMapResources;
  PFN_cuGraphicsUnmapResources* cuGraphicsUnmapResources;
  PFN_cuGraphicsSubResourceGetMappedArray* cuGraphicsSubResourceGetMappedArray;

  PFN_cuImportExternalMemory* cuImportExternalMemory;
  PFN_cuDestroyExternalMemory* cuDestroyExternalMemory;
  PFN_cuExternalMemoryGetMappedBuffer* cuExternalMemoryGetMappedBuffer;
  PFN_cuExternalMemoryGetMappedMipmappedArray* cuExternalMemoryGetMappedMipmappedArray;
  PFN_cuMipmappedArrayDestroy* cuMipmappedArrayDestroy;

  PFN_cuMipmappedArrayGetLevel* cuMipmappedArrayGetLevel;

  PFN_cuImportExternalSemaphore* cuImportExternalSemaphore;
  PFN_cuDestroyExternalSemaphore* cuDestroyExternalSemaphore;
  PFN_cuSignalExternalSemaphoresAsync* cuSignalExternalSemaphoresAsync;
  PFN_cuWaitExternalSemaphoresAsync* cuWaitExternalSemaphoresAsync;
};

struct VTKSTREAMINGNVENCODE_EXPORT vtkNvCudaDriverImportTable
{
  vtkNvCudaDriverImportTable();
  ~vtkNvCudaDriverImportTable();

  bool LoadFunctionsTable();
  bool CloseLibrary();

private:
  // for nvcuda.dll on windows or libcuda.so.1 on nix
  VTKSTREAMING_NV_LIB_HANDLE LibraryHandle = nullptr;
  CudaDriverFunctionsList* FunctionsList = nullptr;

  void FreeFunctions();
};

#endif
// VTK-HeaderTest-Exclude: vtkNvCudaDriverImportTable.h
