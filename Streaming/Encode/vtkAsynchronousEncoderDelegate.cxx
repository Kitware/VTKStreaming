/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousEncoderDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAsynchronousEncoderDelegate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <stdexcept>
#include <vtk_glew.h>

#define ENSURE_MAIN_THREAD                                                                         \
  do                                                                                               \
  {                                                                                                \
    if (this->Tid != std::this_thread::get_id())                                                   \
    {                                                                                              \
      vtkLog(ERROR, "Attempted to execute thread-unsafe code from worker thread.");                \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)

#define ENSURE_NOT_MAIN_THREAD                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (this->Tid == std::this_thread::get_id())                                                   \
    {                                                                                              \
      vtkLog(ERROR, "Attempted to execute thread-unsafe code from main thread");                   \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkAsynchronousEncoderDelegate);

//------------------------------------------------------------------------------
vtkAsynchronousEncoderDelegate::vtkAsynchronousEncoderDelegate()
  : TrySucceeded(false)
  , Tid(std::this_thread::get_id())
  , TaskQueue(1, "encode::worker")
{
}

//------------------------------------------------------------------------------
vtkAsynchronousEncoderDelegate::~vtkAsynchronousEncoderDelegate()
{
  auto finalizeGfxCtx = [this]() {
    ENSURE_NOT_MAIN_THREAD;
    if (this->WorkerContext != nullptr)
    {
      this->WorkerContext->Finalize();
    }
  };
  auto ctxDone = this->TaskQueue.Push(finalizeGfxCtx);
  ctxDone.wait();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "TrySucceeded: " << this->TrySucceeded << "\n";
  os << "Finished: " << this->AwaitableQ.IsEmpty() << "\n";
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Sustain(VTKVideoEncoderInputType input)
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;
  this->FrameSustainer.push(input);
  this->SustainedGB += (float(input->GetActualSize()) / (1024.f * 1024.f * 1024.f));
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Relinquish()
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;
  auto frame = this->FrameSustainer.front();
  if (frame->GetReferenceCount() == 2)
  {
    // the task queue did not yet release reference to the input.
    return;
  }
  this->FrameSustainer.pop();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::RelinquishAll()
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;
  while (!this->FrameSustainer.empty())
  {
    auto frame = this->FrameSustainer.front();
    if (frame->GetReferenceCount() == 2)
    {
      // the task queue did not yet release reference to the input.
      // flush the queue with empty task.
      auto fut = this->TaskQueue.Push([]() {});
      fut.wait();
    }
    this->FrameSustainer.pop();
  }
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::InitializeWorker(
  VTKVideoEncodeWorkerType workerFunc, vtkRenderWindow* mainGfxContext)
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;

  this->WorkerFunction = workerFunc;
  this->MainGfxContext = mainGfxContext;
  this->MainGfxContext->ReleaseCurrent();

  auto setupGfxCtx = [this]() {
    vtkLogScopeF(TRACE, "Setting up worker gfx context");
    ENSURE_NOT_MAIN_THREAD;
    if (this->WorkerContext != nullptr &&
      this->WorkerContext->GetSharedRenderWindow() != this->MainGfxContext)
    {
      this->WorkerContext = nullptr;
    }
    this->WorkerContext = vtk::TakeSmartPointer(this->MainGfxContext->NewInstance());
    this->WorkerContext->ShowWindowOff();
    this->WorkerContext->SetSharedRenderWindow(this->MainGfxContext);
    vtkOpenGLRenderWindow::SafeDownCast(this->WorkerContext)->Initialize();
  };

  auto ctxReady = this->TaskQueue.Push(setupGfxCtx);
  ctxReady.wait();

  this->MainGfxContext->MakeCurrent();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Flush()
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;

  while (!this->AwaitableQ.IsEmpty())
  {
    VTKVideoEncoderResultType result;
    this->AwaitableQ.Pop(result);
  }
  // we do not need the safety net anymore.
  this->RelinquishAll();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;

  this->Flush();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PushWorkUnit(VTKVideoEncoderInputType frame)
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;

  auto tlFrame = this->PrepareThreadLocalResources(frame);
  this->Sustain(tlFrame);
  this->AwaitableQ.PushFuture(
    this->TaskQueue.Push(&vtkAsynchronousEncoderDelegate::Execute, this, tlFrame));
}

//------------------------------------------------------------------------------
bool vtkAsynchronousEncoderDelegate::HasResult()
{
  ENSURE_MAIN_THREAD;
  this->TrySucceeded = this->AwaitableQ.TryPop(this->Result);
  this->Relinquish();
  return this->TrySucceeded;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkAsynchronousEncoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  ENSURE_MAIN_THREAD;
  if (this->TrySucceeded)
  {
    // reset it first.
    this->TrySucceeded = false;
    // the result already exists, give it out.
    return this->Result;
  }
  else if (this->AwaitableQ.IsEmpty())
  {
    vtkLog(TRACE, << "Need frames. Task queue empty.");
    return { VTKVideoProcessingStatusType::VTKVPStatus_EOFError, {} };
  }
  this->AwaitableQ.Pop(this->Result);
  this->Relinquish();

  return this->Result;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkAsynchronousEncoderDelegate::Execute(VTKVideoEncoderInputType frame)
{
  vtkLogScopeFunction(TRACE);
  ENSURE_NOT_MAIN_THREAD;

  VTKVideoEncoderResultType result = { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  auto tlFrame = this->PrepareThreadLocalResources(frame, /*shallow_copy=*/true);
  result = this->WorkerFunction(tlFrame);

  return result;
}

//------------------------------------------------------------------------------
VTKVideoEncoderInputType vtkAsynchronousEncoderDelegate::PrepareThreadLocalResources(
  VTKVideoEncoderInputType from, bool shallow_copy /*=false*/)
{
  vtkLogScopeF(TRACE, "%s, worker_context=%s, from=%s", __func__,
    vtkLogIdentifier(this->WorkerContext.Get()), vtkLogIdentifier(from));

  if (from == nullptr)
  {
    throw std::runtime_error("Input cannot be (nullptr)");
  }

  auto tlFrame = vtk::TakeSmartPointer(from->NewInstance());
  if (auto srcGLFrame = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    auto texture = reinterpret_cast<vtkTextureObject*>(srcGLFrame->GetResourceHandle());
    auto srcGLContext = texture->GetContext();
    auto dstGLFrame = vtkOpenGLVideoFrame::SafeDownCast(tlFrame);

    if (this->Tid != std::this_thread::get_id())
    {
      // we're in a worker thread, not safe to use same context as srcGLFrame.
      auto dstGLContext = vtkOpenGLRenderWindow::SafeDownCast(this->WorkerContext);
      dstGLFrame->SetContext(dstGLContext);
    }
    else
    {
      // we're on main thread, safe to use the same context as srcGLFrame.
      dstGLFrame->SetContext(srcGLContext);
    }
  }
  if (shallow_copy)
  {
    tlFrame->ShallowCopy(from);
  }
  else
  {
    tlFrame->DeepCopy(from);
  }
  return tlFrame;
}
