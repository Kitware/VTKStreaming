// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
// This test exercises display encoding with libvpx vp9 CPU encoder
// with display captured in IYUV pixel format.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVpxEncoder.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <string>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestVpxEncoderPushReceiveDisplayIYUV(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  int width = 641, height = 953;
  int frameId = 0;
  std::vector<uint8_t> bitstream;

#if WRITE_BITSTREAM
  std::ofstream file("cyl_iyuv.vp9", std::ios::out | std::ios::binary);
#endif

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
  enc->SetCodec(VTKVideoCodecType::VTKVC_VP9);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_IYUV);

  vtkNew<vtkOpenGLVideoFrame> picture;
  picture->SetContext(renWin);
  picture->SetWidth(width);
  picture->SetHeight(height);
  picture->SetPixelFormat(VTKPixelFormatType::VTKPF_IYUV);
  picture->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);
  picture->AllocateDataStore();

  vtkNew<vtkCallbackCommand> exitCallback;
  exitCallback->SetClientData(enc);
  exitCallback->SetCallback(
    [](vtkObject* iren_ptr, unsigned long, void* enc_ptr, void*)
    {
      // drain needs an opengl context so it can release the resources.
      auto encoder = reinterpret_cast<vtkVideoEncoder*>(enc_ptr);
      encoder->Drain();
      auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(iren_ptr);
      encoder->Shutdown();
      iren->TerminateApp();
    });
  iren->AddObserver(vtkCommand::ExitEvent, exitCallback);

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
    picture->Capture(renWin);
    enc->Encode(picture);
    vtkLogF(INFO, "Sent %d bytes", picture->GetActualSize());
  }
  return success ? 0 : 1;
}
