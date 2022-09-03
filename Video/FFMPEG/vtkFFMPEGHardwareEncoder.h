/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGHardwareEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGHardwareEncoder
 * @brief   this class implements hardware accelerated (GPU-based) encoder.
 *
 * It configures and uses FFmpeg with a hardware accelerated encoder.
 * The support for encoder codec varies.
 *
 * @sa vtkAbstractVideoEncoder, vtkRawVideoFrame, vtkCodedVideoPacket
 */

#ifndef vtkFFMPEGHardwareEncoder_h
#define vtkFFMPEGHardwareEncoder_h

#include "vtkCodedVideoPacket.h"
#include "vtkSmartPointer.h"
#include "vtkVideoFFMPEGModule.h"

#include "vtkAbstractVideoEncoder.h"
#include "vtkCodecTypes.h"
#include "vtkVideoProcessingStatusTypes.h"

#include <memory>
#include <string>

class vtkFFMPEGEncoderInternals;

class VTKVIDEOFFMPEG_EXPORT vtkFFMPEGHardwareEncoder : public vtkAbstractVideoEncoder
{
public:
  vtkTypeMacro(vtkFFMPEGHardwareEncoder, vtkAbstractVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFMPEGHardwareEncoder* New();

  enum class HardwareEncoderTypeEnum : int
  {
    None,
    VAAPI,
    QSV,
    AMF,
    NVENC,
    VideoToolbox,
    MediaFoundation,
    MaxSupportedHardwareEncoderTypes
  };

  /**
   * Requests the platform for supported hardware encoders and chooses a default.
   *
   * On windows,
   * 1. QSV if intel gpu is available and intel is preferred
   * 2. NVENC if NVIDIA gpu is available and NVIDIA is preferred
   * 3. AMF if AMD gpu is available and AMD is preferred
   * 4. MediaFoundation as best effort.
   *
   * On linux,
   * 1. QSV if intel gpu is available and intel is preferred
   * 2. NVENC if NVIDIA gpu is available and NVIDIA is preferred
   * 3. AMF if AMD gpu is available and AMD is preferred
   * 4. VAAPI as best effort.
   *
   * On MacOS X,
   * 1. VideoToolbox as best effort.
   *
   * This method sets the current hardware encoder type to any of the above.
   * Returns false if the platform doesn't support any of the above.
   */
  bool QueryPlatformSupport();
  void ClearGPUPreference();
  void PreferIntelEncoders();
  void PreferAMDEncoders();
  void PreferNVIDIAEncoders();

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
   * Set/Get device name.
   * Some hardware encoders may be flexible enough to select the device
   * used to accelerate encoding. VAAPI does this.
   * On linux, the device name is like /dev/dri/something-something.
   * This option is only available under drm displays. Normally, you do not have to set this. libva
   * picks up the correct graphics device.
   */
  void SetDeviceName(const char* dev);
  const char* GetDeviceName();
  ///@}

protected:
  vtkFFMPEGHardwareEncoder();
  ~vtkFFMPEGHardwareEncoder() override;

  std::string Device;
  HardwareEncoderTypeEnum HWEncoderType = HardwareEncoderTypeEnum::None;

  enum DesktopGPUVendor
  {
    None,
    AMD,
    Intel,
    NVIDIA
  };
  DesktopGPUVendor PreferredGPU = DesktopGPUVendor::None;

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
   * Implement parent class encoding and hardware encoder resource management.
   */
  bool SetupEncoderFrame(const int& width, const int& height) override;
  bool NeedsNewEncoderFrame(const int& width, const int& height) override;
  void TearDownEncoderFrame() override;
  void DrainInternal() override;
  EncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  ///@}

private:
  vtkFFMPEGHardwareEncoder(const vtkFFMPEGHardwareEncoder&) = delete;
  void operator=(const vtkFFMPEGHardwareEncoder&) = delete;

  std::unique_ptr<vtkFFMPEGEncoderInternals> Internals;
};

#endif // vtkFFMPEGHardwareEncoder_h
