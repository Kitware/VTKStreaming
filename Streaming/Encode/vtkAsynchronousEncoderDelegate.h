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

#include "vtkEncoderDelegate.h"

#include "vtkThreadedTaskQueue.h" // for taskqueue

#include <atomic> // for ivar
#include <memory> // for ivar

class vtkRawVideoFrame;

class vtkAsynchronousEncoderDelegate : public vtkEncoderDelegate
{
public:
  vtkTypeMacro(vtkAsynchronousEncoderDelegate, vtkEncoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousEncoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<VTKVideoEncoderResultType, VTKVideoEncoderInputType>;

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  bool HasResult() override;

protected:
  vtkAsynchronousEncoderDelegate();
  ~vtkAsynchronousEncoderDelegate() override;

  VTKVideoEncoderResultType Result;
  std::unique_ptr<TaskQueueType> TaskQueue;
  int NumberOfTasks = 1;
  bool StrictOrdering = true;
  std::atomic<bool> TrySucceeded;

  void InitializeWorkerInternal(VTKVideoEncodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;

private:
  vtkAsynchronousEncoderDelegate(const vtkAsynchronousEncoderDelegate&) = delete;
  void operator=(const vtkAsynchronousEncoderDelegate&) = delete;
};

#endif // vtkAsynchronousEncoderDelegate_h
// VTK-HeaderTest-Exclude: vtkAsynchronousEncoderDelegate.h
