/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelFormatTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPixelFormatTypes_h
#define vtkPixelFormatTypes_h

#include "vtkStreamingCoreModule.h"

enum class VTKPixelFormatType
{
  VTKPF_RGBA32, // r,g,b,a 8:8:8:8, 32 bpp Ex: 2x2 -> rgba|rgba|rgba|rgba
  VTKPF_RGB24,  // r,g,b   8:8:8, 24 bpp Ex: 2x2 -> rgb|rgb|rgb|rgb
  VTKPF_NV12,   // y,Cb,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|uv
  VTKPF_IYUV,   // y,Cr,Cr   4:2:0, 12 bpp Ex: 2x2 -> yyyy|u|v
};

struct VTKSTREAMINGCORE_EXPORT vtkPixelFormatTypeUtilities
{
  static const char* ToString(VTKPixelFormatType pixelFormat)
  {
    switch (pixelFormat)
    {
      case VTKPixelFormatType::VTKPF_RGBA32:
        return "rgba32";
      case VTKPixelFormatType::VTKPF_RGB24:
        return "rgb24";
      case VTKPixelFormatType::VTKPF_NV12:
        return "nv12";
      case VTKPixelFormatType::VTKPF_IYUV:
        return "iyuv";
      default:
        return "unsupported pixel format";
    }
  }
};

#endif // vtkPixelFormatTypes_h
// VTK-HeaderTest-Exclude: vtkPixelFormatTypes.h
