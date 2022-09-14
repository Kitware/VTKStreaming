/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronousDecoderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSynchronousDecoderDelegate
 * @brief   this internal class implements a synchronous delegate for decoding video packets
 *
 * @sa vtkCPUVideoFrame, vtkCompressedVideoPacket
 */

#ifndef vtkSynchronousDecoderDelegate_h
#define vtkSynchronousDecoderDelegate_h

#include "vtkDecoderDelegate.h"

class vtkCPUVideoFrame;

class vtkSynchronousDecoderDelegate : public vtkDecoderDelegate
{
public:
  vtkTypeMacro(vtkSynchronousDecoderDelegate, vtkDecoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSynchronousDecoderDelegate* New();

protected:
  vtkSynchronousDecoderDelegate();
  ~vtkSynchronousDecoderDelegate() override;

  void InitializeWorkerInternal(VTKVideoDecodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkCompressedVideoPacket* packet) override;
  VTKVideoDecoderResultType GetResultInternal() override;

  void ResetQueues();

  std::queue<VTKVideoDecoderInputType> Packets;
  std::queue<VTKVideoDecoderResultType> Results;
  VTKVideoDecodeWorkerType Worker;

private:
  vtkSynchronousDecoderDelegate(const vtkSynchronousDecoderDelegate&) = delete;
  void operator=(const vtkSynchronousDecoderDelegate&) = delete;
};

#endif // vtkSynchronousDecoderDelegate_h
// VTK-HeaderTest-Exclude: vtkSynchronousDecoderDelegate.h
