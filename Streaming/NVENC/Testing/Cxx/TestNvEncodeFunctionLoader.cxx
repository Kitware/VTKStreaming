/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncodeFunctionLoader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises delay load of nvEncodeAPI.dll/libnvidia-encode.so

#include "vtkNvEncodeImportTable.h"

int TestNvEncodeFunctionLoader(int argc, char* argv[])
{
  vtkNvEncodeImportTable loader;
  return loader.LoadFunctionsTable() ? 0 : 1;
}
