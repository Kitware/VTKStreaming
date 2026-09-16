// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "vtkVulkanGLInterop.h"

#include "vtkLogger.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkTextureObject.h"
#include "vtkVulkanEncoderInternals.h"

#include <unistd.h> // for close()

//------------------------------------------------------------------------------
vtkVulkanGLInterop::vtkVulkanGLInterop() = default;

//------------------------------------------------------------------------------
vtkVulkanGLInterop::~vtkVulkanGLInterop() = default;

//------------------------------------------------------------------------------
bool vtkVulkanGLInterop::GetDeviceUUID(vtkOpenGLRenderWindow* window, unsigned char uuid[16])
{
  window->MakeCurrent();
  if (!GLAD_GL_EXT_memory_object)
  {
    return false;
  }
  glGetUnsignedBytevEXT(GL_DEVICE_UUID_EXT, uuid);
  return true;
}

//------------------------------------------------------------------------------
bool vtkVulkanGLInterop::Import(
  vtkOpenGLRenderWindow* window, const vtkVulkanEncoderInternals& internals, uint32_t width,
  uint32_t height)
{
  if (!GLAD_GL_EXT_memory_object_fd || !GLAD_GL_EXT_semaphore_fd)
  {
    vtkLog(ERROR, "The OpenGL context lacks GL_EXT_memory_object_fd/GL_EXT_semaphore_fd.");
    return false;
  }

  const int fds[3] = { internals.ExportInputMemoryFd(), internals.ExportReadySemaphoreFd(),
    internals.ExportFreeSemaphoreFd() };
  if (fds[0] < 0 || fds[1] < 0 || fds[2] < 0)
  {
    vtkLog(ERROR, "Failed to export Vulkan memory/semaphore file descriptors for GL import.");
    for (int fd : fds)
    {
      if (fd >= 0)
      {
        close(fd);
      }
    }
    return false;
  }

  // Drain stale errors so the check below only reports this import.
  while (glGetError() != GL_NO_ERROR)
  {
  }

  glCreateMemoryObjectsEXT(1, &this->MemoryObject);
  // Must mirror the VkMemoryDedicatedAllocateInfo allocation on the Vulkan side.
  const GLint dedicated = GL_TRUE;
  glMemoryObjectParameterivEXT(this->MemoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);
  // glImportMemoryFdEXT takes ownership of the fd on success; it must not be close()d here.
  glImportMemoryFdEXT(
    this->MemoryObject, internals.GetInputMemorySize(), GL_HANDLE_TYPE_OPAQUE_FD_EXT, fds[0]);
  glCreateTextures(GL_TEXTURE_2D, 1, &this->Texture);
  glTextureStorageMem2DEXT(this->Texture, 1, GL_RGBA8, static_cast<GLsizei>(width),
    static_cast<GLsizei>(height), this->MemoryObject, 0);

  glGenSemaphoresEXT(1, &this->ReadySemaphore);
  glGenSemaphoresEXT(1, &this->FreeSemaphore);
  glImportSemaphoreFdEXT(this->ReadySemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fds[1]);
  glImportSemaphoreFdEXT(this->FreeSemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fds[2]);

  GLenum error = glGetError();
  if (error != GL_NO_ERROR)
  {
    vtkLogF(ERROR, "Importing the Vulkan input image/semaphores into OpenGL failed (0x%x).",
      error);
    this->Destroy(window);
    return false;
  }

  this->TextureObject = vtkSmartPointer<vtkTextureObject>::New();
  this->TextureObject->SetContext(window);
  this->TextureObject->AssignToExistingTexture(this->Texture, GL_TEXTURE_2D);

  // The blit's draw framebuffer: attached once, bound per frame. AddColorAttachment is a
  // no-op until the GL FBO exists, and Bind() is what creates it.
  vtkOpenGLState* state = window->GetState();
  this->Framebuffer = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
  this->Framebuffer->SetContext(window);
  state->PushDrawFramebufferBinding();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);
  this->Framebuffer->AddColorAttachment(0, this->TextureObject, 0, GL_TEXTURE_2D, 0);
  this->Framebuffer->ActivateDrawBuffers(1);
  const bool complete = this->Framebuffer->CheckFrameBufferStatus(GL_DRAW_FRAMEBUFFER) != 0;
  state->PopDrawFramebufferBinding();
  error = glGetError();
  if (!complete || error != GL_NO_ERROR)
  {
    vtkLogF(ERROR, "The imported Vulkan input image is not a usable GL draw framebuffer (0x%x).",
      error);
    this->Destroy(window);
    return false;
  }

  this->Ready = true;
  return true;
}

//------------------------------------------------------------------------------
void vtkVulkanGLInterop::Destroy(vtkOpenGLRenderWindow* window)
{
  if (this->Framebuffer)
  {
    this->Framebuffer->ReleaseGraphicsResources(window);
    this->Framebuffer = nullptr;
  }
  if (this->ReadySemaphore != 0 || this->FreeSemaphore != 0)
  {
    const GLuint semaphores[] = { this->ReadySemaphore, this->FreeSemaphore };
    glDeleteSemaphoresEXT(2, semaphores);
    this->ReadySemaphore = this->FreeSemaphore = 0;
  }
  this->TextureObject = nullptr;
  if (this->Texture != 0)
  {
    glDeleteTextures(1, &this->Texture);
    this->Texture = 0;
  }
  if (this->MemoryObject != 0)
  {
    glDeleteMemoryObjectsEXT(1, &this->MemoryObject);
    this->MemoryObject = 0;
  }
  this->Ready = false;
}

//------------------------------------------------------------------------------
bool vtkVulkanGLInterop::CaptureFrame(vtkOpenGLRenderWindow* window, int width, int height)
{
  if (!this->Ready)
  {
    return false;
  }
  const GLenum generalLayout = GL_LAYOUT_GENERAL_EXT;

  // Blocks until Vulkan has finished reading the previous frame out of the input image.
  glWaitSemaphoreEXT(this->FreeSemaphore, 0, nullptr, 1, &this->Texture, &generalLayout);

  vtkOpenGLState* state = window->GetState();
  vtkOpenGLState::ScopedglViewport viewportSave(state);
  vtkOpenGLState::ScopedglScissor scissorSave(state);
  state->PushDrawFramebufferBinding();
  this->Framebuffer->Bind(GL_DRAW_FRAMEBUFFER);
  // Straight copy of the colour buffer; the Vulkan compute pass handles the Y flip.
  window->BlitDisplayFramebuffer(
    0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  state->PopDrawFramebufferBinding();

  // Hands the input image to Vulkan; EncodeFrame() waits on this semaphore.
  glSignalSemaphoreEXT(this->ReadySemaphore, 0, nullptr, 1, &this->Texture, &generalLayout);
  return true;
}
