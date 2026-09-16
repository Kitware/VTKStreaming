// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVulkanGLInterop
 * @brief   imports vtkVulkanEncoderInternals' exportable input image into OpenGL.
 *
 * Owns the GL-side half of the OpenGL/Vulkan interop path: one GL memory object (the
 * encoder's RGBA8 input image), a GL texture wrapping it, a framebuffer with that texture
 * attached, and two imported GL semaphores synchronized with vtkVulkanEncoderInternals'
 * `ReadySemaphore`/`FreeSemaphore`. Capturing a frame is a single glBlitFramebuffer of the
 * render window's display framebuffer into the imported texture; the colour conversion to
 * NV12 happens on the Vulkan side in a compute shader.
 *
 * GL_EXT_memory_object/GL_EXT_semaphore entry points come from VTK's own glad loader,
 * which resolves them through the window system's GetProcAddress when the context is
 * created; the GL context must therefore run on the same GPU as the Vulkan device.
 *
 * @sa vtkVulkanEncoder, vtkVulkanEncoderInternals
 */

#ifndef vtkVulkanGLInterop_h
#define vtkVulkanGLInterop_h

#include "vtkStreamingVulkanEncodeModule.h"

#include "vtkSmartPointer.h"
#include "vtk_glad.h" // for GLuint

class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkTextureObject;
class vtkVulkanEncoderInternals;

class VTKSTREAMINGVULKANENCODE_NO_EXPORT vtkVulkanGLInterop
{
public:
  vtkVulkanGLInterop();
  ~vtkVulkanGLInterop();
  vtkVulkanGLInterop(const vtkVulkanGLInterop&) = delete;
  void operator=(const vtkVulkanGLInterop&) = delete;

  /**
   * Writes the 16-byte GL_DEVICE_UUID_EXT of the GPU driving `window`'s context into `uuid`,
   * which matches VkPhysicalDeviceIDProperties::deviceUUID of the same GPU. False when the
   * context lacks GL_EXT_memory_object.
   */
  static bool GetDeviceUUID(vtkOpenGLRenderWindow* window, unsigned char uuid[16]);

  /**
   * Imports the encoder's exportable input image and semaphores into the given window's GL
   * context. `internals` must already have interop enabled and be set up (Setup() called) so
   * the exportable image/semaphores exist. `width`/`height` are the image's (aligned) size.
   */
  bool Import(vtkOpenGLRenderWindow* window, const vtkVulkanEncoderInternals& internals,
    uint32_t width, uint32_t height);
  void Destroy(vtkOpenGLRenderWindow* window);
  bool IsReady() const { return this->Ready; }

  /**
   * Waits for the "image free to reuse" semaphore signaled by Vulkan (or, on the very
   * first frame, the initial layout-transition signal from Setup()), blits the
   * `width x height` colour buffer of `window`'s display framebuffer into the imported
   * texture, then signals "image ready to encode" for Vulkan to wait on. The picture keeps
   * GL's bottom-up row order; the Vulkan side flips it.
   */
  bool CaptureFrame(vtkOpenGLRenderWindow* window, int width, int height);

private:
  bool Ready = false;
  GLuint MemoryObject = 0;
  GLuint Texture = 0; // RGBA8, the encoder's input image.
  GLuint ReadySemaphore = 0;
  GLuint FreeSemaphore = 0;
  vtkSmartPointer<vtkTextureObject> TextureObject;
  vtkSmartPointer<vtkOpenGLFramebufferObject> Framebuffer;
};

#endif // vtkVulkanGLInterop_h
// VTK-HeaderTest-Exclude: vtkVulkanGLInterop.h
