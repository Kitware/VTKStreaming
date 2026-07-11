// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkCUDADriverLoader_h
#define vtkCUDADriverLoader_h

#include "vtkNvDynamicLoader.h"
#include "vtkStreamingNvEncodeModule.h"

struct CUDRVFunctions;

struct VTKSTREAMINGNVENCODE_EXPORT vtkCUDADriverLoader
{
  vtkCUDADriverLoader();
  ~vtkCUDADriverLoader();

  bool LoadFunctionsTable();
  bool CloseLibrary();

  CUDRVFunctions* FunctionsList = nullptr;

private:
  // for nvcuda.dll on windows or libcuda.so.1 on nix
  VTKSTREAMING_NV_LIB_HANDLE LibraryHandle = nullptr;

  void FreeFunctions();
};

#endif
// VTK-HeaderTest-Exclude: vtkCUDADriverLoader.h
