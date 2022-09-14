/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMockVideoDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkMockVideoDecoder_h
#define vtkMockVideoDecoder_h

#include "vtkVideoDecoder.h"

#include "vtkStreamingDecodeModule.h" // for export macro

class VTKSTREAMINGDECODE_EXPORT vtkMockVideoDecoder : public vtkVideoDecoder
{
public:
  vtkTypeMacro(vtkMockVideoDecoder, vtkVideoDecoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMockVideoDecoder* New();

  bool IsHardwareAccelerated() const noexcept override { return false; }
  bool SupportsAsynchronousDelegate() const noexcept override { return true; }
  bool SupportsSynchronousDelegate() const noexcept override { return true; }
  vtkIdType GetLastDecodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override { return true; };

protected:
  vtkMockVideoDecoder();
  ~vtkMockVideoDecoder() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;

  VTKVideoProcessingStatusType PushInternal(vtkCompressedVideoPacket* packet) override;
  VTKVideoDecoderResultType GetResultInternal() override;
  VTKVideoDecoderResultType DecodeInternal(vtkCompressedVideoPacket* packet) override;
  void DrainInternal() override;

private:
  vtkMockVideoDecoder(const vtkMockVideoDecoder&) = delete;
  void operator=(const vtkMockVideoDecoder&) = delete;
};

#endif // vtkMockVideoDecoder_h
