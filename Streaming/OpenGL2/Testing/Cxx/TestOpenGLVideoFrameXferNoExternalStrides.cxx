/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLVideoFrameXferNoExternalStrides.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the memory access API for vtkOpenGLVideoFrame without external strides.

#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTestUtilities.h"

int TestOpenGLVideoFrameXferNoExternalStrides(int argc, char* argv[])
{
  bool success = true;
  const int width = 240, height = 240;

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkCylinderSource> cyl;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;

  mapper->SetInputConnection(cyl->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.647, 0.2);
  actor->RotateX(30.0);
  actor->RotateY(-45.0);

  ren->AddActor(actor);
  ren->SetBackground(0.2, 0.2, 0.2);

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);

  renWin->Initialize();
  renWin->Render();

  auto pixels = renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1);

  vtkNew<vtkOpenGLVideoFrame> srcFrame;
  srcFrame->SetContext(renWin);
  srcFrame->SetWidth(width);
  srcFrame->SetHeight(height);
  srcFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  srcFrame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
  srcFrame->ComputeDefaultStrides();
  srcFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture. ");
  srcFrame->CopyData(pixels, 4 * width * height);
  vtkOpenGLCheckErrors("ERROR uploading pixels to gl texture. ");

  vtkNew<vtkOpenGLVideoFrame> dstFrame;
  dstFrame->SetContext(renWin);
  dstFrame->ComputeDefaultStrides();
  // deep copy allocates data if necessary, but do it explicitly so we catch errors.
  dstFrame->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture for destination. ");
  dstFrame->DeepCopy(srcFrame);
  vtkOpenGLCheckErrors("ERROR fetching pixels from gl texture. ");

  unsigned char* srcData = nullptr;
  assert(dstFrame->GetData(srcData) == srcFrame->GetActualSize());
  success &= dstFrame->GetData(srcData) == srcFrame->GetActualSize();
  for (int i = 0; i < dstFrame->GetActualSize(); ++i)
  {
    assert(pixels[i] == srcData[i]);
    success &= pixels[i] == srcData[i];
    // vtkLogF(TRACE, "d[%d]=%d == h[%d]=%d", i, pixels[i], i, srcData[i]);
  }
  return 0;
}
