// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
// This test exercises the shader programs that capture iyuv from a render window.

#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkLogger.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

int TestIYUVBottomUpCapture(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 240, height = 240;
  auto pixels =
    vtk::TakeSmartPointer(vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height));
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;

  ren->SetBackground(0.2, 0.2, 0.2);
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  vtkNew<vtkOpenGLVideoFrame> rgba32Picture;
  rgba32Picture->SetContext(renWin);
  rgba32Picture->SetWidth(width);
  rgba32Picture->SetHeight(height);
  rgba32Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  rgba32Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
  rgba32Picture->ComputeDefaultStrides();
  rgba32Picture->CopyData(pixels, width * 4, height);
  rgba32Picture->Render(renWin);

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->SetContext(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);
  iyuvPicture->ComputeDefaultStrides();

  iyuvPicture->AllocateDataStore();

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

  vtkLog(INFO, << "Threshold diff " << imageDiff->GetThresholdedError());

  if (vtkStreamingTestUtility::GetInteractive(argc, argv))
  {
    int frameId = 0;
    while (true)
    {
      iren->ProcessEvents();
      if (iren->GetDone())
      {
        break;
      }
      const int width = renWin->GetSize()[0];
      const int height = renWin->GetSize()[1];
      rgba32Picture->SetWidth(width);
      rgba32Picture->SetHeight(height);
      rgba32Picture->ComputeDefaultStrides();
      pixels = vtk::TakeSmartPointer(
        vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height, frameId++));
      rgba32Picture->CopyData(pixels, width * 4, height);
      rgba32Picture->Render(renWin);
      iyuvPicture->Capture(renWin);
      iyuvPicture->Render(renWin);
    }
  }
  return imageDiff->GetThresholdedError() < 0.5 ? 0 : 1;
}
