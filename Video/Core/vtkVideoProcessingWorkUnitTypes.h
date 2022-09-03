/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoProcessingWorkUnitTypes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVideoProcessingWorkUnitTypes
 * @brief   this class defines work unit types for convenience.
 */

#ifndef vtkVideoProcessingWorkUnitTypes_h
#define vtkVideoProcessingWorkUnitTypes_h

#include "vtkCodedVideoPacket.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <functional>
#include <utility>

using EncoderInputType = vtkSmartPointer<vtkRawVideoFrame>;
using EncoderResultType =
  std::pair<VTKVideoProcessingStatusType, vtkSmartPointer<vtkCodedVideoPacket>>;
using EncodeWorkerType = std::function<EncoderResultType(EncoderInputType)>;

using DecoderInputType = vtkSmartPointer<vtkCodedVideoPacket>;
using DecoderResultType =
  std::pair<VTKVideoProcessingStatusType, vtkSmartPointer<vtkRawVideoFrame>>;
using DecodeWorkerType = std::function<DecoderResultType(DecoderInputType)>;

#endif // vtkVideoProcessingWorkUnitTypes_h