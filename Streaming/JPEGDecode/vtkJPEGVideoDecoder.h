/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGVideoDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkJPEGVideoDecoder
 * @brief   this class implements video decoding with vtkJPEGReader
 *
 * @sa vtkVideoDecoder, vtkCompressedVideoPacket
 */

#ifndef vtkJPEGVideoDecoder_h
#define vtkJPEGVideoDecoder_h

#include "vtkVideoDecoder.h"

#include "vtkJPEGReader.h"                // for ivar
#include "vtkNew.h"                       // for ivar
#include "vtkStreamingJPEGDecodeModule.h" // for export macro

class VTKSTREAMINGJPEGDECODE_EXPORT vtkJPEGVideoDecoder : public vtkVideoDecoder
{
  vtkTypeMacro(vtkJPEGVideoDecoder, vtkVideoDecoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkJPEGVideoDecoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() const noexcept override { return false; }
  bool SupportsAsyncMode() const noexcept override { return true; }
  vtkIdType GetLastDecodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
  ///@}

protected:
  vtkJPEGVideoDecoder();
  ~vtkJPEGVideoDecoder() override;

  vtkNew<vtkJPEGReader> Reader;
  vtkIdType DecodeTime = 0;

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
   * Implement parent class decoding and decoder resource management.
   */
  VTKVideoDecoderResultType DrainInternal() override;
  VTKVideoProcessingStatusType PushInternal(vtkCompressedVideoPacket* packet) override;
  VTKVideoDecoderResultType GetResultInternal() override;
  VTKVideoDecoderResultType DecodeInternal(vtkCompressedVideoPacket* packet) override;
  ///@}

private:
  vtkJPEGVideoDecoder(const vtkJPEGVideoDecoder&) = delete;
  void operator=(const vtkJPEGVideoDecoder&) = delete;
};

#endif // vtkJPEGVideoDecoder_h
