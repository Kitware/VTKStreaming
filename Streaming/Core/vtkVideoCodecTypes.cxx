// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

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
