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
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAsynchronousEncoderDelegate_h
#define vtkAsynchronousEncoderDelegate_h

#include "vtkThreadedTaskQueue.h"
#include "vtkVideoCoreModule.h"

#include "vtkAbstractEncoderDelegate.h"

#include <atomic>
#include <memory>

class vtkRawVideoFrame;

class VTKVIDEOCORE_EXPORT vtkAsynchronousEncoderDelegate : public vtkAbstractEncoderDelegate
{
public:
  vtkTypeMacro(vtkAsynchronousEncoderDelegate, vtkAbstractEncoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousEncoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<EncoderResultType, EncoderInputType>;

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  bool HasResult() override;

protected:
  vtkAsynchronousEncoderDelegate();
  ~vtkAsynchronousEncoderDelegate() override;

  EncoderResultType Result;
  std::unique_ptr<TaskQueueType> TaskQueue;
  int NumberOfTasks = 1;
  bool StrictOrdering = true;
  std::atomic<bool> TrySucceeded;

  void InitializeWorkerInternal(EncodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkRawVideoFrame* frame) override;
  EncoderResultType GetResultInternal() override;

private:
  vtkAsynchronousEncoderDelegate(const vtkAsynchronousEncoderDelegate&) = delete;
  void operator=(const vtkAsynchronousEncoderDelegate&) = delete;
};

#endif // vtkAsynchronousEncoderDelegate_h
