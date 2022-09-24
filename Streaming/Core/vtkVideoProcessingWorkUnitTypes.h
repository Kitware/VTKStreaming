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
 *
 * A video encoder accepts vtkRawVideoFrame as input. The output
 * is the status code along with one or more vtkCompressedVideoPacket instances.
 *
 * A video decoder accepts vtkCompressedVideoPacket as input. The output
 * is the status code along with one or more vtkRawVideoFrame instances.
 */

#ifndef vtkVideoProcessingWorkUnitTypes_h
#define vtkVideoProcessingWorkUnitTypes_h

#include "vtkCompressedVideoPacket.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <functional>
#include <utility>
#include <vector>

using VTKVideoEncoderInputType = vtkSmartPointer<vtkRawVideoFrame>;
using VTKVideoEncoderResultType =
  std::pair<VTKVideoProcessingStatusType, std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>>;
using VTKVideoEncodeWorkerType = std::function<VTKVideoEncoderResultType(VTKVideoEncoderInputType)>;

using VTKVideoDecoderInputType = vtkSmartPointer<vtkCompressedVideoPacket>;
using VTKVideoDecoderResultType =
  std::pair<VTKVideoProcessingStatusType, std::vector<vtkSmartPointer<vtkRawVideoFrame>>>;
using VTKVideoDecodeWorkerType = std::function<VTKVideoDecoderResultType(VTKVideoDecoderInputType)>;

#endif // vtkVideoProcessingWorkUnitTypes_h
// VTK-HeaderTest-Exclude: vtkVideoProcessingWorkUnitTypes.h
