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

#include <memory>

class vtkCodedVideoPacket;
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
  bool PushInternal(vtkCodedVideoPacket* packet) override;
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  ///@}

  ///@{
  /**
   * Implement parent class decoding.
   */
  void FlushInternal() override;
  bool Decode() override;
  ///@}

private:
  vtkFFMPEGSoftwareDecoder(const vtkFFMPEGSoftwareDecoder&) = delete;
  void operator=(const vtkFFMPEGSoftwareDecoder&) = delete;
  std::unique_ptr<vtkFFMPEGDecoderInternals> Internals;
};

#endif // vtkFFMPEGSoftwareDecoder_h
