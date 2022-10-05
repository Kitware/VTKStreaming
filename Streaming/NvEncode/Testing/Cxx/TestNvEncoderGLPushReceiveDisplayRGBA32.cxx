/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveDisplayRGBA32.cxx

  Copyright (c) 2022 Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises zero-copy display encoding with NvEnc h.264 OpenGL based encoder
// with display captured in RGBA32 pixel format.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <ios>
#include <string>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestNvEncoderGLPushReceiveDisplayRGBA32(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  int width = 647, height = 953;

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkCylinderSource> cyl;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkNamedColors> colors;

  // Set the background color.
  std::array<unsigned char, 4> bkg{ { 26, 51, 102, 255 } };
  colors->SetColor("BkgColor", bkg.data());

  mapper->SetInputConnection(cyl->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(colors->GetColor4d("Tomato").GetData());
  actor->RotateX(30.0);
  actor->RotateY(-45.0);

  ren->AddActor(actor);
  ren->SetBackground(colors->GetColor3d("BkgColor").GetData());

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Render();

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetGraphicsContext(renWin);
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->AsyncModeOff();
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  vtkNew<vtkCallbackCommand> exitCallback;
  exitCallback->SetClientData(enc);
  exitCallback->SetCallback([](vtkObject* iren_ptr, unsigned long, void* enc_ptr, void*) {
    // drain needs an opengl context so it can release the resources.
    auto encoder = reinterpret_cast<vtkVideoEncoder*>(enc_ptr);
    auto result = encoder->Drain();
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
      std::ofstream file("cyl_rgba.h264", std::ios::app | std::ios::binary);
      file.write((char*)bitstream.data(), bitstream.size());
#endif
    }
    (void)result;
    auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(iren_ptr);
    encoder->Shutdown();
    iren->TerminateApp();
  });
  iren->AddObserver(vtkCommand::ExitEvent, exitCallback);
#if WRITE_BITSTREAM
  std::ofstream file("cyl_rgba.h264", std::ios::out | std::ios::binary);
#endif
  int frameId = 0;
  while (true)
  {
    double azimuth = (frameId % 36) * 10;
    std::vector<uint8_t> bitstream;
    ren->GetActiveCamera()->Azimuth(vtkMath::RadiansFromDegrees(azimuth));
    renWin->Render();
    if (frameId > 100)
    {
      iren->ExitEvent();
    }
    if (iren->GetDone())
    {
      break;
    }

    auto result = enc->EncodeDisplay();
    if (result.first == VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain)
    {
      continue;
    }
    vtkLogF(
      TRACE, "EncodeDisplay - %s", vtkVideoProcessingStatusTypeUtilities::ToString(result.first));
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
    success &= bitstream.size() > 10;
    ++frameId;
  }

  return success ? 0 : 1;
}
