/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGVaapiEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGVaapiEncoder
 * @brief   this class implements hardware accelerated (GPU-based) encoder.
 *
 * It uses FFMPEG with libva. The encoder supports vp9 and h265 codecs.
 * Note that this encoder must be preferred on intel GPUs in linux platform.
 *
 * @sa vtkAbstractVideoEncoder, vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkFFMPEGVaapiEncoder_h
#define vtkFFMPEGVaapiEncoder_h

#include "vtkVideoFFMPEGModule.h"

#include "vtkAbstractVideoEncoder.h"
#include "vtkCodecTypes.h"

#include <memory>
#include <string>

class vtkRawVideoFrame;
class vtkFFMPEGEncoderInternals;

class VTKVIDEOFFMPEG_EXPORT vtkFFMPEGVaapiEncoder : public vtkAbstractVideoEncoder
{
public:
  vtkTypeMacro(vtkFFMPEGVaapiEncoder, vtkAbstractVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFMPEGVaapiEncoder* New();

  ///@{
  /**
   * Implement public convenient methods.
   */
  bool IsHardwareAccelerated() override { return true; }
  vtkIdType GetLastEncodeTimeNS() override;
  vtkIdType GetLastScaleTimeNS() override;
  bool IsCodecSupported(VTKCodecType codec) override;
  ///@}

  ///@{
  /**
   * Set/Get device name. On linux, the device name is like /dev/dri/something-something.
   * This option is only available under drm displays. Normally, you do not have to set this. libva
   * picks up the correct graphics device.
   */
  void SetDeviceName(const char* dev);
  const char* GetDeviceName();
  ///@}

protected:
  vtkFFMPEGVaapiEncoder();
  ~vtkFFMPEGVaapiEncoder() override;

  std::string Device;

  ///@{
  /**
   * Implement parent class decoding API.
   */
  bool InitializeInternal() override;
  void ShutdownInternal() override;
  void FlushInternal() override;
  bool PushInternal(vtkRawVideoFrame* frame) override;
  ///@}

  ///@{
  /**
   * Implement parent class encoding and hardware encoder resource management.
   */
  bool SetupEncoderFrame(const int& w, const int& h) override;
  bool NeedsNewEncoderFrame(const int& w, const int& h) override;
  void TearDownEncoderFrame() override;
  bool Encode() override;
  ///@}

private:
  vtkFFMPEGVaapiEncoder(const vtkFFMPEGVaapiEncoder&) = delete;
  void operator=(const vtkFFMPEGVaapiEncoder&) = delete;

  std::unique_ptr<vtkFFMPEGEncoderInternals> Internals;
};

#endif // vtkFFMPEGVaapiEncoder_h
