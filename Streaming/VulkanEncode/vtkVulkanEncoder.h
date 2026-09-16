// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVulkanEncoder
 * @brief   hardware H.264 encoder built on the Vulkan Video encode extensions
 *
 * vtkVulkanEncoder requires an OpenGL `GraphicsContext` (see SetGraphicsContext()), exactly
 * like vtkNvEncoderGL. Every frame is captured straight from that render window's color
 * attachment into GL_EXT_memory_object-imported Vulkan luma/chroma images, copied on the GPU
 * into an NV12 encode image with no CPU readback, and encoded with
 * `VK_KHR_video_encode_h264`. Packets carry an Annex B elementary stream
 * with SPS/PPS prepended to every key frame, matching the NVENC and VideoToolbox backends.
 *
 * The encoder creates its own VkInstance/VkDevice through a runtime-loaded
 * `libvulkan.so.1`; there is no link-time dependency on Vulkan. The GPU driving the OpenGL
 * context must expose an H.264 encode queue, since the captured frames are shared with it
 * as exported device memory (on hybrid-GPU systems, run the window on that GPU, e.g. via
 * PRIME render offload).
 *
 * @sa vtkVideoEncoder, vtkNvEncoderGL, vtkVideoToolboxEncoder, vtkVulkanGLInterop
 */

#ifndef vtkVulkanEncoder_h
#define vtkVulkanEncoder_h

#include "vtkVideoEncoder.h"

#include "vtkStreamingVulkanEncodeModule.h" // for export macro

#include <memory> // for ivar

class vtkVulkanEncoderInternals;
class vtkVulkanGLInterop;
class vtkVulkanVideoDevice;

class VTKSTREAMINGVULKANENCODE_EXPORT vtkVulkanEncoder : public vtkVideoEncoder
{
public:
  vtkTypeMacro(vtkVulkanEncoder, vtkVideoEncoder);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVulkanEncoder* New();

  bool IsHardwareAccelerated() const noexcept override { return true; }
  vtkIdType GetLastEncodeTimeNS() const noexcept override;
  vtkIdType GetLastScaleTimeNS() const noexcept override;
  bool SupportsCodec(VTKVideoCodecType codec) const noexcept override;
  bool SupportsDirectCaptureFromVTKRenderWindow() const noexcept override { return true; }

  /**
   * True when the Vulkan loader is present and a GPU exposes H.264 video encoding.
   */
  static bool CheckAvailability() noexcept;

protected:
  vtkVulkanEncoder();
  ~vtkVulkanEncoder() override;

  bool InitializeInternal() override;
  void ShutdownInternal() override;

  bool SetupEncoderFrame(int width, int height) override;
  void TearDownEncoderFrame() override;

  VTKVideoEncoderResultType EncodeInternal(vtkSmartPointer<vtkRawVideoFrame> frame) override;
  VTKVideoEncoderResultType SendEOS() override;

private:
  vtkVulkanEncoder(const vtkVulkanEncoder&) = delete;
  void operator=(const vtkVulkanEncoder&) = delete;

  std::unique_ptr<vtkVulkanVideoDevice> Device;
  std::unique_ptr<vtkVulkanEncoderInternals> Internals;
  std::unique_ptr<vtkVulkanGLInterop> GLInterop; // only when GraphicsContext is OpenGL.
};

#endif // vtkVulkanEncoder_h
