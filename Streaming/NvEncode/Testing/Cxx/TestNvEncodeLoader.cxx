// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
// This test exercises delay load of nvEncodeAPI.dll/libnvidia-encode.so

#include "vtkNvEncodeLoader.h"
#include "vtkStreamingTestUtility.h"

int TestNvEncodeLoader(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  vtkNvEncodeLoader loader;
  return loader.LoadFunctionsTable() ? 0 : 1;
}
