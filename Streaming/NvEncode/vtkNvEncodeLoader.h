// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkNvEncodeImportTable_h
#define vtkNvEncodeImportTable_h

#include "vtkStreamingNvEncodeModule.h"

#include "nvEncodeAPI.h"        // for status type
#include "vtkNvDynamicLoader.h" // for loader

struct VTKSTREAMINGNVENCODE_EXPORT vtkNvEncodeLoader
{
  typedef NVENCSTATUS (*PFN_NvEncodeAPIGetMaxSupportedVersion)(uint32_t*);
  typedef NVENCSTATUS (*PFN_NvEncodeAPICreateInstance)(NV_ENCODE_API_FUNCTION_LIST*);
  vtkNvEncodeLoader();
  ~vtkNvEncodeLoader();

  bool LoadFunctionsTable();
  bool CloseLibrary();

  // upon loading the lib, we need two functions from it before we get to the cool stuff.
  PFN_NvEncodeAPIGetMaxSupportedVersion NvEncodeAPIGetMaxSupportedVersion = nullptr;
  PFN_NvEncodeAPICreateInstance NvEncodeAPICreateInstance = nullptr;

private:
  // for nvEncodeAPI.dll on windows or libnvidia-encode.so on nix.
  VTKSTREAMING_NV_LIB_HANDLE LibraryHandle = nullptr;
};

#endif // vtkNvEncodeImportTable_h
// VTK-HeaderTest-Exclude: vtkNvEncodeLoader.h
