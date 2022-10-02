/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFmpegHardwareEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFmpegHardwareEncoder
 * @brief   this class implements hardware accelerated (GPU-based) encoder.
 *
 * It configures and uses FFmpeg with a hardware accelerated encoder.
 * The support for encoder codec varies.
 *
 * @sa vtkVideoEncoder, vtkCompressedVideoPacket
 */

#ifndef vtkFFmpegHardwareEncoder_h
#define vtkFFmpegHardwareEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkCompressedVideoPacket.h"       // for arg
#include "vtkSmartPointer.h"                // for arg
#include "vtkStreamingFFmpegEncodeModule.h" // for export macro
#include "vtkVideoCodecTypes.h"             // for enum
#include "vtkVideoProcessingStatusTypes.h"  // for enum

#include <memory> // for ivar
#include <string> // for arg

class vtkFFmpegEncoderInternals;

class VTKSTREAMINGFFMPEGENCODE_EXPORT vtkFFmpegHardwareEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkFFmpegHardwareEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkFFmpegHardwareEncoder* New();

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
  bool IsHardwareAccelerated() const noexcept override { return true; }
  bool SupportsAsyncMode() const noexcept override { return true; }
  bool SupportsZeroCopy() const noexcept override { return false; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
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
  vtkFFmpegHardwareEncoder();
  ~vtkFFmpegHardwareEncoder() override;

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
  bool SetupEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;
  VTKVideoEncoderResultType DrainInternal() override;
  VTKVideoProcessingStatusType PushInternal(vtkRawVideoFrame* frame) override;
  VTKVideoEncoderResultType GetResultInternal() override;
  VTKVideoEncoderResultType EncodeInternal(vtkRawVideoFrame* frame) override;
  ///@}

  VTKVideoEncoderResultType EncodeDisplayInternal() override;

private:
  vtkFFmpegHardwareEncoder(const vtkFFmpegHardwareEncoder&) = delete;
  void operator=(const vtkFFmpegHardwareEncoder&) = delete;

  std::unique_ptr<vtkFFmpegEncoderInternals> Internals;
};

#endif // vtkFFmpegHardwareEncoder_h
