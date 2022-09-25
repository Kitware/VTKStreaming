/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoProcessingStatusTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
