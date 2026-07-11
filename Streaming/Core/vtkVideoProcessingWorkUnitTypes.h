// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVideoProcessingWorkUnitTypes
 * @brief   this class defines work unit types for convenience.
 *
 * A video encoder accepts vtkRawVideoFrame. The output
 * is the status code along with one or more vtkCompressedVideoPacket instances.
 *
 * A video decoder accepts vtkCompressedVideoPacket. The output
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

using VTKVideoEncoderResultType =
  std::pair<VTKVideoProcessingStatusType, std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>>;

using VTKVideoDecoderInputType = vtkSmartPointer<vtkCompressedVideoPacket>;
using VTKVideoDecoderResultType =
  std::pair<VTKVideoProcessingStatusType, std::vector<vtkSmartPointer<vtkRawVideoFrame>>>;

#endif // vtkVideoProcessingWorkUnitTypes_h
// VTK-HeaderTest-Exclude: vtkVideoProcessingWorkUnitTypes.h
