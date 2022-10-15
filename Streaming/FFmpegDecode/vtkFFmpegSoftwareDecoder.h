/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegSoftwareDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFmpegSoftwareDecoder
 * @brief   this class implements software decoder with FFmpeg.
 *
 * @sa vtkVideoDecoder, vtkCompressedVideoPacket
 */

#ifndef vtkFFmpegSoftwareDecoder_h
#define vtkFFmpegSoftwareDecoder_h

#include "vtkVideoDecoder.h"

#include "vtkStreamingFFmpegDecodeModule.h" // for export macro
#include "vtkVideoProcessingStatusTypes.h"  // for enum

#include <memory> // for ivar

class vtkFFmpegDecoderInternals;

class VTKSTREAMINGFFMPEGDECODE_EXPORT vtkFFmpegSoftwareDecoder : public vtkVideoDecoder
{
public:
  vtkTypeMacro(vtkFFmpegSoftwareDecoder, vtkVideoDecoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFmpegSoftwareDecoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool SupportsAsyncMode() const noexcept override { return true; }
  bool IsHardwareAccelerated() const noexcept override { return false; }
  vtkIdType GetLastDecodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
  ///@}

protected:
  vtkFFmpegSoftwareDecoder();
  ~vtkFFmpegSoftwareDecoder() override;

  ///@{
  /**
   * Implement parent class decoder context management.
   */
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;
  ///@}

  ///@{
  /**
   * Implement parent class decoding.
   */
  VTKVideoProcessingStatusType PushInternal(vtkCompressedVideoPacket* packet) override;
  VTKVideoDecoderResultType GetResultInternal() override;
  VTKVideoDecoderResultType DecodeInternal(vtkCompressedVideoPacket* packet) override;
  VTKVideoDecoderResultType DrainInternal() override;
  ///@}

private:
  vtkFFmpegSoftwareDecoder(const vtkFFmpegSoftwareDecoder&) = delete;
  void operator=(const vtkFFmpegSoftwareDecoder&) = delete;
  std::unique_ptr<vtkFFmpegDecoderInternals> Internals;
};

#endif // vtkFFmpegSoftwareDecoder_h
