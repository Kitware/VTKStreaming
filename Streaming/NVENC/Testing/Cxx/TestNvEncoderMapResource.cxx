/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNvEncoderMapResource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test exercises memory mapping procedure explicitly from host -> gpu
// and from cpu -> nvenc chip.

#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkNvEncoderGL.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderer.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkXOpenGLRenderWindow.h"

int TestNvEncoderMapResource(int argc, char* argv[])
{
  bool success = true;
  const int w = 480, h = 480;

  vtkNew<vtkXOpenGLRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.5, 0.5, 0.5);

  renWin->AddRenderer(ren);
  renWin->SetSize(w, h);

  renWin->Initialize();
  renWin->Render();

  vtkNew<vtkOpenGLVideoFrame> frame;
  frame->InitializeGraphicsResources(renWin);
  frame->SetWidth(w);
  frame->SetHeight(h);
  frame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
  frame->SetSliceOrderType(vtkRawVideoFrame::SliceOrderType::BottomUp);

  // setup data storage and capture render window contents.
  frame->ComputeDefaultStrides();
  frame->AllocateDataStore();
  frame->Capture(renWin);

  vtkNew<vtkNvEncoderGL> enc;
  enc->SetCodec(VTKVideoCodecType::VTKVC_H264);
  enc->InitializeOpenGLContext(renWin);
  enc->SetWidth(w);
  enc->SetHeight(h);
  enc->UseAsynchronousDelegateOn();
  enc->SetInputPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);

  success &= (enc->Push(frame) == VTKVideoProcessingStatusType::VTKVPStatus_Success);
  enc->Flush();
  enc->Shutdown();

  return success ? 0 : 1;
}
