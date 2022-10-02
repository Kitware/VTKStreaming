/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveIYUV.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with IYUV inputs.

#include "vtkActor.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkTestUtilities.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <cstdint>
#include <fstream>
#include <ios>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestNvEncoderGLPushReceiveIYUV(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
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
  filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "cars_320x240.h264");
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
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_IYUV);

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->SetContext(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  iyuvPicture->ComputeDefaultStrides();

  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_IYUV);
  int frameId = 0;
#if WRITE_BITSTREAM
  std::ofstream file("cars_320x240_iyuv.h264", std::ios::out | std::ios::binary);
#endif
  while (true)
  {
    std::unique_ptr<uint8_t[]> pixels(new uint8_t[estSize]);
    std::streamsize numRead = fpIn.read(reinterpret_cast<char*>(pixels.get()), estSize).gcount();
    std::vector<uint8_t> bitstream;

    if (numRead != estSize)
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
    iyuvPicture->CopyData(pixels.get(), width, height + ((height + 1) >> 1));
    iyuvPicture->Render(renWin);

    auto status = enc->Push(iyuvPicture);
    vtkLogF(INFO, "Sent %d bytes", iyuvPicture->GetActualSize());
    if (status == VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain)
    {
      continue;
    }
    auto result = enc->GetResult();
    for (const auto& packet : result.second)
    {
      auto data = reinterpret_cast<char*>(packet->GetData()->GetPointer(0));
      auto size = packet->GetSize();
      vtkLogF(INFO, "Recvd %d bytes", size);
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
