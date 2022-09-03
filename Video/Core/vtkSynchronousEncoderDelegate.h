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
 * @sa vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkSynchronousEncoderDelegate_h
#define vtkSynchronousEncoderDelegate_h

#include "vtkVideoCoreModule.h"

#include "vtkAbstractEncoderDelegate.h"

class vtkRawVideoFrame;

class VTKVIDEOCORE_EXPORT vtkSynchronousEncoderDelegate : public vtkAbstractEncoderDelegate
{
public:
  vtkTypeMacro(vtkSynchronousEncoderDelegate, vtkAbstractEncoderDelegate);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSynchronousEncoderDelegate* New();

protected:
  vtkSynchronousEncoderDelegate();
  ~vtkSynchronousEncoderDelegate() override;

  void InitializeWorkerInternal(EncodeWorkerType) override;
  void FlushInternal() override;
  void TerminateInternal() override;

  void PushWorkUnitInternal(vtkRawVideoFrame* frame) override;
  EncoderResultType GetResultInternal() override;

  void ResetQueues();

  std::queue<EncoderInputType> Frames;
  std::queue<EncoderResultType> Results;
  EncodeWorkerType Worker;

private:
  vtkSynchronousEncoderDelegate(const vtkSynchronousEncoderDelegate&) = delete;
  void operator=(const vtkSynchronousEncoderDelegate&) = delete;
};

#endif // vtkSynchronousEncoderDelegate_h
