/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveNV12.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with NV12 inputs.

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
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <cstdint>
#include <fstream>
#include <ios>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestNvEncoderGLPushReceiveNV12(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int width = 320, height = 240;
  int frameId = 0;
  std::vector<uint8_t> bitstream;

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "cars_320x240.nv12");
  vtkLogF(INFO, "Read %s", filename);
  std::ifstream fpIn(filename, std::ifstream::in | std::ifstream::binary);
  if (!fpIn)
  {
    vtkLogF(ERROR, "Unable to open %s", filename);
    return 1;
  }
  delete[] filename;

#if WRITE_BITSTREAM
  std::ofstream file("cars_320x240_nv12.h264", std::ios::out | std::ios::binary);
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
    (VTKVideoEncoderResultType result) {
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

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetOutputHandler(writeBitstream);
  enc->SetGraphicsContext(renWin);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_NV12);

  vtkNew<vtkOpenGLVideoFrame> nv12Picture;
  nv12Picture->SetContext(renWin);
  nv12Picture->SetWidth(width);
  nv12Picture->SetHeight(height);
  nv12Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_NV12);
  nv12Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  nv12Picture->ComputeDefaultStrides();
  nv12Picture->AllocateDataStore();

  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_NV12);
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
    nv12Picture->CopyPlanarData(pixels.get(), width, height, 0);
    nv12Picture->CopyPlanarData(pixels.get() + width * height, width, (height + 1) >> 1, 1);
    nv12Picture->Render(renWin);

    enc->Encode(nv12Picture);
    vtkLogF(INFO, "Sent %d bytes", nv12Picture->GetActualSize());
  }
  return success ? 0 : 1;
}
