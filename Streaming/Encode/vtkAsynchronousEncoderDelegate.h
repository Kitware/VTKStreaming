/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousEncoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAsynchronousEncoderDelegate
 * @brief   this class implements an asynchronous delegate for encoding video frames.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkAsynchronousEncoderDelegate_h
#define vtkAsynchronousEncoderDelegate_h

#include "vtkObject.h"

#include "vtkAsyncTaskQueue.h"               // for async
#include "vtkSmartPointer.h"                 // for ivar
#include "vtkVideoProcessingWorkUnitTypes.h" // for return value

#include <memory>

class vtkRawVideoFrame;

class vtkAsynchronousEncoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkAsynchronousEncoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousEncoderDelegate* New();

  /**
   * Construct `TaskQueueType` with strict ordering, infinite buffer size and
   * no. of threads = 1.
   * Specify the main thread graphics context so that we can share graphics resources.
   */
  void InitializeWorker(VTKVideoEncodeWorkerType, vtkRenderWindow* mainGfxContext);
  vtkRenderWindow* GetWorkerGfxContext() { return this->WorkerContext; }

  /**
   * Force execute all pending tasks. This does not terminate the worker thread.
   * Upon finishing, the number of pending tasks becomes 0.
   */
  void Flush();

  /**
   * Flush the task queue and terminate the worker thread.
   */
  void Terminate();

  void PushWorkUnit(VTKVideoEncoderInputType frame);
  VTKVideoEncoderResultType GetResult();
  bool HasResult();

  VTKVideoEncoderResultType Execute(VTKVideoEncoderInputType frame);

protected:
  vtkAsynchronousEncoderDelegate();
  ~vtkAsynchronousEncoderDelegate() override;

  vtkAsyncTaskQueue TaskQueue;
  WaitingQueue<VTKVideoEncoderResultType> AwaitableQ;
  VTKVideoEncodeWorkerType WorkerFunction;
  VTKVideoEncoderResultType Result;
  std::atomic<bool> TrySucceeded;

  /**
   * Keeps the input frames alive on main thread.
   * Prevents the worker thread from cleaning up main thread resources upon task completion.
   * Certain graphics resources must be freed by the thread that created them.
   * @warning: The only way to empty the safety net is to call ::GetResult often
   *           ::Flush/::Terminate also empty it.
   */
  std::queue<VTKVideoEncoderInputType> FrameSustainer;
  float SustainedGB = 0;

  ///{@
  // a vtk render window whose OpenGL context is managed by the worker thread.
  // it shares OpenGL object lists with another window on the main thread.
  vtkSmartPointer<vtkRenderWindow> WorkerContext;
  vtkRenderWindow* MainGfxContext = nullptr;
  ///@}

  ///{@
  /**
   * Thread safe task tracking.
   */
  std::thread::id Tid;
  ///@}

  ///{@
  /**
   * Lifetime of the main thread resources must end on the main-thread.
   * Call sustain to make sure main thread keeps the resource alive.
   * Use relinquish to release the resource on main thread.
   * These methods will abort if not called on the main thread.
   * @note: main thread refers to the thread that constructed the delegate.
   */
  void Sustain(VTKVideoEncoderInputType input);
  void Relinquish();
  void RelinquishAll();
  ///@}

  /**
   * Sets up resources necessary for a work unit on the worker/main thread.
   */
  VTKVideoEncoderInputType PrepareThreadLocalResources(
    VTKVideoEncoderInputType from, bool shallow_copy = false);

private:
  vtkAsynchronousEncoderDelegate(const vtkAsynchronousEncoderDelegate&) = delete;
  void operator=(const vtkAsynchronousEncoderDelegate&) = delete;
};

#endif // vtkAsynchronousEncoderDelegate_h
// VTK-HeaderTest-Exclude: vtkAsynchronousEncoderDelegate.h
