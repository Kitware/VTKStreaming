/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronousEncoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSynchronousEncoderDelegate
 * @brief   this class implements a synchronous delegate for encoding video frames.
 *
 * @sa vtkCPUVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkSynchronousEncoderDelegate_h
#define vtkSynchronousEncoderDelegate_h

#include "vtkEncoderDelegate.h"

#include <queue> // for ivar

class vtkCPUVideoFrame;

class vtkSynchronousEncoderDelegate : public vtkEncoderDelegate
{
public:
  vtkTypeMacro(vtkSynchronousEncoderDelegate, vtkEncoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSynchronousEncoderDelegate* New();

protected:
  vtkSynchronousEncoderDelegate();
  ~vtkSynchronousEncoderDelegate() override;

  void InitializeWorkerInternal(VTKVideoEncodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;

  void ResetQueues();

  std::queue<VTKVideoEncoderInputType> Frames;
  std::queue<VTKVideoEncoderResultType> Results;
  VTKVideoEncodeWorkerType Worker;

private:
  vtkSynchronousEncoderDelegate(const vtkSynchronousEncoderDelegate&) = delete;
  void operator=(const vtkSynchronousEncoderDelegate&) = delete;
};

#endif // vtkSynchronousEncoderDelegate_h
// VTK-HeaderTest-Exclude: vtkSynchronousEncoderDelegate.h
