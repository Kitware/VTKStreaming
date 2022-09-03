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

// For new codecs, please insert above MaxCount

enum VTKVideoProcessingStatusType
{
  Success,
  EOFError,
  InvalidValue,
  OutOfMemory,
  TrySendAgain,
  TryRecvAgain,
  UnknownError,
  MaxCount
};

static const char* VTKVideoProcessingStatusTypeToStr(VTKVideoProcessingStatusType status)
{
  switch (status)
  {
    case VTKVideoProcessingStatusType::Success:
      return "Success";
    case VTKVideoProcessingStatusType::EOFError:
      return "EOF reached";
    case VTKVideoProcessingStatusType::InvalidValue:
      return "Invalid value";
    case VTKVideoProcessingStatusType::OutOfMemory:
      return "Out of memory";
    case VTKVideoProcessingStatusType::TrySendAgain:
      return "Recv packets and try to send again";
    case VTKVideoProcessingStatusType::TryRecvAgain:
      return "Send more frames and try to recv again";
    case VTKVideoProcessingStatusType::UnknownError:
    default:
      return "Unknown error";
  }
}

#endif // vtkVideoProcessingStatus_h