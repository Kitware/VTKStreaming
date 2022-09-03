/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractDecoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAbstractDecoderDelegate
 * @brief   this class defines an abstract delegate for decoding video frames.
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkAbstractDecoderDelegate_h
#define vtkAbstractDecoderDelegate_h

#include "vtkVideoCoreModule.h"

#include "vtkCodecTypes.h"
#include "vtkObject.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <queue>

class VTKVIDEOCORE_EXPORT vtkAbstractDecoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractDecoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  void InitializeWorker(DecodeWorkerType workerFunc);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkCodedVideoPacket* frame);
  DecoderResultType GetResult();
  virtual bool HasResult() { return true; }

protected:
  vtkAbstractDecoderDelegate();
  ~vtkAbstractDecoderDelegate() override;

  int BufferSize = -1;

  virtual void InitializeWorkerInternal(DecodeWorkerType workerFunc) = 0;
  virtual void FlushInternal() = 0;
  virtual void TerminateInternal() = 0;

  void PreparePacketInternal(vtkCodedVideoPacket* packet, DecoderInputType& dstPacket);

  virtual void PushWorkUnitInternal(vtkCodedVideoPacket* packet) = 0;
  virtual DecoderResultType GetResultInternal() = 0;

private:
  vtkAbstractDecoderDelegate(const vtkAbstractDecoderDelegate&) = delete;
  void operator=(const vtkAbstractDecoderDelegate&) = delete;
};

#endif // vtkAbstractDecoderDelegate_h
