// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVideoToolboxEncoder
 * @brief   hardware-accelerated H.264/H.265 encoder for Apple platforms
 *
 * vtkVideoToolboxEncoder encodes raw video frames on Apple Silicon (and Intel
 * Macs) using the system VideoToolbox framework's `VTCompressionSession`, which
 * drives the dedicated hardware media engine. It supports the H.264 and H.265
 * (HEVC) codecs.
 *
 * The incoming vtkRawVideoFrame pixels are uploaded (CPU copy) into an
 * IOSurface-backed CVPixelBuffer obtained from the compression session's own
 * pool, then submitted to the hardware encoder. Emitted packets carry an Annex B
 * elementary stream (NAL units prefixed with `00 00 00 01` start codes, with the
 * codec parameter sets prepended on key frames), matching the NVENC backend and
 * directly playable by tools such as ffplay.
 *
 * @sa vtkVideoEncoder, vtkNvEncoderGL, vtkVpxEncoder
 */

#ifndef vtkVideoToolboxEncoder_h
#define vtkVideoToolboxEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingVTEncodeModule.h" // for export macro

#include <memory> // for ivar

struct vtkVideoToolboxEncoderInternals;

class VTKSTREAMINGVTENCODE_EXPORT vtkVideoToolboxEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkVideoToolboxEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVideoToolboxEncoder* New();

  bool IsHardwareAccelerated() const noexcept override { return true; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;

  /**
   * Returns true when a hardware-accelerated VideoToolbox encoder for H.264 is
   * available on this machine. Mirrors vtkNvEncoderGL::CheckAvailability.
   */
  static bool CheckAvailability() noexcept;

protected:
  vtkVideoToolboxEncoder();
  ~vtkVideoToolboxEncoder() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;

  bool SetupEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;

  VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) override;
  VTKVideoEncoderResultType SendEOS() override;

private:
  vtkVideoToolboxEncoder(const vtkVideoToolboxEncoder&) = delete;
  void operator=(const vtkVideoToolboxEncoder&) = delete;

  std::unique_ptr<vtkVideoToolboxEncoderInternals> Internals;
};

#endif // vtkVideoToolboxEncoder_h
