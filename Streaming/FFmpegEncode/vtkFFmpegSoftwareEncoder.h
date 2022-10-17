/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegSoftwareEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFmpegSoftwareEncoder
 * @brief   this class implements software decoder with FFmpeg.
 *
 * @sa vtkVideoEncoder, vtkCompressedVideoPacket
 */

#ifndef vtkFFmpegSoftwareEncoder_h
#define vtkFFmpegSoftwareEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingFFmpegEncodeModule.h" // for export macro
#include "vtkVideoCodecTypes.h"             // for enum
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <memory> // for ivar

class vtkFFmpegEncoderInternals;

class VTKSTREAMINGFFMPEGENCODE_EXPORT vtkFFmpegSoftwareEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkFFmpegSoftwareEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFmpegSoftwareEncoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() const noexcept override { return false; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
  ///@}

protected:
  vtkFFmpegSoftwareEncoder();
  ~vtkFFmpegSoftwareEncoder() override;

  ///@{
  /**
   * Implement parent class encoder context management and status translation.
   */
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  ///@}

  ///@{
  /**
   * Implement parent class encoding and encoder resource management.
   */
  bool SetupEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;
  VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) override;
  VTKVideoEncoderResultType SendEOS() override;
  ///@}

private:
  vtkFFmpegSoftwareEncoder(const vtkFFmpegSoftwareEncoder&) = delete;
  void operator=(const vtkFFmpegSoftwareEncoder&) = delete;

  std::unique_ptr<vtkFFmpegEncoderInternals> Internals;
};

#endif // vtkFFmpegSoftwareEncoder_h
