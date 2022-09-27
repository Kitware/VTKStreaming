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

#include "vtkObject.h"

#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"

int TestOpenGLVideoFrameXferWithExternalStrides(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 239, height = 240;

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
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

  auto pixels =
    vtk::TakeSmartPointer(vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height));
  int strides[3] = { width + (240 - width), width + (240 - width), 0 };
  srcFrame->SetStrides(strides, 3);

  srcFrame->SetContext(renWin);
  srcFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture for source. ");
  srcFrame->CopyData(pixels);
  vtkOpenGLCheckErrors("ERROR uploading pixels to gl texture for source. ");

  vtkNew<vtkOpenGLVideoFrame> dstFrame;

  dstFrame->SetContext(renWin);
  // deep copy allocates data if necessary, but do it explicitly so we catch errors.
  dstFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture for destination. ");
  dstFrame->DeepCopy(srcFrame);
  vtkOpenGLCheckErrors("ERROR fetching pixels from gl texture of source -> destination. ");

  unsigned char *dstData = nullptr, *srcData = pixels->GetPointer(0);
  assert(dstFrame->GetData(dstData) == srcFrame->GetActualSize());
  success &= dstFrame->GetData(dstData) == srcFrame->GetActualSize();
  for (int i = 0; i < dstFrame->GetActualSize(); ++i)
  {
    assert(srcData[i] == dstData[i]);
    success &= srcData[i] == dstData[i];
    // vtkLogF(TRACE, "src[%d]=%d == dsr[%d]=%d", i, srcData[i], i, dstData[i]);
  }
  return 0;
}
