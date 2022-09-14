/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDecoderDelegate
 * @brief   this internal class defines an abstract delegate for decoding video frames.
 *
 * @sa vtkCPUVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkDecoderDelegate_h
#define vtkDecoderDelegate_h

#include "vtkObject.h"

#include "vtkVideoCodecTypes.h"              // for enum
#include "vtkVideoProcessingWorkUnitTypes.h" // for work unit

#include <queue> // for ivar

class vtkDecoderDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkDecoderDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(BufferSize, int);
  vtkGetMacro(BufferSize, int);

  void InitializeWorker(VTKVideoDecodeWorkerType workerFunc);
  void Flush();
  void Terminate();

  void PushWorkUnit(vtkCompressedVideoPacket* frame);
  VTKVideoDecoderResultType GetResult();
  virtual bool HasResult() { return true; }

protected:
  vtkDecoderDelegate();
  ~vtkDecoderDelegate() override;

  int BufferSize = -1;

  virtual void InitializeWorkerInternal(VTKVideoDecodeWorkerType workerFunc) = 0;
  virtual void FlushInternal() = 0;
  virtual void TerminateInternal() = 0;

  void PreparePacketInternal(vtkCompressedVideoPacket* packet, VTKVideoDecoderInputType& dstPacket);

  virtual void PushWorkUnitInternal(vtkCompressedVideoPacket* packet) = 0;
  virtual VTKVideoDecoderResultType GetResultInternal() = 0;

private:
  vtkDecoderDelegate(const vtkDecoderDelegate&) = delete;
  void operator=(const vtkDecoderDelegate&) = delete;
};

#endif // vtkDecoderDelegate_h
// VTK-HeaderTest-Exclude: vtkDecoderDelegate.h
