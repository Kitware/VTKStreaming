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
 * @brief   this class implements a synchronous delegate for decoding video packets
 *
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkSynchronousDecoderDelegate_h
#define vtkSynchronousDecoderDelegate_h

#include "vtkVideoCoreModule.h"

#include "vtkAbstractDecoderDelegate.h"

class vtkRawVideoFrame;

class VTKVIDEOCORE_EXPORT vtkSynchronousDecoderDelegate : public vtkAbstractDecoderDelegate
{
public:
  vtkTypeMacro(vtkSynchronousDecoderDelegate, vtkAbstractDecoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSynchronousDecoderDelegate* New();

protected:
  vtkSynchronousDecoderDelegate();
  ~vtkSynchronousDecoderDelegate() override;

  void InitializeWorkerInternal(DecodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkCodedVideoPacket* packet) override;
  DecoderResultType GetResultInternal() override;

  void ResetQueues();

  std::queue<DecoderInputType> Packets;
  std::queue<DecoderResultType> Results;
  DecodeWorkerType Worker;

private:
  vtkSynchronousDecoderDelegate(const vtkSynchronousDecoderDelegate&) = delete;
  void operator=(const vtkSynchronousDecoderDelegate&) = delete;
};

#endif // vtkSynchronousDecoderDelegate_h
