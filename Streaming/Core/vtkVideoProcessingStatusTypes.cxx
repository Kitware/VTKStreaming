/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoProcessingStatusTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoProcessingStatusTypes.h"

const char* vtkVideoProcessingStatusTypeUtilities::ToString(VTKVideoProcessingStatusType status)
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
