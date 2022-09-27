/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestEncoderAsyncDelegateMultiCurrentContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test rigorously verifies that the async encoder delegate
// indeed uses a separate thread-local OpenGL context shared
// with the main thread's OpenGL context.
// Similar to a real-life use case - live encoding VTK renders.

#include "vtkCallbackCommand.h"
#include "vtkLogger.h"
#include "vtkMockVideoEncoder.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStreamingTestUtility.h"
#include "vtkVideoEncoder.h"

#include <chrono>
#include <thread>

#define MAX_NUM_FRAMES 64

static int readyCount = 0;
void TestEncoderAsyncDelegateMultiCurrentContext_callback(
  vtkObject* enc, unsigned long, void*, void*)
{
  vtkLogF(INFO, "=> ready %d", readyCount++);
  auto encoder = vtkVideoEncoder::SafeDownCast(enc);
}

int TestEncoderAsyncDelegateMultiCurrentContext(int argc, char* argv[])
{
  vtkStreamingTestUtility::SetLoggerVerbosityFromCli(argc, argv);
  vtkNew<vtkMockVideoEncoder> encoder;
  bool success = true;
  // deliberately use OpenGL video frame to ensure we don't do anything bad with the context.
  std::vector<vtkNew<vtkOpenGLVideoFrame>> frames(MAX_NUM_FRAMES);

  encoder->SetMockEncodeTimeMilliseconds(1);
  encoder->SetMockLargeFramePeriod(MAX_NUM_FRAMES / 4);
  encoder->SetMockLargeFrameIntervalRatio(2);

  encoder->AsyncModeOn();

  vtkNew<vtkRenderWindow> window;
  vtkNew<vtkRenderer> renderer;
  window->AddRenderer(renderer);
  window->Render();

  vtkNew<vtkCallbackCommand> cmd;
  cmd->SetCallback(TestEncoderAsyncDelegateMultiCurrentContext_callback);
  encoder->AddObserver(vtkCommand::ProgressEvent, cmd);

  encoder->SetGraphicsContext(window);
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
    window->Render();
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
