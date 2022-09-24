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
#include "vtkCPUVideoFrame.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkVideoProcessingStatusTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <vtk_glew.h>

#include <functional>

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
  : TaskQueue(nullptr)
  , TrySucceeded(false)
  , Tid(std::this_thread::get_id())
  , TaskId(0)
  , NumberOfPendingTasks(0)
{
  this->ResourcesInitialized = false;
  this->InitializeResourcesNow = false;
  this->ResourcesDestroyed = false;
  this->DestroyResourcesNow = false;
}

//------------------------------------------------------------------------------
vtkAsynchronousEncoderDelegate::~vtkAsynchronousEncoderDelegate() = default;

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "TrySucceeded: " << this->TrySucceeded << "\n";
  os << "TaskId : " << this->TaskId << "\n";
  os << "NumberOfPendingTasks : " << this->NumberOfPendingTasks << "\n";
  os << "TaskQueueIsEmpty: " << this->TaskQueue->IsEmpty() << "\n";
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Sustain(VTKVideoEncoderInputType input)
{
  vtkLogScopeF(TRACE, "%s", __func__);
  ENSURE_MAIN_THREAD;
  this->FrameSustainer.push(input);
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Relinquish()
{
  vtkLogScopeF(TRACE, "%s", __func__);
  ENSURE_MAIN_THREAD;
  auto frame = this->FrameSustainer.front();
  this->FrameSustainer.pop();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::RelinquishAll()
{
  vtkLogScopeF(TRACE, "%s", __func__);
  ENSURE_MAIN_THREAD;
  while (!this->FrameSustainer.empty())
  {
    this->Relinquish();
  }
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::InitializeWorker(
  VTKVideoEncodeWorkerType workerFunc, vtkRenderWindow* mainGfxContext)
{
  using namespace std::placeholders; // for _1;
  auto taskFunc = std::bind(&vtkAsynchronousEncoderDelegate::TaskExecute, this, _1);

  this->TaskQueue.reset(new TaskQueueType(taskFunc, true, -1, static_cast<int>(1)));
  this->TaskId = 0;
  this->NumberOfPendingTasks = 0;
  this->WorkerFunction = workerFunc;
  this->MainGfxContext = mainGfxContext;

  this->ResourcesInitialized = false;
  this->InitializeResourcesNow = false;
  this->ResourcesDestroyed = false;
  this->DestroyResourcesNow = false;
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Flush()
{
  if (this->TaskQueue != nullptr)
  {
    this->TaskQueue->Flush();
  }
  // we do not need the safety net anymore.
  this->RelinquishAll();
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::Terminate()
{
  vtkLogScopeFunction(TRACE);
  if (this->TaskQueue != nullptr)
  {
    vtkLog(TRACE, << "Flushing the task queue ...");
    // if task queue is empty, push a placeholder work unit.
    if (this->TaskQueue->IsEmpty())
    {
      this->TaskQueue->Push(nullptr);
    }
    // signal the worker thread to destroy resources.
    this->ResourcesDestroyed = false;
    this->DestroyResourcesNow = true;
    this->TaskQueue->Flush();
    // we do not need the safety net anymore.
    this->RelinquishAll();

    // wait until resources are destroyed before terminating the thread.
    std::unique_lock<std::mutex> destroyLock(this->DestroyResourcesMtx);
    this->DestroyResourcesCV.wait(destroyLock, [this] { return this->ResourcesDestroyed.load(); });
    this->DestroyResourcesNow = false;
  }

  this->TaskQueue.reset(nullptr);
  this->TaskId = 0;
  this->NumberOfPendingTasks = 0;
  this->MainGfxContext = nullptr;
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PushWorkUnit(vtkRawVideoFrame* frame)
{
  vtkLogScopeFunction(TRACE);
  vtkSmartPointer<vtkRawVideoFrame> dstFrame;
  this->PrepareThreadLocalResources(frame, dstFrame);
  glFlush();

  this->Sustain(dstFrame);
  this->TaskQueue->Push(std::move(dstFrame));

  this->PreTaskExecute();
  this->NumberOfPendingTasks += 1;
}

//------------------------------------------------------------------------------
bool vtkAsynchronousEncoderDelegate::HasResult()
{
  this->TrySucceeded = this->TaskQueue->TryPop(this->Result);
  this->Relinquish();
  return this->TrySucceeded;
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkAsynchronousEncoderDelegate::GetResult()
{
  vtkLogScopeFunction(TRACE);
  if (this->TrySucceeded)
  {
    // reset it first.
    this->TrySucceeded = false;
    // the result already exists, give it out.
    return this->Result;
  }
  else if (this->TaskQueue->IsEmpty())
  {
    vtkLog(WARNING, << "Need frames. Task queue empty.") return {
      VTKVideoProcessingStatusType::VTKVPStatus_EOFError, {}
    };
  }
  this->TaskQueue->Pop(this->Result);
  this->Relinquish();

  return this->Result;
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PreTaskExecute()
{
  vtkLogScopeF(TRACE, "%s", __func__);
  if (this->TaskId == 0)
  {
    // for the very first task.
    this->PostInitializeWorker();
  }
}

//------------------------------------------------------------------------------
VTKVideoEncoderResultType vtkAsynchronousEncoderDelegate::TaskExecute(
  VTKVideoEncoderInputType frame)
{
  vtkLogScopeF(TRACE, "%s", __func__);
  ENSURE_NOT_MAIN_THREAD;

  this->PreTaskExecute();

  VTKVideoEncoderResultType result = { VTKVideoProcessingStatusType::VTKVPStatus_UnknownError, {} };
  if (frame != nullptr)
  {
    VTKVideoEncoderInputType threadLocalFrame;
    this->PrepareThreadLocalResources(frame, threadLocalFrame, /*shallow_copy=*/true);
    result = this->WorkerFunction(threadLocalFrame);
    this->NumberOfPendingTasks = this->NumberOfPendingTasks - 1;
  }

  this->PostTaskExecute();

  return result;
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PostTaskExecute()
{
  vtkLogScopeF(TRACE, "%s", __func__);
  if (this->NumberOfPendingTasks == 0 && this->DestroyResourcesNow)
  {
    // we're about to be terminated.
    this->PreTerminateWorker();
  }
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PrepareThreadLocalResources(
  vtkRawVideoFrame* from, VTKVideoEncoderInputType& dstFrame, bool shallow_copy /*=false*/)
{
  vtkLogScopeF(TRACE, "%s, worker_context=%s, from=%s", __func__,
    vtkLogIdentifier(this->WorkerContext.Get()), vtkLogIdentifier(from));

  if (from == nullptr)
  {
    return;
  }

  if (this->Tid != std::this_thread::get_id())
  {
    // only increment this on the worker because it is the worker who executes tasks.
    this->TaskId += 1;
  }

  dstFrame = vtk::TakeSmartPointer(from->NewInstance());
  if (auto srcGLFrame = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    auto texture = reinterpret_cast<vtkTextureObject*>(srcGLFrame->GetResourceHandle());
    auto srcGLContext = texture->GetContext();
    auto dstGLFrame = vtkOpenGLVideoFrame::SafeDownCast(dstFrame);

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
    dstFrame->ShallowCopy(from);
  }
  else
  {
    dstFrame->DeepCopy(from);
  }
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PostInitializeWorker()
{
  vtkLogScopeF(TRACE, "%s, worker_context=%s, main_gfx_context=%s", __func__,
    vtkLogIdentifier(this->WorkerContext), vtkLogIdentifier(this->MainGfxContext));
  if (auto srcGLContext = vtkOpenGLRenderWindow::SafeDownCast(this->MainGfxContext))
  {
    if (this->Tid == std::this_thread::get_id())
    {
      srcGLContext->ReleaseCurrent();
      this->ResourcesInitialized = false;
      this->InitializeResourcesNow = true;
      this->InitResourcesCV.notify_one();

      // wait until the worker initializes it's resources.
      std::unique_lock<std::mutex> initLock(this->InitResourcesMtx);
      this->InitResourcesCV.wait(initLock, [this] { return this->ResourcesInitialized.load(); });
      srcGLContext->MakeCurrent();
      this->InitializeResourcesNow = false;
      return;
    }
    else
    {
      // wait until main thread signals us to initialize.
      std::unique_lock<std::mutex> initLock(this->InitResourcesMtx);
      this->InitResourcesCV.wait(initLock, [this] { return this->InitializeResourcesNow.load(); });
      initLock.unlock();

      if (this->WorkerContext == nullptr)
      {
        this->WorkerContext = vtk::TakeSmartPointer(srcGLContext->NewInstance());
        this->WorkerContext->ShowWindowOff();
        this->WorkerContext->SetSharedRenderWindow(srcGLContext);
      }
      vtkOpenGLRenderWindow::SafeDownCast(this->WorkerContext)->Initialize();

      this->ResourcesInitialized = true;
      this->InitResourcesCV.notify_one();
      return;
    }
  }
  else
  {
    // some other graphics implementation..
  }
}

//------------------------------------------------------------------------------
void vtkAsynchronousEncoderDelegate::PreTerminateWorker()
{
  vtkLogScopeF(TRACE, "%s", __func__);
  ENSURE_NOT_MAIN_THREAD;

  if (this->WorkerContext != nullptr)
  {
    this->WorkerContext->Finalize();
    // TODO: What happens when the original renderwindow is no longer alive?
    // this->WorkerContext = nullptr;
  }
  this->ResourcesDestroyed = true;
  this->DestroyResourcesCV.notify_one();
}
