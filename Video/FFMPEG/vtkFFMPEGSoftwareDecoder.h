/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGSoftwareDecoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGSoftwareDecoder
 * @brief   this class implements software decoder with FFMPEG.
 *
 * @sa vtkAbstractVideoDecoder, vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkFFMPEGSoftwareDecoder_h
#define vtkFFMPEGSoftwareDecoder_h

#include "vtkVideoFFMPEGModule.h"

#include "vtkAbstractVideoDecoder.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <memory>

class vtkFFMPEGDecoderInternals;

class VTKVIDEOFFMPEG_EXPORT vtkFFMPEGSoftwareDecoder : public vtkAbstractVideoDecoder
{
public:
  vtkTypeMacro(vtkFFMPEGSoftwareDecoder, vtkAbstractVideoDecoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFMPEGSoftwareDecoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() override { return false; }
  vtkIdType GetLastDecodeTimeNS() override;
  vtkIdType GetLastScaleTimeNS() override;
  ///@}

protected:
  vtkFFMPEGSoftwareDecoder();
  ~vtkFFMPEGSoftwareDecoder() override;

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
  void DrainInternal() override;
  DecoderResultType DecodeInternal(vtkCodedVideoPacket* frame) override;
  ///@}

private:
  vtkFFMPEGSoftwareDecoder(const vtkFFMPEGSoftwareDecoder&) = delete;
  void operator=(const vtkFFMPEGSoftwareDecoder&) = delete;
  std::unique_ptr<vtkFFMPEGDecoderInternals> Internals;
};

#endif // vtkFFMPEGSoftwareDecoder_h
