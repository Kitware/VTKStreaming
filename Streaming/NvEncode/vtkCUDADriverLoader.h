/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCUDADriverLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
