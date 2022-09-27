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

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "movin_color_bars_320x240_64_frames.h264");
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

  std::vector<uint8_t> bitstream;
  int shift = 0;
  while (true)
  {
    auto pixels = vtk::TakeSmartPointer(
      vtkStreamingTestUtility::GenerateRGBA32ColorBars(width, height, shift++));

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
      auto data = result.second[0]->GetData()->GetPointer(0);
      auto size = result.second[0]->GetSize();
      for (std::size_t i = 0; i < size; ++i)
      {
        bitstream.push_back(data[i]);
      }
      vtkLogF(TRACE, "Wrote %d bytes", size);
    }
  }

  std::ifstream baseline;
  vtkLogF(INFO, "Read %s", baselineFile.c_str());
  baseline.open(baselineFile, std::ios::in | std::ios::binary);
  baseline.ignore(std::numeric_limits<std::streamsize>::max());

  const std::size_t size1 = baseline.gcount();
  const std::size_t size2 = bitstream.size();
  vtkLogF(TRACE, "%zu, %zu", size1, bitstream.size());
  success = bitstream.size() == size1;

  baseline.clear();
  baseline.seekg(0, std::ios_base::beg);

  std::vector<uint8_t> baseline_ptr(size1, 0);
  baseline.read(reinterpret_cast<char*>(baseline_ptr.data()), size1);

  for (std::size_t i1 = 0, i2 = 0; i1 < size1 && i2 < size2 && success; ++i1 && ++i2)
  {
    vtkLogF(TRACE, "%d, %d", int(baseline_ptr[i1]), int(bitstream[i2]));
    success &= (baseline_ptr[i1] == bitstream[i2]);
  }
  if (!success)
  {
    filename = vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(
      "-T", argc, argv, "VTKSTREAMING_DATA_ROOT", "Temporary", "movin_color_bars_320x240_64_frames.h264");
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<char*>(bitstream.data()), bitstream.size());
  }
  return success ? 0 : 1;
}
