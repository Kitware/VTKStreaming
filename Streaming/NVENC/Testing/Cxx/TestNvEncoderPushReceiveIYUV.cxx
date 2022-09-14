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
// This test exercises NvEnc h.264 encoder with IYUV inputs.

#include "vtkActor.h"
#include "vtkCPUVideoFrame.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkXOpenGLRenderWindow.h"

#include <fstream>

int TestNvEncoderPushReceiveIYUV(int argc, char* argv[])
{
  bool success = true;
  const int width = 320, height = 240;

  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cars_320x240.iyuv");
  vtkLogF(INFO, "Read %s", filename);
  std::ifstream fpIn(filename, std::ifstream::in | std::ifstream::binary);
  if (!fpIn)
  {
    vtkLogF(ERROR, "Unable to open %s", filename);
    return 1;
  }
  delete[] filename;

  vtkNew<vtkXOpenGLRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->SetSize(width, height);
  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkNvEncoderGL> enc;
  enc->InitializeOpenGLContext(renWin);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->BypassDelegateOn();
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_IYUV);

  vtkNew<vtkOpenGLVideoFrame> iyuvPicture;
  iyuvPicture->InitializeGraphicsResources(renWin);
  iyuvPicture->SetWidth(width);
  iyuvPicture->SetHeight(height);
  iyuvPicture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  iyuvPicture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  iyuvPicture->ComputeDefaultStrides();

  std::ofstream outFile("cars.h264", std::ofstream::out | std::ofstream::binary);
  auto estSize = vtkRawVideoFrame::GetEstimatedSize(width, height, VTKPixelFormatType::VTKPF_IYUV);
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
    iyuvPicture->CopyData(pixels.get(), numRead);
    vtkOpenGLCheckErrors("ERROR uploading data to gl texture");

    iyuvPicture->Render(renWin);

    auto status = enc->Push(iyuvPicture);
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
