// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

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
