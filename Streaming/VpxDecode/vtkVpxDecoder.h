// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef vtkVpxDecoder_h
#define vtkVpxDecoder_h

#include "vtkVideoDecoder.h"

#include "vtkStreamingVpxDecodeModule.h" // for export macro

#include <memory> // for ivar

class VTKSTREAMINGVPXDECODE_EXPORT vtkVpxDecoder : public vtkVideoDecoder
{
public:
  vtkTypeMacro(vtkVpxDecoder, vtkVideoDecoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVpxDecoder* New();

  bool IsHardwareAccelerated() const noexcept override { return false; };
  vtkIdType GetLastDecodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;

protected:
  vtkVpxDecoder();
  ~vtkVpxDecoder() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;

  VTKVideoDecoderResultType DecodeInternal(vtkSmartPointer<vtkCompressedVideoPacket>) override;
  VTKVideoDecoderResultType SendEOS() override;

private:
  vtkVpxDecoder(const vtkVpxDecoder&) = delete;
  void operator=(const vtkVpxDecoder&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
