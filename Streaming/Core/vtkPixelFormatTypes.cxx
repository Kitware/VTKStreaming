/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixelFormatTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPixelFormatTypes.h"

const char* vtkPixelFormatTypeUtilities::ToString(VTKPixelFormatType pixelFormat)
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
