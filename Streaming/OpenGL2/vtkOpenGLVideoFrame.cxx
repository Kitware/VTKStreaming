/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVideoFrame.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVideoFrame.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIYUVRenderDelegate.h"
#include "vtkOpenGLNV12RenderDelegate.h"
#include "vtkOpenGLRGB24RenderDelegate.h"
#include "vtkOpenGLRGBA32RenderDelegate.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkPixelBufferObject.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkType.h"

#include <cstddef>
#include <vtk_glew.h>

#include <algorithm>
#include <cassert>
#include <string>

vtkStandardNewMacro(vtkOpenGLVideoFrame);

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::vtkOpenGLVideoFrame()
  : Texture(vtkTextureObject::New())
  , FBO(vtkOpenGLFramebufferObject::New())
  , IYUVDelegate(std::unique_ptr<vtkOpenGLIYUVRenderDelegate>(new vtkOpenGLIYUVRenderDelegate()))
  , NV12Delegate(std::unique_ptr<vtkOpenGLNV12RenderDelegate>(new vtkOpenGLNV12RenderDelegate()))
  , RGB24Delegate(std::unique_ptr<vtkOpenGLRGB24RenderDelegate>(new vtkOpenGLRGB24RenderDelegate()))
  , RGBA32Delegate(
      std::unique_ptr<vtkOpenGLRGBA32RenderDelegate>(new vtkOpenGLRGBA32RenderDelegate()))
{
  this->Texture = vtkTextureObject::New();
  this->FBO = vtkOpenGLFramebufferObject::New();
}

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::~vtkOpenGLVideoFrame()
{
  this->ReleaseGraphicsResources();
  if (this->Texture != nullptr)
  {
    this->Texture->Delete();
    this->Texture = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Size: " << this->GetActualSize() << '\n';
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::InitializeGraphicsResources(vtkRenderWindow* window)
{
  vtkLogScopeF(TRACE, "%s, window=%s", __func__, vtkLogIdentifier(window));
  if (this->Texture->GetContext() != nullptr && this->FBO->GetContext() != nullptr)
  {
    this->ReleaseGraphicsResources();
  }
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(window);
  this->Texture->SetContext(oglRenWin);
  this->FBO->SetContext(oglRenWin);
  this->FBO->Bind(GL_READ_FRAMEBUFFER);
  this->FBO->UnBind(GL_READ_FRAMEBUFFER);
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::ReleaseGraphicsResources()
{
  auto oglRenWin = this->Texture->GetContext();
  if (oglRenWin == nullptr)
  {
    return;
  }
  this->Texture->ReleaseGraphicsResources(oglRenWin);
  this->FBO->ReleaseGraphicsResources(oglRenWin);
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::Capture(vtkRenderWindow* window)
{
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(window);
  vtkLogScopeF(TRACE, "%s->%s, currentContext=%s, window=%s", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(this->Texture->GetContext()), vtkLogIdentifier(oglRenWin));
  if (this->Texture->GetContext() != oglRenWin)
  {
    vtkLogF(TRACE, "Invalid opengl context %s. Current context = %s", vtkLogIdentifier(oglRenWin),
      vtkLogIdentifier(this->Texture->GetContext()));
    return;
  }
  if (this->PixelFormat != VTKPixelFormatType::VTKPF_RGBA32)
  {
    vtkLogF(ERROR, "Capture API supports only RGBA32 pictures.");
    return;
  }

  oglRenWin->MakeCurrent();
  oglRenWin->GetState()->PushReadFramebufferBinding();
  oglRenWin->GetDisplayFramebuffer()->Bind(GL_READ_FRAMEBUFFER);
  oglRenWin->GetDisplayFramebuffer()->ActivateReadBuffer(0);

  this->Texture->Bind();
  if (this->SliceOrder == vtkRawVideoFrame::SliceOrderType::BottomUp)
  {
    glCopyTexSubImage2D(this->Texture->GetTarget(), 0, 0, 0, 0, 0, this->Width, this->Height);
  }
  else
  {
    vtkLog(TRACE, "Desired slice order is TopDown. Will invert picture along Y dimension.");
    for (int i1 = 0, i2 = this->Height - 1; i1 < this->Height && i2 >= 0; ++i1, --i2)
    {
      int xofst = 0;
      int yofst = i2;
      int xsrc = 0;
      int ysrc = i1;
      int width = this->Width;
      int height = 1;
      glCopyTexSubImage2D(this->Texture->GetTarget(), 0, xofst, yofst, xsrc, ysrc, width, height);
    }
  }
  vtkOpenGLCheckErrorMacro("FBO->Texture xfer failed ");

  oglRenWin->GetState()->PopReadFramebufferBinding();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::UploadData(unsigned char* data)
{
  vtkLogScopeF(TRACE, "%s->%s currentContext=%s, Texture=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(this->Texture->GetContext()), this->Texture->GetHandle());
  // recover key attributes of the texture.
  auto tex = this->Texture;
  tex->Bind();
  const GLenum& target = tex->GetTarget();
  const GLenum& format = tex->GetFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
  const GLenum& datatype = tex->GetDataType(tex->GetVTKDataType());
  const auto& width = tex->GetWidth();
  const auto& height = tex->GetHeight();
  glTexSubImage2D(target, 0, 0, 0, width, height, format, datatype, data);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::CopyDataInternal(unsigned char* data, unsigned int size)
{
  vtkLogScopeF(TRACE, "%s->%s currentContext=%s, Texture=%d Size=%d", vtkLogIdentifier(this),
    __func__, vtkLogIdentifier(this->Texture->GetContext()), this->Texture->GetHandle(), size);
  const int allocSize = this->GetActualSize();
  if (size > allocSize)
  {
    this->AllocateDataStore();
    this->UploadData(data);
  }
  else if (size < allocSize)
  {
    vtkLog(TRACE, << "Padding reason: Given data size " << size << " smaller than frame size "
                  << allocSize << " - " << this->Width << "x" << this->Height
                  << " for pixel format "
                  << vtkPixelFormatTypeUtilities::ToString(this->PixelFormat));
    std::vector<unsigned char> padded(allocSize);
    std::copy(data, data + size, padded.begin());
    // pad with zeros.
    std::fill(padded.begin() + size, padded.end(), 0);
    this->UploadData(padded.data());
  }
  else
  {
    this->UploadData(data);
  }
}

//------------------------------------------------------------------------------
unsigned int vtkOpenGLVideoFrame::GetDataInternal(unsigned char*& data) const
{
  vtkLogScopeF(TRACE, "%p->%s currentContext=%s, Texture=%d", this, __func__,
    vtkLogIdentifier(this->Texture->GetContext()), this->Texture->GetHandle());

  if (this->Cache->GetMTime() > this->MTime)
  {
    data = this->Cache->GetPointer(0);
    return this->Cache->GetNumberOfValues();
  }
  vtkLogF(TRACE, "Resource Hdl (%d)", this->Texture->GetHandle());
  this->Cache->SetNumberOfValues(this->ActualSize);
  data = this->Cache->GetPointer(0);
  std::fill(data, data + this->ActualSize, 0);

  this->Texture->Bind();
  const GLenum& target = this->Texture->GetTarget();
  const GLenum& format = this->Texture->GetFormat(
    this->Texture->GetVTKDataType(), this->Texture->GetComponents(), false);
  const GLenum& datatype = this->Texture->GetDataType(this->Texture->GetVTKDataType());
  glGetTexImage(target, 0, format, datatype, data);
  return this->ActualSize;
}

//------------------------------------------------------------------------------
unsigned int vtkOpenGLVideoFrame::GetActualSize() const
{
  return this->ActualSize;
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::AllocateDataStore()
{
  vtkLogScopeF(TRACE, "%s->%s currentContext=%s, Texture=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(this->Texture->GetContext()), this->Texture->GetHandle());
  auto oglRenWin = this->Texture->GetContext();
  if (oglRenWin == nullptr)
  {
    vtkLog(
      ERROR, << "An OpenGL render window is required. Call ::InitializeGraphicsResources with a "
                "vtkOpenGLRenderWindow instance.");
    return;
  }
  const auto estimate =
    vtkRawVideoFrame::GetEstimatedSize(this->Width, this->Height, this->PixelFormat, this->Strides);
  if (this->ActualSize == estimate)
  {
    return;
  }

  const unsigned int chromaHeight =
    vtkRawVideoFrame::GetChromaHeight(this->Height, this->PixelFormat);
  const unsigned int numCrPlanes = vtkRawVideoFrame::GetNumberOfChromaPlanes(this->PixelFormat);
  const unsigned int widthBytes = vtkRawVideoFrame::GetWidthBytes(this->Width, this->PixelFormat);
  const int dataType = VTK_UNSIGNED_CHAR;

  this->Texture->SetMagnificationFilter(vtkTextureObject::Nearest);
  this->Texture->SetMinificationFilter(vtkTextureObject::Nearest);
  this->Texture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->Texture->SetWrapT(vtkTextureObject::ClampToEdge);

  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      this->Texture->Allocate2D(this->Width, this->Height, 4, dataType, 0);
      this->ActualSize = widthBytes * this->Height;
      vtkLogF(TRACE, "WidthBytes: %d, Height: %d", widthBytes, this->Height);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      this->Texture->Allocate2D(this->Width, this->Height, 3, dataType, 0);
      this->ActualSize = widthBytes * this->Height;
      vtkLogF(TRACE, "WidthBytes: %d, Height: %d", widthBytes, this->Height);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      this->Texture->Allocate2D(this->Strides[0], this->Height + chromaHeight, 1, dataType, 0);
      this->ActualSize = this->Strides[0] * (this->Height + chromaHeight);
      vtkLogF(TRACE, "Strides: %d|%d|%d, Height: %d", this->Strides[0], this->Strides[1],
        this->Strides[2], this->Height);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      this->Texture->Allocate2D(this->Strides[0], this->Height + chromaHeight, 1, dataType, 0);
      this->ActualSize = this->Strides[0] * (this->Height + chromaHeight);
      vtkLogF(TRACE, "Strides: %d|%d|%d, Height: %d", this->Strides[0], this->Strides[1],
        this->Strides[2], this->Height);
      break;
  }
  vtkLogF(TRACE, "ActualSize: %d", this->ActualSize);

  vtkLogF(TRACE, "tex=%d, internalformat=%06x, format=%06x ", this->Texture->GetHandle(),
    this->Texture->GetInternalFormat(dataType, 1, 0), this->Texture->GetFormat(dataType, 1, 0));

  this->Modified();
}

//------------------------------------------------------------------------------
void* vtkOpenGLVideoFrame::GetResourceHandle() noexcept
{
  if (this->AttachedWindow == nullptr)
  {
    return this->Texture;
  }
  else
  {
    auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(this->AttachedWindow);
    return oglRenWin->GetDisplayFramebuffer()->GetColorAttachmentAsTextureObject(0);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::ShallowCopy(vtkRawVideoFrame* from) noexcept
{
  this->Superclass::ShallowCopy(from);
  if (auto glFrame = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    this->ReleaseGraphicsResources();
    this->InitializeGraphicsResources(glFrame->Texture->GetContext());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::CopyFrameDataInternal(vtkRawVideoFrame* from)
{
  vtkSmartPointer<vtkTextureObject> srcTexture = nullptr;

  if (auto glInput = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    srcTexture = vtk::MakeSmartPointer(glInput->Texture);
  }
  else
  {
    // use an intermediate opengl frame to aid in the transfer.
    unsigned char* data = nullptr;
    const unsigned int size = from->GetData(data);
    this->CopyDataInternal(data, size);
    return;
  }

  const auto& width = this->Texture->GetWidth();
  const auto& height = this->Texture->GetHeight();

  this->FBO->SaveCurrentBindingsAndBuffers();
  this->FBO->AddColorAttachment(0, srcTexture, 0, srcTexture->GetTarget(), 0);
  vtkOpenGLCheckErrorMacro("Failed to add input texture to read framebuffer. ");

  this->FBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);
  this->FBO->Bind(GL_READ_FRAMEBUFFER);
  vtkOpenGLCheckErrorMacro("Failed to bind read framebuffer. ");

  this->FBO->ActivateReadBuffer(0);
  vtkOpenGLCheckErrorMacro("Failed to activate read framebuffer. ");

  this->Texture->Bind();
  vtkOpenGLCheckErrorMacro("Failed to bind destination texture. ");

  if (this->SliceOrder == from->GetSliceOrderType())
  {
    glCopyTexSubImage2D(this->Texture->GetTarget(), 0, 0, 0, 0, 0, width, height);
  }
  else
  {
    vtkLog(TRACE, "Desired slice order is TopDown. Will invert picture along Y dimension.");
    for (int i1 = 0, i2 = height - 1; i1 < height && i2 >= 0; ++i1, --i2)
    {
      int xofst = 0;
      int yofst = i2;
      int xsrc = 0;
      int ysrc = i1;
      int width = this->Width;
      int height = 1;
      glCopyTexSubImage2D(this->Texture->GetTarget(), 0, xofst, yofst, xsrc, ysrc, width, height);
    }
  }
  vtkOpenGLCheckErrorMacro("FBO->Texture xfer failed ");
  this->FBO->RemoveColorAttachments(0);
  this->FBO->RestorePreviousBindingsAndBuffers();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::Render(vtkRenderWindow* window)
{
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(window);
  if (oglRenWin == nullptr)
  {
    return;
  }
  // when rendering into opengl, we may want to invert along Y dimension.
  bool invert_y = this->SliceOrder == vtkRawVideoFrame::SliceOrderType::TopDown;
  const int chromaHeight = vtkRawVideoFrame::GetChromaHeight(this->Height, this->PixelFormat);

  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      this->RGBA32Delegate->Render(this->Texture, oglRenWin, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      this->RGB24Delegate->Render(this->Texture, oglRenWin, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      this->NV12Delegate->Render(this->Texture, oglRenWin, this->Strides, chromaHeight, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      this->IYUVDelegate->Render(this->Texture, oglRenWin, this->Strides, chromaHeight, invert_y);
      break;
  }
}
