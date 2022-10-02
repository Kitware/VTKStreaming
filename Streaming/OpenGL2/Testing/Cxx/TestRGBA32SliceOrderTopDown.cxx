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

#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkImageFlip.h"
#include "vtkLogger.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

int TestRGBA32SliceOrderTopDown(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 240, height = 240;
  auto pixels =
    vtk::TakeSmartPointer(vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height));

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);

  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkOpenGLVideoFrame> rgba32Picture;
  rgba32Picture->SetContext(renWin);
  rgba32Picture->SetWidth(width);
  rgba32Picture->SetHeight(height);
  rgba32Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  rgba32Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  rgba32Picture->ComputeDefaultStrides();

  rgba32Picture->AllocateDataStore();
  rgba32Picture->CopyData(pixels, width * 4, height);
  rgba32Picture->Render(renWin);

  // we cannot use vtkRegressionTest macro because it re-renders and reads the front/back buffer.
  // both of which will not have rgba32Picture overlay.

  vtkNew<vtkUnsignedCharArray> pixels2;
  renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1, pixels2);

  vtkNew<vtkImageData> baseline;
  baseline->SetDimensions(width, height, 1);
  baseline->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
  std::copy(pixels->GetPointer(0), pixels->GetPointer(pixels->GetNumberOfValues()),
    reinterpret_cast<vtkUnsignedCharArray::ValueType*>(
      baseline->GetPointData()->GetScalars()->GetVoidPointer(0)));

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

  vtkLog(INFO, << "Threshold diff " << imageDiff->GetThresholdedError());
  return imageDiff->GetThresholdedError() < 0.5 ? 0 : 1;
}
