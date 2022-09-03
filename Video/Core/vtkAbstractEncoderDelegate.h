/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractEncoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractEncoderDelegate
 * @brief   this class defines an abstract delegate for encoding video frames.
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractEncoderDelegate_h
#define vtkAbstractEncoderDelegate_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkObject.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <queue>

class VTKVIDEOCORE_EXPORT vtkAbstractEncoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractEncoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  void InitializeWorker(EncodeWorkerType workerFunc);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkRawVideoFrame* frame);
  EncoderResultType GetResult();
  virtual bool HasResult() { return true; }

protected:
  vtkAbstractEncoderDelegate();
  ~vtkAbstractEncoderDelegate() override;

  int BufferSize = -1;

  virtual void InitializeWorkerInternal(EncodeWorkerType workerFunc) = 0;
  virtual void FlushInternal() = 0;
  virtual void TerminateInternal() = 0;

  void PrepareFrameInternal(vtkRawVideoFrame* frame, EncoderInputType& dstFrame);

  virtual void PushWorkUnitInternal(vtkRawVideoFrame* frame) = 0;
  virtual EncoderResultType GetResultInternal() = 0;

private:
  vtkAbstractEncoderDelegate(const vtkAbstractEncoderDelegate&) = delete;
  void operator=(const vtkAbstractEncoderDelegate&) = delete;
};

#endif // vtkAbstractEncoderDelegate_h
