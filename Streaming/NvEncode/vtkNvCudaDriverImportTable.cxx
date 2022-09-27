/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvCudaDriverImportTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNvCudaDriverImportTable.h"
#include "vtkLogger.h"

#define GetProcEntryPoint(name)                                                                    \
  do                                                                                               \
  {                                                                                                \
    void* result = VTKSTREAMING_NV_SYM_FUNC(this->LibraryHandle, #name);                           \
    if (result == nullptr)                                                                         \
    {                                                                                              \
      vtkLogF(ERROR, "Failed to load %s", #name);                                                  \
      return false;                                                                                \
    }                                                                                              \
    vtkLogF(TRACE, "found symbol %s (%p)", #name, result);                                         \
    this->FunctionsList->name = (PFN_##name*)result;                                               \
                                                                                                   \
  } while (0)

vtkNvCudaDriverImportTable::vtkNvCudaDriverImportTable() = default;

vtkNvCudaDriverImportTable::~vtkNvCudaDriverImportTable()
{
  this->FreeFunctions();
  if (this->LibraryHandle != nullptr)
  {
    VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  }
}

bool vtkNvCudaDriverImportTable::LoadFunctionsTable()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle != nullptr)
  {
    return true;
  }

  this->LibraryHandle = VTKSTREAMING_NV_LOAD_LIB(VTKSTREAMING_CUDA_LIBNAME);
  if (this->LibraryHandle == nullptr)
  {
    vtkLogF(ERROR,
      "Failed to load %s. Please install or upgrade NVIDIA drivers if you have an NVIDIA GPU.",
      VTKSTREAMING_CUDA_LIBNAME);
    return false;
  }
  else
  {
    vtkLogF(TRACE, "Loaded %s.", VTKSTREAMING_CUDA_LIBNAME);
  }
  this->FreeFunctions();
  this->FunctionsList = (CudaDriverFunctionsList*)calloc(1, sizeof(*this->FunctionsList));
  if (this->FunctionsList == nullptr)
  {
    vtkLogF(ERROR, "Out of memory!");
    return false;
  }
  GetProcEntryPoint(cuInit);
  GetProcEntryPoint(cuDeviceGetCount);
  GetProcEntryPoint(cuDeviceGet);
  GetProcEntryPoint(cuDeviceGetAttribute);
  GetProcEntryPoint(cuDeviceGetName);
  GetProcEntryPoint(cuDeviceGetUuid);
  GetProcEntryPoint(cuDeviceComputeCapability);
  GetProcEntryPoint(cuCtxCreate);
  GetProcEntryPoint(cuCtxSetLimit);
  GetProcEntryPoint(cuCtxPushCurrent);
  GetProcEntryPoint(cuCtxPopCurrent);
  GetProcEntryPoint(cuCtxDestroy);
  GetProcEntryPoint(cuMemAlloc);
  GetProcEntryPoint(cuMemAllocPitch);
  GetProcEntryPoint(cuMemAllocManaged);
  GetProcEntryPoint(cuMemsetD8Async);
  GetProcEntryPoint(cuMemFree);
  GetProcEntryPoint(cuMemcpy);
  GetProcEntryPoint(cuMemcpyAsync);
  GetProcEntryPoint(cuMemcpy2D);
  GetProcEntryPoint(cuMemcpy2DAsync);
  GetProcEntryPoint(cuMemcpyHtoD);
  GetProcEntryPoint(cuMemcpyHtoDAsync);
  GetProcEntryPoint(cuMemcpyDtoH);
  GetProcEntryPoint(cuMemcpyDtoHAsync);
  GetProcEntryPoint(cuMemcpyDtoD);
  GetProcEntryPoint(cuMemcpyDtoDAsync);
  GetProcEntryPoint(cuGetErrorName);
  GetProcEntryPoint(cuGetErrorString);
  GetProcEntryPoint(cuCtxGetDevice);
  GetProcEntryPoint(cuDevicePrimaryCtxRetain);
  GetProcEntryPoint(cuDevicePrimaryCtxRelease);
  GetProcEntryPoint(cuDevicePrimaryCtxSetFlags);
  GetProcEntryPoint(cuDevicePrimaryCtxGetState);
  GetProcEntryPoint(cuDevicePrimaryCtxReset);
  GetProcEntryPoint(cuStreamCreate);
  GetProcEntryPoint(cuStreamQuery);
  GetProcEntryPoint(cuStreamSynchronize);
  GetProcEntryPoint(cuStreamDestroy);
  GetProcEntryPoint(cuStreamAddCallback);
  GetProcEntryPoint(cuEventCreate);
  GetProcEntryPoint(cuEventDestroy);
  GetProcEntryPoint(cuEventSynchronize);
  GetProcEntryPoint(cuEventQuery);
  GetProcEntryPoint(cuEventRecord);
  GetProcEntryPoint(cuLaunchKernel);
  GetProcEntryPoint(cuLinkCreate);
  GetProcEntryPoint(cuLinkAddData);
  GetProcEntryPoint(cuLinkComplete);
  GetProcEntryPoint(cuLinkDestroy);
  GetProcEntryPoint(cuModuleLoadData);
  GetProcEntryPoint(cuModuleUnload);
  GetProcEntryPoint(cuModuleGetFunction);
  GetProcEntryPoint(cuModuleGetGlobal);
  GetProcEntryPoint(cuTexObjectCreate);
  GetProcEntryPoint(cuTexObjectDestroy);
  GetProcEntryPoint(cuGLGetDevices);
  GetProcEntryPoint(cuGraphicsGLRegisterImage);
  GetProcEntryPoint(cuGraphicsUnregisterResource);
  GetProcEntryPoint(cuGraphicsMapResources);
  GetProcEntryPoint(cuGraphicsUnmapResources);
  GetProcEntryPoint(cuGraphicsSubResourceGetMappedArray);
  GetProcEntryPoint(cuImportExternalMemory);
  GetProcEntryPoint(cuDestroyExternalMemory);
  GetProcEntryPoint(cuExternalMemoryGetMappedBuffer);
  GetProcEntryPoint(cuExternalMemoryGetMappedMipmappedArray);
  GetProcEntryPoint(cuMipmappedArrayDestroy);
  GetProcEntryPoint(cuMipmappedArrayGetLevel);
  GetProcEntryPoint(cuImportExternalSemaphore);
  GetProcEntryPoint(cuDestroyExternalSemaphore);
  GetProcEntryPoint(cuSignalExternalSemaphoresAsync);
  GetProcEntryPoint(cuWaitExternalSemaphoresAsync);
  return true;
}

bool vtkNvCudaDriverImportTable::CloseLibrary()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle == nullptr)
  {
    return true;
  }
  VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  return true;
}

void vtkNvCudaDriverImportTable::FreeFunctions()
{
  vtkLogScopeF(TRACE, "%s this->FunctionsList=%p", __func__, this->FunctionsList);
  if (this->FunctionsList != nullptr)
  {
    free(this->FunctionsList);
    this->FunctionsList = nullptr;
  }
}