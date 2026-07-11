// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef vtkVideoProcessingStatusTypes_h
#define vtkVideoProcessingStatusTypes_h

#include "vtkStreamingCoreModule.h"

// For new status types, please insert above MaxCount

enum class VTKVideoProcessingStatusType
{
  VTKVPStatus_Success,
  VTKVPStatus_EOFError,
  VTKVPStatus_InvalidValue,
  VTKVPStatus_OutOfMemory,
  VTKVPStatus_TrySendAgain,
  VTKVPStatus_TryRecvAgain,
  VTKVPStatus_UnknownError,
  VTKVPStatus_Busy,
  VTKVPStatus_MaxCount
};

struct VTKSTREAMINGCORE_EXPORT vtkVideoProcessingStatusTypeUtilities
{
  static const char* ToString(VTKVideoProcessingStatusType status);
};

#endif // vtkVideoProcessingStatusTypes_h
// VTK-HeaderTest-Exclude: vtkVideoProcessingStatusTypes.h
