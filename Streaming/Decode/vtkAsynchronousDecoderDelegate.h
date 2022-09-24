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

#include "vtkObject.h"

#include "vtkThreadedTaskQueue.h"            // for taskqueue
#include "vtkVideoProcessingWorkUnitTypes.h" // for return value

#include <atomic> // for ivar
#include <memory> // for ivar

class vtkAsynchronousDecoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkAsynchronousDecoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkAsynchronousDecoderDelegate* New();

  using TaskQueueType = vtkThreadedTaskQueue<VTKVideoDecoderResultType, VTKVideoDecoderInputType>;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  vtkSetMacro(NumberOfTasks, int);
  vtkGetMacro(NumberOfTasks, int);

  vtkSetMacro(StrictOrdering, bool);
  vtkGetMacro(StrictOrdering, int);
  vtkBooleanMacro(StrictOrdering, bool);

  void InitializeWorker(VTKVideoDecodeWorkerType);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkCompressedVideoPacket* frame);

  VTKVideoDecoderResultType GetResult();
  bool HasResult();

protected:
  vtkAsynchronousDecoderDelegate();
  ~vtkAsynchronousDecoderDelegate() override;

  bool StrictOrdering = true;
  int BufferSize = -1;
  int NumberOfTasks = 1;
  VTKVideoDecoderResultType Result;
  std::atomic<bool> TrySucceeded;
  std::unique_ptr<TaskQueueType> TaskQueue;

  void PreparePacket(vtkCompressedVideoPacket* packet, VTKVideoDecoderInputType& dstPacket);

private:
  vtkAsynchronousDecoderDelegate(const vtkAsynchronousDecoderDelegate&) = delete;
  void operator=(const vtkAsynchronousDecoderDelegate&) = delete;
};

#endif // vtkAsynchronousDecoderDelegate_h
// VTK-HeaderTest-Exclude: vtkAsynchronousDecoderDelegate.h
