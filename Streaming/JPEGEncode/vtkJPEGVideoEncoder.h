/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGVideoEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkJPEGVideoEncoder
 * @brief   this class implements video encoding with vtkJPEGWriter
 *
 * @sa vtkVideoEncoder, vtkCPUVideoFrame, vtkCompressedVideoPacket
 */
#ifndef vtkJPEGVideoEncoder_h
#define vtkJPEGVideoEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingJPEGEncodeModule.h" // for export macro

class vtkJPEGWriter;

class VTKSTREAMINGJPEGENCODE_EXPORT vtkJPEGVideoEncoder : public vtkVideoEncoder
{
  vtkTypeMacro(vtkJPEGVideoEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkJPEGVideoEncoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() const noexcept override { return false; }
  bool SupportsAsynchronousDelegate() const noexcept override { return true; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
  ///@}

protected:
  vtkJPEGVideoEncoder();
  ~vtkJPEGVideoEncoder() override;

  ///@{
  /**
   * Implement parent class encoder context management.
   */
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;
  ///@}

  ///@{
  /**
   * Implement parent class encoding and encoder resource management.
   */
  bool SetupEncoderFrame(int width, int height) override;
  bool NeedsNewEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;
  VTKVideoEncoderResultType DrainInternal() override;
  VTKVideoProcessingStatusType PushInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;
  VTKVideoEncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  ///@}

  VTKVideoEncoderResultType EncodeScreenInternal(vtkRenderWindow* window) override { return {}; }

private:
  vtkJPEGVideoEncoder(const vtkJPEGVideoEncoder&) = delete;
  void operator=(const vtkJPEGVideoEncoder&) = delete;

  vtkJPEGWriter* Writer = nullptr;
  vtkIdType EncodeTime = 0;
};

#endif // vtkJPEGVideoEncoder_h
