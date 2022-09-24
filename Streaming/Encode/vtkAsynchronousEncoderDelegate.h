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

#include "vtkThreadedTaskQueue.h"            // for taskqueue
#include "vtkVideoProcessingWorkUnitTypes.h" // for return value

#include <atomic> // for ivar
#include <memory> // for ivar

class vtkRawVideoFrame;

class vtkAsynchronousEncoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkAsynchronousEncoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousEncoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<VTKVideoEncoderResultType, VTKVideoEncoderInputType>;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  void InitializeWorker(VTKVideoEncodeWorkerType);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkRawVideoFrame* frame);
  VTKVideoEncoderResultType GetResult();
  bool HasResult();

protected:
  vtkAsynchronousEncoderDelegate();
  ~vtkAsynchronousEncoderDelegate() override;

  bool StrictOrdering = true;
  int BufferSize = -1;
  int NumberOfTasks = 1;
  VTKVideoEncoderResultType Result;
  std::atomic<bool> TrySucceeded;
  std::unique_ptr<TaskQueueType> TaskQueue;

  void PrepareFrame(vtkRawVideoFrame* frame, VTKVideoEncoderInputType& dstFrame);

private:
  vtkAsynchronousEncoderDelegate(const vtkAsynchronousEncoderDelegate&) = delete;
  void operator=(const vtkAsynchronousEncoderDelegate&) = delete;
};

#endif // vtkAsynchronousEncoderDelegate_h
// VTK-HeaderTest-Exclude: vtkAsynchronousEncoderDelegate.h
