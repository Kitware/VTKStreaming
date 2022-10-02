/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLPushReceiveDisplayNV12.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises zero-copy display encoding with NvEnc h.264 OpenGL based encoder
// with display captured in NV12 pixel format.

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCylinderSource.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
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
#include <string>

#ifndef WRITE_BITSTREAM
#define WRITE_BITSTREAM 1
#endif

int TestNvEncoderGLPushReceiveDisplayNV12(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  int width = 641, height = 953;

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
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_NV12);

  vtkNew<vtkCallbackCommand> exitCallback;
  exitCallback->SetClientData(enc);
  exitCallback->SetCallback([](vtkObject* iren_ptr, unsigned long, void* enc_ptr, void*) {
    // drain needs an opengl context so it can release the resources.
    auto encoder = reinterpret_cast<vtkVideoEncoder*>(enc_ptr);
    auto result = encoder->Drain();
    (void)result;
    auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(iren_ptr);
    encoder->Shutdown();
    iren->TerminateApp();
  });
  iren->AddObserver(vtkCommand::ExitEvent, exitCallback);

  int frameId = 0;
#if WRITE_BITSTREAM
  std::ofstream file("cyl_nv12.h264", std::ios::out | std::ios::binary);
#endif
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
    vtkOpenGLCheckErrors("error uploading data to gl texture");

    auto result = enc->EncodeDisplay();

    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(result.first));
    success &= !(result.second.empty() || result.second[0] == nullptr);
    if (!success)
    {
      vtkLog(ERROR, "Empty encoder result");
      break;
    }
    auto data = result.second[0]->GetData()->GetPointer(0);
    auto size = result.second[0]->GetSize();
    vtkLogF(INFO, "Recv %d bytes", size);
    for (std::size_t i = 0; i < size; ++i)
    {
      bitstream.push_back(data[i]);
    }
    success &= bitstream.size() > 200;
#if WRITE_BITSTREAM
    file.write((char*)bitstream.data(), bitstream.size());
#endif
    ++frameId;
  }
  return success ? 0 : 1;
}
