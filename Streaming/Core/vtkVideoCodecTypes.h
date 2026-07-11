// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkVideoCodecTypes_h
#define vtkVideoCodecTypes_h

#include "vtkStreamingCoreModule.h"

// For new codecs, please insert above MaxNumberOfSupportedCodecs

enum VTKVideoCodecType
{
  VTKVC_VP9,
  VTKVC_AV1,
  VTKVC_H264,
  VTKVC_H265,
  VTKVC_MaxNumberOfSupportedCodecs
};

struct VTKSTREAMINGCORE_EXPORT vtkVideoCodecTypeUtilities
{
  static const char* ToString(VTKVideoCodecType codec);
};

#endif // vtkVideoCodecTypes_h
// VTK-HeaderTest-Exclude: vtkVideoCodecTypes.h
