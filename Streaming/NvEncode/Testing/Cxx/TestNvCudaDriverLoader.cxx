/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvCudaDriverLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises delay load of nvcuda.dll or libcuda.so.1

#include "vtkNvCudaDriverImportTable.h"

int TestNvCudaDriverLoader(int argc, char* argv[])
{
  vtkNvCudaDriverImportTable loader;
  return loader.LoadFunctionsTable() ? 0 : 1;
}
