/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRGBA32SliceOrderTopDown.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the shader programs that draw RGBA32 top-down pictures.

#include "vtkCylinderSource.h"
#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkImageFlip.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPNGWriter.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXOpenGLRenderWindow.h"
#include "vtkXRenderWindowInteractor.h"

int TestRGBA32SliceOrderTopDown(int argc, char* argv[])
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

  vtkNew<vtkUnsignedCharArray> pixels;
  renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1, pixels);
  // now let's remove it.
  ren->RemoveActor(actor);
  // display blank screen.
  renWin->Render();

  vtkNew<vtkOpenGLVideoFrame> rgba32Picture;
  rgba32Picture->SetContext(renWin);
  rgba32Picture->SetWidth(width);
  rgba32Picture->SetHeight(height);
  rgba32Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  rgba32Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  rgba32Picture->ComputeDefaultStrides();

  rgba32Picture->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture. ");

  rgba32Picture->CopyData(pixels->GetPointer(0), 4 * width * height);
  vtkOpenGLCheckErrors("ERROR uploading pixels to gl texture. ");

  rgba32Picture->Render(renWin);
  vtkOpenGLCheckErrors("ERROR rendering rgba32. ");

  // we cannot use vtkRegressionTest macro because it re-renders and reads the front/back buffer.
  // both of which will not have rgba32Picture overlay.

  vtkNew<vtkUnsignedCharArray> pixels2;
  renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1, pixels2);

  vtkNew<vtkImageData> baseline;
  baseline->SetDimensions(width, height, 1);
  baseline->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  baseline->GetPointData()->SetScalars(pixels);

  vtkNew<vtkImageData> testImage;
  testImage->SetDimensions(width, height, 1);
  testImage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  testImage->GetPointData()->SetScalars(pixels2);

  vtkNew<vtkImageFlip> flipper;
  flipper->SetInputData(baseline);
  flipper->SetFilteredAxes(1);
  flipper->SetFlipAboutOrigin(1);

  vtkNew<vtkImageDifference> imageDiff;
  imageDiff->SetInputData(testImage);
  imageDiff->SetImageConnection(flipper->GetOutputPort());
  imageDiff->SetThreshold(15);
  imageDiff->Update();

  cout << "Threshold diff " << imageDiff->GetThresholdedError();
  return imageDiff->GetThresholdedError() < 0.5 ? 0 : 1;
}
