/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderGLMapResource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises memory mapping procedure explicitly from host -> gpu
// and from cpu -> nvenc chip.

#include "vtkLogger.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

int TestNvEncoderGLMapResource(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  bool success = true;
  const int w = 480, h = 480;

  vtkNew<vtkRenderWindow> win;
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.5, 0.5, 0.5);

  auto renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  renWin->AddRenderer(ren);
  renWin->SetSize(w, h);

  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkOpenGLVideoFrame> frame;
  frame->SetContext(renWin);
  frame->SetWidth(w);
  frame->SetHeight(h);
  frame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);

  // setup data storage and capture render window contents.
  frame->ComputeDefaultStrides();
  frame->AllocateDataStore();
  frame->Capture(renWin);

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetOutputHandler([&success](VTKVideoEncoderResultType result)
    { success = (result.first == VTKVideoProcessingStatusType::VTKVPStatus_Success); });
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->SetGraphicsContext(renWin);
  enc->SetWidth(w);
  enc->SetHeight(h);
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  enc->Encode(frame);
  enc->Flush();
  enc->Shutdown();

  return success ? 0 : 1;
}
