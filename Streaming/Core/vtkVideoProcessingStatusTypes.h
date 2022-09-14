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
  static const char* ToString(VTKVideoProcessingStatusType status)
  {
    switch (status)
    {
      case VTKVideoProcessingStatusType::VTKVPStatus_Success:
        return "Success.";
      case VTKVideoProcessingStatusType::VTKVPStatus_EOFError:
        return "EOF reached.";
      case VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue:
        return "Invalid value.";
      case VTKVideoProcessingStatusType::VTKVPStatus_OutOfMemory:
        return "Out of memory.";
      case VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain:
        return "Recv packets and try to send again.";
      case VTKVideoProcessingStatusType::VTKVPStatus_TryRecvAgain:
        return "Send more frames and try to recv again.";
      case VTKVideoProcessingStatusType::VTKVPStatus_Busy:
        return "Busy right now, please try again later.";
      case VTKVideoProcessingStatusType::VTKVPStatus_UnknownError:
      default:
        return "Unknown error.";
    }
  }
};

#endif // vtkVideoProcessingStatusTypes_h
// VTK-HeaderTest-Exclude: vtkVideoProcessingStatusTypes.h
