/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveRGBA32.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with RGBA32 inputs.

#include "vtkActor.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkTestUtilities.h"
#include "vtkVideoProcessingStatusTypes.h"

int TestNvEncoderGLPushReceiveRGBA32(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 320, height = 240;

  char* filename =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "movin_color_bars_320x240_64_frames.h264");
  std::string baselineFile = filename;
  delete[] filename;

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->SetSize(width, height);
  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetGraphicsContext(renWin);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->AsyncModeOff();
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  vtkNew<vtkOpenGLVideoFrame> rgba32Picture;
  rgba32Picture->SetContext(renWin);
  rgba32Picture->SetWidth(width);
  rgba32Picture->SetHeight(height);
  rgba32Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  rgba32Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  rgba32Picture->ComputeDefaultStrides();

  int shift = 0, frameId = 0;
  while (true)
  {
    auto pixels = vtk::TakeSmartPointer(
      vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height, shift++));
    std::vector<uint8_t> bitstream;

    if (shift >= 64)
    {
      auto result = enc->Drain();
      if (!result.second.empty())
      {
        auto data = result.second[0]->GetData()->GetPointer(0);
        auto size = result.second[0]->GetSize();
        for (std::size_t i = 0; i < size; ++i)
        {
          bitstream.push_back(data[i]);
        }
      }
      enc->Shutdown();
      break;
    }
    rgba32Picture->CopyData(pixels);
    vtkOpenGLCheckErrors("ERROR uploading data to gl texture");

    rgba32Picture->Render(renWin);

    auto status = enc->Push(rgba32Picture);
    vtkOpenGLCheckErrors("ERROR fetching data from gl texture");
    vtkLogF(TRACE, "Push - %s", vtkVideoProcessingStatusTypeUtilities::ToString(status));

    auto result = enc->GetResult();
    vtkLogF(TRACE, "GetResult - %s", vtkVideoProcessingStatusTypeUtilities::ToString(result.first));

    if (!result.second.empty() && result.second[0] != nullptr)
    {
      auto data = reinterpret_cast<char*>(result.second[0]->GetData()->GetPointer(0));
      auto size = result.second[0]->GetSize();
      vtkLogF(INFO, "Recv %d bytes", size);
      for (int i = 0; i < size; ++i)
      {
        bitstream.push_back(data[i]);
      }
    }
    if (frameId > 0)
    {
      success &= bitstream.size() > 200;
    }
    ++frameId;
  }
  return success ? 0 : 1;
}
