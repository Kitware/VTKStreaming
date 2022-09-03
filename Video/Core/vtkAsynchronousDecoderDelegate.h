/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAsynchronousDecoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAsynchronousDecoderDelegate
 * @brief   this class implements an asynchronous delegate for decoding video packets
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAsynchronousDecoderDelegate_h
#define vtkAsynchronousDecoderDelegate_h

#include "vtkThreadedTaskQueue.h"
#include "vtkVideoCoreModule.h"

#include "vtkAbstractDecoderDelegate.h"

#include <atomic>
#include <memory>

class vtkRawVideoFrame;

class VTKVIDEOCORE_EXPORT vtkAsynchronousDecoderDelegate : public vtkAbstractDecoderDelegate
{
public:
  vtkTypeMacro(vtkAsynchronousDecoderDelegate, vtkAbstractDecoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousDecoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<DecoderResultType, DecoderInputType>;

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  bool HasResult() override;

protected:
  vtkAsynchronousDecoderDelegate();
  ~vtkAsynchronousDecoderDelegate() override;

  DecoderResultType Result;
  std::unique_ptr<TaskQueueType> TaskQueue;
  int NumberOfTasks = 1;
  bool StrictOrdering = true;
  std::atomic<bool> TrySucceeded;

  void InitializeWorkerInternal(DecodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkCodedVideoPacket* frame) override;
  DecoderResultType GetResultInternal() override;

private:
  vtkAsynchronousDecoderDelegate(const vtkAsynchronousDecoderDelegate&) = delete;
  void operator=(const vtkAsynchronousDecoderDelegate&) = delete;
};

#endif // vtkAsynchronousDecoderDelegate_h
