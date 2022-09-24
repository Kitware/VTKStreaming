/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCPUVideoFrameXferWithExternalStrides.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the memory access API for vtkCPUVideoFrame w/ external strides.

#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkPixelFormatTypes.h"

int TestCPUVideoFrameXferWithExternalStrides(int argc, char* argv[])
{
  bool success = true;
  const int width = 239, height = 240;

  vtkNew<vtkCPUVideoFrame> srcFrame;
  srcFrame->SetWidth(width);
  srcFrame->SetHeight(height);
  srcFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  int strides[3] = { width + 21, (width + 21) >> 2, (width + 21) >> 2 };
  srcFrame->SetStrides(strides, 3);
  srcFrame->AllocateDataStore();

  assert(srcFrame->GetStrides()[0] == width + 21);
  assert(srcFrame->GetStrides()[1] == (width + 21) >> 2);
  assert(srcFrame->GetStrides()[2] == (width + 21) >> 2);

  vtkLog(TRACE, << "Actual size: " << srcFrame->GetActualSize());
  vtkLog(TRACE, << "Estimated size: "
                << srcFrame->GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_IYUV));
  std::vector<unsigned char> rawPixels(srcFrame->GetActualSize());
  for (int i = 0; i < srcFrame->GetActualSize(); ++i)
  {
    rawPixels[i] = i % 255;
  }
  srcFrame->CopyData(rawPixels.data(), rawPixels.size());

  vtkNew<vtkCPUVideoFrame> dstFrame;
  dstFrame->DeepCopy(srcFrame);

  unsigned char* dstData = nullptr;
  assert(dstFrame->GetData(dstData) == srcFrame->GetActualSize());
  success &= dstFrame->GetData(dstData) == srcFrame->GetActualSize();
  for (int i = 0; i < srcFrame->GetActualSize(); ++i)
  {
    assert(rawPixels[i] == dstData[i]);
    success &= rawPixels[i] == dstData[i];
    // vtkLogF(TRACE, "d[%d]=%d == h[%d]=%d", i, rawPixels[i], i, dstData[i]);
  }
  return success ? 0 : 1;
}
