/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveRGBA32.cxx

  Copyright (c) 2022 Kitware, Inc.
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
#include "vtkVideoProcessingStatusTypes.h"

#include <cstdint>
#include <fstream>
#include <ios>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestNvEncoderGLPushReceiveRGBA32(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 333, height = 429;

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
  rgba32Picture->Print(std::cout);

  int shift = 0, frameId = 0;
#if WRITE_BITSTREAM
  std::ofstream file("moving_bars_rgba.h264", std::ios::out | std::ios::binary);
#endif
  while (true)
  {
    auto pixels = vtk::TakeSmartPointer(
      vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height, shift++));
    std::vector<uint8_t> bitstream;

    if (shift >= 64)
    {
      // drain out remaining packets
      auto result = enc->Drain();
      std::vector<uint8_t> bitstream;
      for (const auto& packet : result.second)
      {
        auto data = reinterpret_cast<char*>(packet->GetData()->GetPointer(0));
        auto size = packet->GetSize();
        vtkLogF(INFO, "Recvd %d bytes", size);
        for (int i = 0; i < size; ++i)
        {
          bitstream.push_back(data[i]);
        }
#if WRITE_BITSTREAM
        file.write((char*)bitstream.data(), bitstream.size());
#endif
      }
      enc->Shutdown();
      break;
    }
    rgba32Picture->CopyData(pixels, width * 4, height);
    rgba32Picture->Render(renWin);

    auto status = enc->Push(rgba32Picture);
    vtkLogF(INFO, "Sent %d bytes", rgba32Picture->GetActualSize());
    if (status == VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain)
    {
      continue;
    }
    auto result = enc->GetResult();
    vtkLogF(TRACE, "GetResult - %s", vtkVideoProcessingStatusTypeUtilities::ToString(result.first));
    for (const auto& packet : result.second)
    {
      auto data = reinterpret_cast<char*>(packet->GetData()->GetPointer(0));
      auto size = packet->GetSize();
      for (int i = 0; i < size; ++i)
      {
        bitstream.push_back(data[i]);
      }
    }
#if WRITE_BITSTREAM
    file.write((char*)bitstream.data(), bitstream.size());
#endif
    if (frameId > 0)
    {
      assert(bitstream.size() > 10);
      success &= bitstream.size() > 10;
    }
    ++frameId;
  }
  return success ? 0 : 1;
}
