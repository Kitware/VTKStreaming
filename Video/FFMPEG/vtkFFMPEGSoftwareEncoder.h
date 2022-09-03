/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGSoftwareEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGSoftwareEncoder
 * @brief   this class implements software decoder with FFMPEG.
 *
 * @sa vtkAbstractVideoEncoder, vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkFFMPEGSoftwareEncoder_h
#define vtkFFMPEGSoftwareEncoder_h

#include "vtkVideoFFMPEGModule.h"

#include "vtkAbstractVideoEncoder.h"
#include "vtkCodecTypes.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <memory>

class vtkFFMPEGEncoderInternals;

class VTKVIDEOFFMPEG_EXPORT vtkFFMPEGSoftwareEncoder : public vtkAbstractVideoEncoder
{
public:
  vtkTypeMacro(vtkFFMPEGSoftwareEncoder, vtkAbstractVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFMPEGSoftwareEncoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() override { return false; }
  vtkIdType GetLastEncodeTimeNS() override;
  vtkIdType GetLastScaleTimeNS() override;
  bool IsCodecSupported(VTKCodecType codec) override;
  ///@}

protected:
  vtkFFMPEGSoftwareEncoder();
  ~vtkFFMPEGSoftwareEncoder() override;

  ///@{
  /**
   * Implement parent class encoder context management and status translation.
   */
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;
  ///@}

  ///@{
  /**
   * Implement parent class encoding and encoder resource management.
   */
  bool SetupEncoderFrame(const int& width, const int& height) override;
  bool NeedsNewEncoderFrame(const int& width, const int& height) override;
  void TearDownEncoderFrame() override;
  void DrainInternal() override;
  EncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  ///@}

private:
  vtkFFMPEGSoftwareEncoder(const vtkFFMPEGSoftwareEncoder&) = delete;
  void operator=(const vtkFFMPEGSoftwareEncoder&) = delete;

  std::unique_ptr<vtkFFMPEGEncoderInternals> Internals;
};

#endif // vtkFFMPEGSoftwareEncoder_h
