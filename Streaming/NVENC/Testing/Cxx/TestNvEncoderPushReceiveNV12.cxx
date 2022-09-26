/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderPushReceiveH264.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with NV12 inputs.

#include "vtkActor.h"
#include "vtkCPUVideoFrame.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <fstream>

int TestNvEncoderPushReceiveNV12(int argc, char* argv[])
{
  bool success = true;
  const int width = 320, height = 240;

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cars_320x240.nv12");
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
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_NV12);

  vtkNew<vtkOpenGLVideoFrame> nv12Picture;
  nv12Picture->SetContext(renWin);
  nv12Picture->SetWidth(width);
  nv12Picture->SetHeight(height);
  nv12Picture->SetPixelFormat(VTKPixelFormatType::VTKPF_NV12);
  nv12Picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  nv12Picture->ComputeDefaultStrides();

  std::ofstream outFile("cars.h264", std::ofstream::out | std::ofstream::binary);
  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_NV12);
  while (true)
  {
    std::unique_ptr<uint8_t[]> pixels(new uint8_t[estSize]);
    std::streamsize numRead = fpIn.read(reinterpret_cast<char*>(pixels.get()), estSize).gcount();

    if (numRead != estSize)
    {
      auto result = enc->Drain();
      if (!result.second.empty())
      {
        outFile.write(reinterpret_cast<char*>(result.second[0]->GetData()->GetPointer(0)),
          result.second[0]->GetSize());
        outFile.close();
      }
      enc->Shutdown();
      outFile.close();
      break;
    }
    nv12Picture->CopyData(pixels.get(), numRead);
    vtkOpenGLCheckErrors("ERROR uploading data to gl texture");

    nv12Picture->Render(renWin);

    auto status = enc->Push(nv12Picture);
    vtkOpenGLCheckErrors("ERROR fetching data from gl texture");
    vtkLogF(TRACE, "Push - %s", vtkVideoProcessingStatusTypeUtilities::ToString(status));

    auto result = enc->GetResult();
    vtkLogF(TRACE, "GetResult - %s", vtkVideoProcessingStatusTypeUtilities::ToString(result.first));

    if (!result.second.empty() && result.second[0] != nullptr)
    {
      auto data = reinterpret_cast<char*>(result.second[0]->GetData()->GetPointer(0));
      auto size = result.second[0]->GetSize();
      outFile.write(data, size);
      vtkLogF(TRACE, "Wrote %d bytes", size);
    }
  }

  return 0;
}
