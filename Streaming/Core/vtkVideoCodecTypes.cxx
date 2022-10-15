/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoCodecTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoCodecTypes.h"

const char* vtkVideoCodecTypeUtilities::ToString(VTKVideoCodecType codec)
{
  switch (codec)
  {
    case VTKVideoCodecType::VTKVC_VP9:
      return "vp9";
    case VTKVideoCodecType::VTKVC_AV1:
      return "av1";
    case VTKVideoCodecType::VTKVC_H264:
      return "h.264";
    case VTKVideoCodecType::VTKVC_H265:
      return "h.265";
    default:
      return "unsupported codec";
  }
}
