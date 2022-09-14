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
 * @brief   this internal class implements an asynchronous delegate for decoding video packets
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkAsynchronousDecoderDelegate_h
#define vtkAsynchronousDecoderDelegate_h

#include "vtkDecoderDelegate.h"

#include "vtkThreadedTaskQueue.h" // for taskqueue

#include <atomic> // for ivar
#include <memory> // for ivar

class vtkAsynchronousDecoderDelegate : public vtkDecoderDelegate
{
public:
  vtkTypeMacro(vtkAsynchronousDecoderDelegate, vtkDecoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousDecoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<VTKVideoDecoderResultType, VTKVideoDecoderInputType>;

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  bool HasResult() override;

protected:
  vtkAsynchronousDecoderDelegate();
  ~vtkAsynchronousDecoderDelegate() override;

  VTKVideoDecoderResultType Result;
  std::unique_ptr<TaskQueueType> TaskQueue;
  int NumberOfTasks = 1;
  bool StrictOrdering = true;
  std::atomic<bool> TrySucceeded;

  void InitializeWorkerInternal(VTKVideoDecodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkCompressedVideoPacket* frame) override;
  VTKVideoDecoderResultType GetResultInternal() override;

private:
  vtkAsynchronousDecoderDelegate(const vtkAsynchronousDecoderDelegate&) = delete;
  void operator=(const vtkAsynchronousDecoderDelegate&) = delete;
};

#endif // vtkAsynchronousDecoderDelegate_h
// VTK-HeaderTest-Exclude: vtkAsynchronousDecoderDelegate.h
