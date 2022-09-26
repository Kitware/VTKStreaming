/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncodeImportTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkNvEncodeImportTable_h
#define vtkNvEncodeImportTable_h

#include "vtkStreamingNvEncodeModule.h"

#include "nvEncodeAPI.h"      // for status type
#include "vtkDynamicLoader.h" // for loader

struct VTKSTREAMINGNVENCODE_EXPORT vtkNvEncodeImportTable
{
  typedef NVENCSTATUS (*PFN_NvEncodeAPIGetMaxSupportedVersion)(uint32_t*);
  typedef NVENCSTATUS (*PFN_NvEncodeAPICreateInstance)(NV_ENCODE_API_FUNCTION_LIST*);
  vtkNvEncodeImportTable();
  ~vtkNvEncodeImportTable();

  bool LoadFunctionsTable();
  bool CloseLibrary();

  // upon loading the lib, we need two functions from it before we get to the cool stuff.
  PFN_NvEncodeAPIGetMaxSupportedVersion NvEncodeAPIGetMaxSupportedVersion = nullptr;
  PFN_NvEncodeAPICreateInstance NvEncodeAPICreateInstance = nullptr;

private:
  // for nvEncodeAPI.dll on windows or libnvidia-encode.so on nix.
  vtkLibHandle LibraryHandle = nullptr;
};

#endif // vtkNvEncodeImportTable_h
// VTK-HeaderTest-Exclude: vtkNvEncodeImportTable.h
