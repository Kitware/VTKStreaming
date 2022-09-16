/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCPUVideoFrameXferNoExternalStrides.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the memory access API for vtkCPUVideoFrame w/o external strides.

#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkPixelFormatTypes.h"

int TestCPUVideoFrameXferNoExternalStrides(int argc, char* argv[])
{
  bool success = true;
  const int width = 240, height = 240;

  vtkNew<vtkCPUVideoFrame> srcFrame;
  srcFrame->SetWidth(width);
  srcFrame->SetHeight(height);
  srcFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  srcFrame->ComputeDefaultStrides();
  srcFrame->AllocateDataStore();
  assert(srcFrame->GetStrides()[0] == width * 4);
  vtkLog(TRACE, << "Actual size: " << srcFrame->GetActualSize());
  vtkLog(TRACE, << "Estimated size: "
                << srcFrame->GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_RGBA32));
  std::vector<unsigned char> rawPixels(srcFrame->GetActualSize());
  for (int i = 0; i < srcFrame->GetActualSize(); ++i)
  {
    rawPixels[i] = i % 255;
  }
  srcFrame->CopyData(rawPixels.data(), rawPixels.size());

  vtkNew<vtkCPUVideoFrame> dstFrame;
  dstFrame->CopyMetadata(srcFrame);
  dstFrame->CopyFrameData(srcFrame);

  unsigned char* dstData = nullptr;
  assert(dstFrame->GetData(dstData) == 4 * width * height);
  success &= dstFrame->GetData(dstData) == 4 * width * height;
  for (int i = 0; i < 4 * width * height; ++i)
  {
    assert(rawPixels[i] == dstData[i]);
    success &= rawPixels[i] == dstData[i];
    // vtkLogF(TRACE, "d[%d]=%d == h[%d]=%d", i, rawPixels[i], i, dstData[i]);
  }
  return success ? 0 : 1;
}
