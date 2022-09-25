/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestJPEGEncoderPushReceiveRGBA32.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises JPEG video encoder with rgba32 inputs.
// It is disabled because it requres human intervention right now.
// The output chunks of this test will be inputs to TestJPEGDecoderPushReceiveRGBA32.

#include "vtkActor.h"
#include "vtkCPUVideoFrame.h"
#include "vtkCallbackCommand.h"
#include "vtkCylinderSource.h"
#include "vtkJPEGVideoEncoder.h"
#include "vtkLogger.h"
#include "vtkNamedColors.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVideoEncoder.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <fstream>
#include <iomanip>

#define WRITE_CHUNKS 1

int TestJPEGEncoderPushReceiveRGBA32(int argc, char* argv[])
{
  bool success = true;
  int width = 320, height = 240;

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

  vtkNew<vtkJPEGVideoEncoder> enc;
  enc->SetWidth(width);
  enc->SetHeight(height);
  enc->UseAsynchronousDelegateOn();
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  vtkNew<vtkOpenGLVideoFrame> dFrame;
  dFrame->SetContext(renWin);
  dFrame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  dFrame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);

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

  int frame = 0, lastw = 0, lasth = 0;
  while (true)
  {
    iren->ProcessEvents();
    if (iren->GetDone())
    {
      break;
      ;
    }
#if WRITE_CHUNKS
    std::stringstream filename;
    filename << "frame-" << std::setfill('0') << std::setw(3) << frame << ".jpeg";
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
    cout << "write " << filename.str() << " - " << result.second[0]->GetData()->GetNumberOfValues()
         << " bytes\n";
    outFile.write(reinterpret_cast<char*>(result.second[0]->GetData()->GetPointer(0)),
      result.second[0]->GetSize());
#if WRITE_CHUNKS
    outFile.close();
#endif
    ++frame;
  }

  return 0;
}
