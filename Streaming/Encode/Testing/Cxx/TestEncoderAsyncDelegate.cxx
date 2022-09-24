/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEncoderDelegateUsage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test demonstrates the arrival of packets when a video encoder uses
// asynchronous delegate. The mock video encoder simulates an encoder lag
// by sleeping for a given time interval. You can increase the mock lag inteval
// and notice that arrival of packets is interspersed with Push calls.

#include "vtkCallbackCommand.h"
#include "vtkLogger.h"
#include "vtkMockVideoEncoder.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

#include <chrono>
#include <thread>
#include <vtkOpenGLRenderWindow.h>

#define MAX_NUM_FRAMES 100

static int readyCount = 0;
void callback(vtkObject*, unsigned long, void*, void*)
{
  vtkLogF(INFO, "=> ready %d", readyCount++);
}

int TestEncoderAsyncDelegate(int argc, char* argv[])
{
  vtkNew<vtkMockVideoEncoder> encoder;
  bool success = true;
  // deliberately use OpenGL video frame to ensure we don't do anything bad with the context.
  std::vector<vtkNew<vtkOpenGLVideoFrame>> frames(MAX_NUM_FRAMES);

  encoder->SetMockEncodeTimeMilliseconds(MAX_NUM_FRAMES / 10);
  encoder->SetMockLargeFramePeriod(MAX_NUM_FRAMES / 20);
  encoder->SetMockLargeFrameIntervalRatio(MAX_NUM_FRAMES / 2);

  encoder->UseAsynchronousDelegateOn();

  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkRenderer> renderer;
  window->AddRenderer(renderer);
  window->Render();

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(callback);
  encoder->AddObserver(vtkCommand::ProgressEvent, cmd);

  int pushCount = 0;
  for (const auto& frame : frames)
  {
    frame->SetWidth(4096);
    frame->SetHeight(2160);
    frame->SetPixelFormat(VTKPixelFormatType::VTKPF_RGBA32);
    frame->SetContext(vtkOpenGLRenderWindow::SafeDownCast(window));
    frame->ComputeDefaultStrides();
    frame->AllocateDataStore();
    vtkLogF(INFO, "Push %d", pushCount++);
    encoder->Push(frame);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  encoder->Flush();
  success = readyCount == pushCount;
  if (!success)
  {
    vtkLogF(ERROR, "Failed readyCount != pushCount | %d != %d", readyCount, pushCount);
    return 1;
  }

  return 0;
}
