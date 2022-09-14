/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncodeImportTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNvEncodeImportTable.h"
#include "vtkDynamicLoader.h"
#include "vtkLogger.h"
#include <vtksys/SystemInformation.hxx>

#define GetProcEntryPoint(name)                                                                    \
  do                                                                                               \
  {                                                                                                \
    /* Hack to cast pointer-to-function to pointer-to-function with different signature.*/         \
    union                                                                                          \
    {                                                                                              \
      vtkSymbolPointer psym;                                                                       \
      PFN_##name u_##name;                                                                         \
    } result;                                                                                      \
    result.psym = vtkDynamicLoader::GetSymbolAddress(this->LibraryHandle, #name);                  \
    if (!result.psym)                                                                              \
    {                                                                                              \
      vtkLogF(ERROR, "Failed to load %s", #name);                                                  \
      return false;                                                                                \
    }                                                                                              \
    vtkLogF(TRACE, "found symbol %s (%p)", #name, result.psym);                                    \
    this->name = result.u_##name;                                                                  \
                                                                                                   \
  } while (0)

vtkNvEncodeImportTable::vtkNvEncodeImportTable() = default;

vtkNvEncodeImportTable::~vtkNvEncodeImportTable()
{
  if (this->LibraryHandle != nullptr)
  {
    vtkDynamicLoader::CloseLibrary(this->LibraryHandle);
  }
}

bool vtkNvEncodeImportTable::LoadFunctionsTable()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle != nullptr)
  {
    return true;
  }

  vtksys::SystemInformation sysImpl;
  const char* libName = sysImpl.GetOSIsWindows() ? "nvEncodeAPI.dll" : "libnvidia-encode.so";

  this->LibraryHandle = vtkDynamicLoader::OpenLibrary(libName);
  if (this->LibraryHandle == nullptr)
  {
    vtkLogF(ERROR,
      "Failed to load %s. Please install or upgrade NVIDIA drivers if you have an NVIDIA GPU.",
      libName);
    return false;
  }
  else
  {
    vtkLogF(TRACE, "Loaded %s.", libName);
  }
  GetProcEntryPoint(NvEncodeAPIGetMaxSupportedVersion);
  GetProcEntryPoint(NvEncodeAPICreateInstance);
  return true;
}

bool vtkNvEncodeImportTable::CloseLibrary()
{
  vtkLogScopeF(TRACE, "%s this->LibraryHandle=%p", __func__, this->LibraryHandle);
  if (this->LibraryHandle == nullptr)
  {
    return true;
  }
  return vtkDynamicLoader::CloseLibrary(this->LibraryHandle);
}
