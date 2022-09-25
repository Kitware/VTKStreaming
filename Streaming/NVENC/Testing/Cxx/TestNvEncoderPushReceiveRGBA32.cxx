/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderPushReceiveRGBA32.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises NvEnc h.264 encoder with RGBA32 inputs.

#include "vtkActor.h"
#include "vtkCPUVideoFrame.h"
#include "vtkCallbackCommand.h"
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
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVideoCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <fstream>
#include <iomanip>

#define WRITE_CHUNKS 0

int TestNvEncoderPushReceiveRGBA32(int argc, char* argv[])
{
  bool success = true;
  int width = 320, height = 240;
  vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_9);
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkCylinderSource> cyl;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkNamedColors> colors;

  mapper->SetInputConnection(cyl->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(colors->GetColor4d("Tomato").GetData());
  actor->RotateX(30.0);
  actor->RotateY(-45.0);

  ren->AddActor(actor);
  ren->SetBackground(0.2, 0.2, 0.2);

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(width, height);
  ren->SetBackground(0.5, 0.5, 0.5);
  iren->SetRenderWindow(renWin);
  iren->Initialize();
  iren->Render();

  vtkNew<vtkNvEncoderGL> enc;
  enc->InitializeOpenGLContext(renWin);
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  vtkNew<vtkOpenGLVideoFrame> dFrame;
  dFrame->SetContext(renWin);
  dFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  dFrame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::TopDown);

  vtkNew<vtkCallbackCommand> exitCallback;
  exitCallback->SetClientData(enc);
  exitCallback->SetCallback(
    [](vtkObject* iren_ptr, unsigned long, void* enc_ptr, void*)
    {
      // drain needs an opengl context so it can release the resources.
      auto encoder = reinterpret_cast<vtkVideoEncoder*>(enc_ptr);
      auto result = encoder->Drain();
      (void)result;
      auto iren = reinterpret_cast<vtkRenderWindowInteractor*>(iren_ptr);
      encoder->Shutdown();
      iren->TerminateApp();
    });
  iren->AddObserver(vtkCommand::ExitEvent, exitCallback);

#if !WRITE_CHUNKS
  std::ofstream outFile("cylinder.h264", std::ofstream::out | std::ofstream::binary);
#endif

  int frame = 0, lastw = 0, lasth = 0;
  while (true)
  {
    iren->ProcessEvents();
    if (iren->GetDone())
    {
      break;
    }
#if WRITE_CHUNKS
    std::stringstream filename;
    filename << "frame-" << std::setfill('0') << std::setw(3) << frame << ".bin";
    std::ofstream outFile(filename.str(), std::ofstream::out | std::ofstream::binary);
#endif
    width = renWin->GetSize()[0];
    height = renWin->GetSize()[1];
    if (width != lastw || height != lasth)
    {
      dFrame->SetWidth(width);
      dFrame->SetHeight(height);
      dFrame->ComputeDefaultStrides();
      dFrame->AllocateDataStore();
    }
    dFrame->Capture(renWin);
    lasth = height;
    lastw = width;
    vtkOpenGLCheckErrors("error uploading data to gl texture");

    auto status = enc->Push(dFrame);
    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(status));
    auto result = enc->GetResult();
    vtkLog(TRACE, << vtkVideoProcessingStatusTypeUtilities::ToString(result.first));
    outFile.write(reinterpret_cast<char*>(result.second[0]->GetData()->GetPointer(0)),
      result.second[0]->GetSize());
#if WRITE_CHUNKS
    outFile.close();
#endif
    ++frame;
  }
#if !WRITE_CHUNKS
  outFile.close();
#endif

  return 0;
}
