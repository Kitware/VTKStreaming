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
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkXRenderWindowInteractor.h"
#include <sstream>

int TestOpenGLVideoFrameXferNoExternalStrides(int argc, char* argv[])
{
  bool success = true;
  const int width = 240, height = 240;

  vtkNew<vtkXOpenGLRenderWindow> renWin;
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

  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);

  renWin->Initialize();
  renWin->Render();

  auto pixels = renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1);

  vtkNew<vtkOpenGLVideoFrame> srcFrame;
  srcFrame->InitializeGraphicsResources(renWin);
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
  dstFrame->InitializeGraphicsResources(renWin);
  dstFrame->ShallowCopy(srcFrame);
  dstFrame->ComputeDefaultStrides();
  dstFrame->AllocateDataStore();
  dstFrame->CopyFrameData(srcFrame);
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
