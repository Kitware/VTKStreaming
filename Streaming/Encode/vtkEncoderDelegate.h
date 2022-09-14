/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEncoderDelegate
 * @brief   this class defines an abstract delegate for encoding video frames.
 *
 * @sa vtkRawVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkEncoderDelegate_h
#define vtkEncoderDelegate_h

#include "vtkObject.h"

#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for return value

class vtkEncoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkEncoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  void InitializeWorker(VTKVideoEncodeWorkerType workerFunc);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkRawVideoFrame* frame);
  VTKVideoEncoderResultType GetResult();
  virtual bool HasResult() { return false; }

protected:
  vtkEncoderDelegate();
  ~vtkEncoderDelegate() override;

  int BufferSize = -1;

  virtual void InitializeWorkerInternal(VTKVideoEncodeWorkerType workerFunc) = 0;
  virtual void FlushInternal() = 0;
  virtual void TerminateInternal() = 0;

  void PrepareFrameInternal(vtkRawVideoFrame* frame, VTKVideoEncoderInputType& dstFrame);

  virtual void PushWorkUnitInternal(vtkRawVideoFrame* frame) = 0;
  virtual VTKVideoEncoderResultType GetResultInternal() = 0;

private:
  vtkEncoderDelegate(const vtkEncoderDelegate&) = delete;
  void operator=(const vtkEncoderDelegate&) = delete;
};

#endif // vtkEncoderDelegate_h
// VTK-HeaderTest-Exclude: vtkEncoderDelegate.h
