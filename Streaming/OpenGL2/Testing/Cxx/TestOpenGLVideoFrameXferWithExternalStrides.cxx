/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLVideoFrameXferWithExternalStrides.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the memory access API for vtkOpenGLVideoFrame with strides.

#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRenderer.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkXRenderWindowInteractor.h"
#include <vector>

int TestOpenGLVideoFrameXferWithExternalStrides(int argc, char* argv[])
{
  bool success = true;
  const int width = 239, height = 240;

  vtkNew<vtkXOpenGLRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  ren->SetBackground(0.5, 0.5, 0.5);
  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkOpenGLVideoFrame> srcFrame;
  srcFrame->SetWidth(width);
  srcFrame->SetHeight(height);
  srcFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_NV12);
  srcFrame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);

  int strides[3] = { width + (240 - width), width + (240 - width), 0 };
  srcFrame->SetStrides(strides, 3);

  srcFrame->InitializeGraphicsResources(renWin);
  srcFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture for source. ");

  std::vector<unsigned char> pixels(srcFrame->GetActualSize());
  for (int i = 0; i < srcFrame->GetActualSize(); ++i)
  {
    pixels[i] = i % 255;
  }
  srcFrame->CopyData(pixels.data(), pixels.size());
  vtkOpenGLCheckErrors("ERROR uploading pixels to gl texture for source. ");

  vtkNew<vtkOpenGLVideoFrame> dstFrame;
  dstFrame->ShallowCopy(srcFrame);

  dstFrame->InitializeGraphicsResources(renWin);
  dstFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture for destination. ");
  dstFrame->CopyFrameData(srcFrame);
  vtkOpenGLCheckErrors("ERROR fetching pixels from gl texture of source -> destination. ");

  unsigned char* hData = nullptr;
  assert(dstFrame->GetData(hData) == srcFrame->GetActualSize());
  success &= dstFrame->GetData(hData) == srcFrame->GetActualSize();
  for (int i = 0; i < dstFrame->GetActualSize(); ++i)
  {
    assert(pixels[i] == hData[i]);
    success &= pixels[i] == hData[i];
    // vtkLogF(TRACE, "d[%d]=%d == h[%d]=%d", i, pixels[i], i, hData[i]);
  }
  return 0;
}
