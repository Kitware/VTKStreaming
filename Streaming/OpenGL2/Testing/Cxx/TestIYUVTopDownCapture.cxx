/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIYUVTopDownCapture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the shader programs that capture iyuv from a render window.

#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPNGWriter.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

int TestIYUVTopDownCapture(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 240, height = 240;
  auto pixels =
    vtk::TakeSmartPointer(vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height));
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;

  ren->SetBackground(0.2, 0.2, 0.2);
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);

  renWin->Initialize();

  vtkNew<vtkOpenGLVideoFrame> rgba32Picture;
  rgba32Picture->SetContext(renWin);
  rgba32Picture->SetWidth(width);
  rgba32Picture->SetHeight(height);
  rgba32Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  rgba32Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
  rgba32Picture->ComputeDefaultStrides();
  rgba32Picture->CopyData(pixels);
  rgba32Picture->Render(renWin);

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->SetContext(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  iyuvPicture->ComputeDefaultStrides();

  iyuvPicture->AllocateDataStore();
  vtkOpenGLCheckErrors("ERROR allocating gl texture. ");

  iyuvPicture->Capture(renWin);
  vtkOpenGLCheckErrors("ERROR capturing render window. ");

  iyuvPicture->Render(renWin);
  vtkOpenGLCheckErrors("ERROR rendering iyuv. ");

  // we cannot use vtkRegressionTest macro because it re-renders and reads the front/back buffer.
  // both of which will not have iyuvPicture overlay.

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

  vtkNew<vtkImageDifference> imageDiff;
  imageDiff->SetInputData(testImage);
  imageDiff->SetImageData(baseline);
  imageDiff->SetThreshold(15);
  imageDiff->Update();

  cout << "Threshold diff " << imageDiff->GetThresholdedError();
  return imageDiff->GetThresholdedError() < 0.5 ? 0 : 1;
  return 0;
}
