/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncodeLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNvEncodeLoader.h"
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
    this->name = (PFN_##name)result;                                                               \
                                                                                                   \
  } while (0)

vtkNvEncodeLoader::vtkNvEncodeLoader() = default;

vtkNvEncodeLoader::~vtkNvEncodeLoader()
{
  if (this->LibraryHandle != nullptr)
  {
    VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  }
}

bool vtkNvEncodeLoader::LoadFunctionsTable()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle != nullptr)
  {
    return true;
  }

  this->LibraryHandle = VTKSTREAMING_NV_LOAD_LIB(VTKSTREAMING_NVENC_LIBNAME);
  if (this->LibraryHandle == nullptr)
  {
    vtkLogF(ERROR,
      "Failed to load %s. Please install or upgrade NVIDIA drivers if you have an NVIDIA GPU.",
      VTKSTREAMING_NVENC_LIBNAME);
    return false;
  }
  else
  {
    vtkLogF(TRACE, "Loaded %s.", VTKSTREAMING_NVENC_LIBNAME);
  }
  GetProcEntryPoint(NvEncodeAPIGetMaxSupportedVersion);
  GetProcEntryPoint(NvEncodeAPICreateInstance);
  return true;
}

bool vtkNvEncodeLoader::CloseLibrary()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle == nullptr)
  {
    return true;
  }
  VTKSTREAMING_NV_FREE_LIB(this->LibraryHandle);
  return true;
}
