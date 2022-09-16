/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMockVideoEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMockVideoEncoder_h
#define vtkMockVideoEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingEncodeModule.h" // for export macro

class VTKSTREAMINGENCODE_EXPORT vtkMockVideoEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkMockVideoEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMockVideoEncoder* New();

  bool IsHardwareAccelerated() const noexcept override { return false; }
  bool SupportsAsynchronousDelegate() const noexcept override { return true; }
  bool SupportsSynchronousDelegate() const noexcept override { return true; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override { return true; };

protected:
  vtkMockVideoEncoder();
  ~vtkMockVideoEncoder() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;

  bool SetupEncoderFrame(int, int) override;
  bool NeedsNewEncoderFrame(int, int) override;
  void TearDownEncoderFrame() override;

  VTKVideoProcessingStatusType PushInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;
  VTKVideoEncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType DrainInternal() override;

private:
  vtkMockVideoEncoder(const vtkMockVideoEncoder&) = delete;
  void operator=(const vtkMockVideoEncoder&) = delete;
};

#endif // vtkMockVideoEncoder_h
