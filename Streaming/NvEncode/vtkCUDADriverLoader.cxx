/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCUDADriverLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCUDADriverLoader.h"
#include "vtkCUDADriverAPI.h"
#include "vtkLogger.h"

#include <cstring>

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
    this->FunctionsList->name = (TFN_##name*)result;                                               \
                                                                                                   \
  } while (0)

vtkCUDADriverLoader::vtkCUDADriverLoader() = default;

vtkCUDADriverLoader::~vtkCUDADriverLoader()
{
  this->FreeFunctions();
  if (this->LibraryHandle != nullptr)
  {
    VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  }
}

bool vtkCUDADriverLoader::LoadFunctionsTable()
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
  this->FunctionsList = new CUDRVFunctions;
  std::memset(this->FunctionsList, 0, sizeof(CUDRVFunctions));
  if (this->FunctionsList == nullptr)
  {
    vtkLogF(ERROR, "Out of memory!");
    return false;
  }
  GetProcEntryPoint(cuInit);

  GetProcEntryPoint(cuDeviceGetCount);
  GetProcEntryPoint(cuDeviceGet);
  GetProcEntryPoint(cuDeviceGetName);

  GetProcEntryPoint(cuCtxCreate_v2);
  GetProcEntryPoint(cuCtxGetApiVersion);
  GetProcEntryPoint(cuCtxGetDevice);
  GetProcEntryPoint(cuCtxPushCurrent_v2);
  GetProcEntryPoint(cuCtxPopCurrent_v2);
  GetProcEntryPoint(cuCtxDestroy_v2);

  GetProcEntryPoint(cuGetErrorName);
  GetProcEntryPoint(cuGetErrorString);

  GetProcEntryPoint(cuGLGetDevices_v2);

  GetProcEntryPoint(cuGraphicsGLRegisterImage);
  GetProcEntryPoint(cuGraphicsUnregisterResource);
  GetProcEntryPoint(cuGraphicsMapResources);
  GetProcEntryPoint(cuGraphicsUnmapResources);
  GetProcEntryPoint(cuGraphicsResourceSetMapFlags);
  GetProcEntryPoint(cuGraphicsSubResourceGetMappedArray);

  return true;
}

bool vtkCUDADriverLoader::CloseLibrary()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle == nullptr)
  {
    return true;
  }
  VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  return true;
}

void vtkCUDADriverLoader::FreeFunctions()
{
  vtkLogScopeF(TRACE, "%s this->FunctionsList=%p", __func__, this->FunctionsList);
  if (this->FunctionsList != nullptr)
  {
    delete this->FunctionsList;
    this->FunctionsList = nullptr;
  }
}

bool vtkCUDADriverLoader::CheckAvailability() noexcept
{
  auto library = VTKSTREAMING_NV_LOAD_LIB(VTKSTREAMING_CUDA_LIBNAME);
  if (library)
  {
    VTKSTREAMING_NV_FREE_LIB(library);
    return true;
  }

  return false;
}
