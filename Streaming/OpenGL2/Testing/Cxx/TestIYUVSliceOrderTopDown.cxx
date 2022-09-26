/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIYUVSliceOrderTopDown.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises the shader programs that draw IYUV top-down pictures.

#include "vtkCPUVideoFrame.h"
#include "vtkCylinderSource.h"
#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkLogger.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPNGReader.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestIYUVSliceOrderTopDown(int argc, char* argv[])
{
  bool success = true;
  const int width = 320, height = 240;
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "cars_320x240.iyuv");
  vtkLogF(INFO, "Read %s", filename);
  std::ifstream fpIn(filename, std::ifstream::in | std::ifstream::binary);
  if (!fpIn)
  {
    vtkLogF(ERROR, "Unable to open %s", filename);
    return 1;
  }
  delete[] filename;

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;

  ren->SetBackground(0, 0, 0);
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  renWin->Initialize();
  // display blank screen.
  renWin->Render();

  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_IYUV);
  std::unique_ptr<uint8_t[]> pixels(new uint8_t[estSize]);
  std::streamsize numRead = fpIn.read(reinterpret_cast<char*>(pixels.get()), estSize).gcount();

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->SetContext(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  iyuvPicture->ComputeDefaultStrides();

  iyuvPicture->AllocateDataStore();
  vtkOpenGLCheckErrors("Error allocating gl texture. ");

  iyuvPicture->CopyData(pixels.get(), numRead);
  vtkOpenGLCheckErrors("Error uploading pixels to gl texture. ");

  iyuvPicture->Print(std::cout);

  iyuvPicture->Render(renWin);
  vtkOpenGLCheckErrors("Error rendering IYUV. ");

  // we cannot use vtkRegressionTest macro because it re-renders and reads the front/back buffer.
  // both of which will not have rgba32Picture overlay.

  vtkNew<vtkUnsignedCharArray> pixels2;
  renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, 1, pixels2);

  vtkNew<vtkPNGReader> reader;
  filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "cars_320x240_00.png");
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();

  vtkNew<vtkImageData> baseline;
  baseline->ShallowCopy(reader->GetOutput());

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
}
