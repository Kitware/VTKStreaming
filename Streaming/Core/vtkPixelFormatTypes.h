// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkPixelFormatTypes_h
#define vtkPixelFormatTypes_h

#include "vtkStreamingCoreModule.h"

enum VTKPixelFormatType
{
  VTKPF_RGBA32, // r,g,b,a 8:8:8:8, 32 bpp Ex: 2x2 -> rgba|rgba|rgba|rgba
  VTKPF_RGB24,  // r,g,b   8:8:8, 24 bpp Ex: 2x2 -> rgb|rgb|rgb|rgb
  VTKPF_NV12,   // y,Cb,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|uv
  VTKPF_IYUV,   // y,Cr,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|u|v
};

struct VTKSTREAMINGCORE_EXPORT vtkPixelFormatTypeUtilities
{
  static const char* ToString(VTKPixelFormatType pixelFormat);
};

#endif // vtkPixelFormatTypes_h
// VTK-HeaderTest-Exclude: vtkPixelFormatTypes.h
