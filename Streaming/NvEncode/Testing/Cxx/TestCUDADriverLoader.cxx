// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
// This test exercises delay load of nvcuda.dll or libcuda.so.1

#include "vtkCUDADriverLoader.h"
#include "vtkStreamingTestUtility.h"

int TestCUDADriverLoader(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  vtkCUDADriverLoader loader;
  return loader.LoadFunctionsTable() ? 0 : 1;
}
