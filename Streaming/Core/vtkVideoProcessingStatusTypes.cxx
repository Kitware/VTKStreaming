// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

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
