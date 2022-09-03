/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGCommon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFFMPEGCommon_h
#define vtkFFMPEGCommon_h

#include "vtkVideoProcessingStatusTypes.h"

extern "C"
{
#include <libavutil/error.h>
}

//------------------------------------------------------------------------------
static VTKVideoProcessingStatusType ParseFFMPEGStatus(int statusCode, bool during_send = true)
{
  VTKVideoProcessingStatusType result;
  switch (statusCode)
  {
    case 0:
      result = VTKVideoProcessingStatusType::Success;
      break;
    case AVERROR_EOF:
      result = VTKVideoProcessingStatusType::EOFError;
      break;
    case AVERROR(EINVAL):
      result = VTKVideoProcessingStatusType::InvalidValue;
      break;
    case AVERROR(ENOMEM):
      result = VTKVideoProcessingStatusType::OutOfMemory;
      break;
    case AVERROR(EAGAIN):
      result = during_send ? VTKVideoProcessingStatusType::TrySendAgain
                           : VTKVideoProcessingStatusType::TryRecvAgain;
      break;
    case AVERROR_UNKNOWN:
    default:
      result = VTKVideoProcessingStatusType::UnknownError;
      break;
  }
  return result;
}

#endif // vtkFFMPEGCommon_h
