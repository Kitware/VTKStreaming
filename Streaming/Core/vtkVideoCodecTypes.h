/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoCodecTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVideoCodecTypes_h
#define vtkVideoCodecTypes_h

#include "vtkStreamingCoreModule.h"

// For new codecs, please insert above MaxNumberOfSupportedCodecs

enum class VTKVideoCodecType
{
  VTKVC_VP9,
  VTKVC_AV1,
  VTKVC_H264,
  VTKVC_H265,
  VTKVC_JPEG,
  VTKVC_MaxNumberOfSupportedCodecs
};

struct VTKSTREAMINGCORE_EXPORT vtkVideoCodecTypeUtilities
{
  static const char* ToString(VTKVideoCodecType codec)
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
      case VTKVideoCodecType::VTKVC_JPEG:
        return "jpeg";
      default:
        return "unsupported codec";
    }
  }
};

#endif // vtkVideoCodecTypes_h
// VTK-HeaderTest-Exclude: vtkVideoCodecTypes.h
