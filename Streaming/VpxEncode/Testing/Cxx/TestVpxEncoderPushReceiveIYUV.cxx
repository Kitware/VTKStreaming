/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVpxEncoderPushReceiveIYUV.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises libvpx vp9 encoder with IYUV inputs.

#include "vtkActor.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
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
#include "vtkVideoProcessingWorkUnitTypes.h"
#include "vtkVpxEncoder.h"

#include <cstdint>
#include <fstream>
#include <ios>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestVpxEncoderPushReceiveIYUV(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 320, height = 240;
  int frameId = 0;
  std::vector<uint8_t> bitstream;

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "cars_320x240.iyuv");
  vtkLogF(INFO, "Read %s", filename);
  std::ifstream fpIn(filename, std::ifstream::in | std::ifstream::binary);
  if (!fpIn)
  {
    vtkLogF(ERROR, "Unable to open %s", filename);
    return 1;
  }
  delete[] filename;

#if WRITE_BITSTREAM
  std::ofstream file("cars_320x240_iyuv.vp9", std::ios::out | std::ios::binary);
#endif

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->SetSize(width, height);
  renWin->Initialize();
  renWin->Render();

  auto writeBitstream =
#if WRITE_BITSTREAM
    [&file, &bitstream, &frameId, &success]
#else
    [&bitstream, &frameId, &success]
#endif
    (VTKVideoEncoderResultType result)
  {
    bitstream.clear();
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
      if (frameId > 0)
      {
        assert(bitstream.size() > 10);
        success &= bitstream.size() > 10;
      }
      ++frameId;
    }
  };

  vtkNew<vtkVpxEncoder> enc;
  enc->SetOutputHandler(writeBitstream);
  enc->SetGraphicsContext(renWin);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->SetCodec(VTKVideoCodecType::VTKVC_VP9);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_IYUV);

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->SetContext(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  iyuvPicture->ComputeDefaultStrides();
  iyuvPicture->AllocateDataStore();

  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_IYUV);
  while (true)
  {
    std::unique_ptr<uint8_t[]> pixels(new uint8_t[estSize]);
    std::streamsize numRead = fpIn.read(reinterpret_cast<char*>(pixels.get()), estSize).gcount();
    std::vector<uint8_t> bitstream;

    if (numRead != estSize)
    {
      // drain out remaining packets
      enc->Drain();
      break;
    }
    iyuvPicture->CopyPlanarData(pixels.get(), width, height, 0);
    iyuvPicture->CopyPlanarData(pixels.get() + width * height, (width >> 1), (height + 1) >> 1, 1);
    iyuvPicture->CopyPlanarData(
      pixels.get() + (width * height * 5 / 4), (width >> 1), (height + 1) >> 1, 2);
    iyuvPicture->Render(renWin);

    enc->Encode(iyuvPicture);
    vtkLogF(INFO, "Sent %d bytes", iyuvPicture->GetActualSize());
  }
  return success ? 0 : 1;
}
