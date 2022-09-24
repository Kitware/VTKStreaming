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
#include "vtkOpenGLIYUVRenderDelegate.h"
#include "vtkOpenGLNV12RenderDelegate.h"
#include "vtkOpenGLRGB24RenderDelegate.h"
#include "vtkOpenGLRGBA32RenderDelegate.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVideoFrameInternals.h"
#include "vtkPixelFormatTypes.h"
#include "vtkSmartPointer.h"

#include <sstream>
#include <vtk_glew.h>

#include <algorithm>

vtkStandardNewMacro(vtkOpenGLVideoFrame);

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::vtkOpenGLVideoFrame()
  : IYUVDelegate(std::unique_ptr<vtkOpenGLIYUVRenderDelegate>(new vtkOpenGLIYUVRenderDelegate()))
  , NV12Delegate(std::unique_ptr<vtkOpenGLNV12RenderDelegate>(new vtkOpenGLNV12RenderDelegate()))
  , RGB24Delegate(std::unique_ptr<vtkOpenGLRGB24RenderDelegate>(new vtkOpenGLRGB24RenderDelegate()))
  , RGBA32Delegate(
      std::unique_ptr<vtkOpenGLRGBA32RenderDelegate>(new vtkOpenGLRGBA32RenderDelegate()))
  , Internals(std::unique_ptr<vtkOpenGLVideoFrameInternals>(new vtkOpenGLVideoFrameInternals()))
{
}

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::~vtkOpenGLVideoFrame()
{
  this->ReleaseGraphicsResources();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Size: " << this->GetActualSize() << '\n';

  os << "Texture: \n";
  this->Internals->VtkTexture->PrintSelf(os, indent.GetNextIndent());

  os << "FrameBuffer: \n";
  this->Internals->VtkFrameBuffer->PrintSelf(os, indent.GetNextIndent());

  os << "Cache: \n";
  this->Internals->Cache->PrintSelf(os, indent.GetNextIndent());

  os << "Context: \n";
  if (this->Internals->VtkTexture->GetContext())
  {
    this->Internals->VtkTexture->GetContext()->PrintSelf(os, indent.GetNextIndent());
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::SetContext(vtkOpenGLRenderWindow* window)
{
  auto& internals = (*this->Internals);
  vtkLogScopeF(
    TRACE, "%s->%s, window=%s", vtkLogIdentifier(this), __func__, vtkLogIdentifier(window));

  internals.VtkTexture->SetContext(window);
  internals.VtkFrameBuffer->SetContext(window);

  // CreateFBO is a protected method.
  // indirectly invoke it by binding and unbind right after.
  internals.VtkFrameBuffer->Bind(GL_READ_FRAMEBUFFER);
  internals.VtkFrameBuffer->UnBind(GL_READ_FRAMEBUFFER);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::ReleaseGraphicsResources()
{
  auto& internals = (*this->Internals);
  auto myWindow = internals.VtkTexture->GetContext();
  vtkLogScopeF(
    TRACE, "%s->%s, myWindow=%s", vtkLogIdentifier(this), __func__, vtkLogIdentifier(myWindow));
  if (myWindow == nullptr)
  {
    return;
  }
  internals.VtkTexture->ReleaseGraphicsResources(myWindow);
  internals.VtkFrameBuffer->ReleaseGraphicsResources(myWindow);
  this->ActualSize = 0;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::Capture(vtkRenderWindow* window)
{
  auto& internals = (*this->Internals);
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(window);
  vtkLogScopeF(TRACE, "%s->%s, handle=%d, myWindow=%s, window=%s", vtkLogIdentifier(this), __func__,
    internals.VtkTexture->GetHandle(), vtkLogIdentifier(internals.VtkTexture->GetContext()),
    vtkLogIdentifier(oglRenWin));

  if (this->PixelFormat != VTKPixelFormatType::VTKPF_RGBA32)
  {
    vtkLogF(ERROR, "Capture API only supports RGBA32 pixel format.");
    return;
  }

  oglRenWin->MakeCurrent();
  oglRenWin->GetState()->PushReadFramebufferBinding();
  oglRenWin->GetDisplayFramebuffer()->Bind(GL_READ_FRAMEBUFFER);
  oglRenWin->GetDisplayFramebuffer()->ActivateReadBuffer(0);

  internals.VtkTexture->Bind();

  if (this->SliceOrder == vtkRawVideoFrame::SliceOrderType::BottomUp)
  {
    glCopyTexSubImage2D(
      internals.VtkTexture->GetTarget(), 0, 0, 0, 0, 0, this->Width, this->Height);
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
      glCopyTexSubImage2D(
        internals.VtkTexture->GetTarget(), 0, xofst, yofst, xsrc, ysrc, width, height);
    }
  }
  vtkOpenGLCheckErrorMacro("FBO->Internals->VtkTexture xfer failed ");
  glBindTexture(internals.VtkTexture->GetTarget(), 0);
  oglRenWin->GetState()->PopReadFramebufferBinding();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::UploadData(unsigned char* data)
{
  auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, Texture=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), internals.VtkTexture->GetHandle());

  // recover key attributes of the texture.
  auto& tex = internals.VtkTexture;
  const GLenum& target = tex->GetTarget();
  const GLenum& format = tex->GetFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
  const GLenum& datatype = tex->GetDataType(tex->GetVTKDataType());
  const auto& width = tex->GetWidth();
  const auto& height = tex->GetHeight();
  tex->GetContext()->MakeCurrent();
  tex->Bind();
  glTexSubImage2D(target, 0, 0, 0, width, height, format, datatype, data);
  glBindTexture(target, 0);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::CopyDataInternal(unsigned char* data, unsigned int size)
{
  const auto& internals = (*this->Internals);

  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, Texture=%d Size=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), internals.VtkTexture->GetHandle(), size);

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
unsigned int vtkOpenGLVideoFrame::GetDataInternal(unsigned char*& data)
{
  auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, handle=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), internals.VtkTexture->GetHandle());

  if (internals.Cache->GetMTime() > this->MTime)
  {
    data = internals.Cache->GetPointer(0);
    return internals.Cache->GetNumberOfValues();
  }
  internals.Cache->SetNumberOfValues(this->ActualSize);
  data = internals.Cache->GetPointer(0);
  std::fill(data, data + this->ActualSize, 0);

  auto& tex = internals.VtkTexture;
  tex->GetContext()->MakeCurrent();
  tex->Activate();
  const GLenum& target = tex->GetTarget();
  const GLenum& format = tex->GetFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
  const GLenum& datatype = tex->GetDataType(tex->GetVTKDataType());
  vtkLogF(TRACE, "Fetch %d bytes..", this->ActualSize);
  glGetTexImage(target, 0, format, datatype, data);
  vtkOpenGLCheckErrorMacro("Failed after glGetTexImage. ");
  tex->Deactivate();
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
  auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, handle=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), internals.VtkTexture->GetHandle());
  auto myWindow = internals.VtkTexture->GetContext();
  if (myWindow == nullptr)
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

  internals.VtkTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
  internals.VtkTexture->SetMinificationFilter(vtkTextureObject::Nearest);
  internals.VtkTexture->SetWrapS(vtkTextureObject::Repeat);
  internals.VtkTexture->SetWrapT(vtkTextureObject::Repeat);

  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      internals.VtkTexture->Allocate2D(this->Width, this->Height, 4, dataType, 0);
      this->ActualSize = widthBytes * this->Height;
      vtkLogF(TRACE, "WidthBytes: %d, Height: %d", widthBytes, this->Height);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      internals.VtkTexture->Allocate2D(this->Width, this->Height, 3, dataType, 0);
      this->ActualSize = widthBytes * this->Height;
      vtkLogF(TRACE, "WidthBytes: %d, Height: %d", widthBytes, this->Height);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      internals.VtkTexture->Allocate2D(
        this->Strides[0], this->Height + chromaHeight, 1, dataType, 0);
      this->ActualSize = this->Strides[0] * (this->Height + chromaHeight);
      vtkLogF(TRACE, "Strides: %d|%d|%d, Height: %d", this->Strides[0], this->Strides[1],
        this->Strides[2], this->Height);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      internals.VtkTexture->Allocate2D(
        this->Strides[0], this->Height + chromaHeight, 1, dataType, 0);
      this->ActualSize = this->Strides[0] * (this->Height + chromaHeight);
      vtkLogF(TRACE, "Strides: %d|%d|%d, Height: %d", this->Strides[0], this->Strides[1],
        this->Strides[2], this->Height);
      break;
  }
  vtkLogF(TRACE, "ActualSize: %d", this->ActualSize);

  vtkLogF(TRACE, "handle=%d, internalformat=%06x, format=%06x ", internals.VtkTexture->GetHandle(),
    internals.VtkTexture->GetInternalFormat(dataType, 1, 0),
    internals.VtkTexture->GetFormat(dataType, 1, 0));

  this->Modified();
}

//------------------------------------------------------------------------------
void* vtkOpenGLVideoFrame::GetResourceHandle() noexcept
{
  auto& internals = (*this->Internals);
  return internals.VtkTexture;
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::ShallowCopy(vtkRawVideoFrame* from) noexcept
{
  vtkLogScopeF(TRACE, "%s->%s", vtkLogIdentifier(this), __func__);
  this->Superclass::ShallowCopy(from);

  if (auto glFrame = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    auto& internals = (*this->Internals);
    // recover key attributes of the texture.
    auto& tex = glFrame->Internals->VtkTexture;
    const auto& handle = tex->GetHandle();
    const GLenum& target = tex->GetTarget();
    const GLenum& format = tex->GetFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
    const GLenum& internalFormat =
      tex->GetInternalFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
    const GLenum& dataType = tex->GetDataType(tex->GetVTKDataType());
    const auto& width = tex->GetWidth();
    const auto& height = tex->GetHeight();

    internals.VtkTexture->GetContext()->MakeCurrent();
    internals.VtkTexture->ReleaseGraphicsResources(internals.VtkTexture->GetContext());
    // reference the source texture.
    internals.VtkTexture->AssignToExistingTexture(handle, target);
    internals.VtkTexture->Resize(width, height);
    internals.VtkTexture->SetFormat(format);
    internals.VtkTexture->SetInternalFormat(internalFormat);
    internals.VtkTexture->SetDataType(dataType);
    this->ActualSize = glFrame->ActualSize;
  }
  else
  {
    // cannot shallow copy.
    this->DeepCopy(from);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::DeepCopy(vtkRawVideoFrame* from)
{
  vtkLogScopeF(TRACE, "%s->%s", vtkLogIdentifier(this), __func__);
  this->Superclass::DeepCopy(from);

  vtkSmartPointer<vtkTextureObject> srcTexture = nullptr;

  if (auto glInput = vtkOpenGLVideoFrame::SafeDownCast(from))
  {
    srcTexture = vtk::MakeSmartPointer(glInput->Internals->VtkTexture.GetPointer());
  }
  else
  {
    unsigned char* data = nullptr;
    const unsigned int size = from->GetData(data);
    this->CopyDataInternal(data, size);
    return;
  }

  auto& internals = (*this->Internals);
  const auto& width = internals.VtkTexture->GetWidth();
  const auto& height = internals.VtkTexture->GetHeight();

  internals.VtkFrameBuffer->SaveCurrentBindingsAndBuffers();
  internals.VtkFrameBuffer->AddColorAttachment(0, srcTexture, 0, srcTexture->GetTarget(), 0);
  vtkOpenGLCheckErrorMacro("Failed to add input texture to read framebuffer. ");

  internals.VtkFrameBuffer->CheckFrameBufferStatus(GL_FRAMEBUFFER);
  internals.VtkFrameBuffer->Bind(GL_READ_FRAMEBUFFER);
  vtkOpenGLCheckErrorMacro("Failed to bind read framebuffer. ");

  internals.VtkFrameBuffer->ActivateReadBuffer(0);
  vtkOpenGLCheckErrorMacro("Failed to activate read framebuffer. ");

  internals.VtkTexture->Bind();
  vtkOpenGLCheckErrorMacro("Failed to bind destination texture. ");

  vtkLogF(TRACE, "Copy %d bytes from texture %d -> %d", from->GetActualSize(),
    srcTexture->GetHandle(), internals.VtkTexture->GetHandle());

  if (this->SliceOrder == from->GetSliceOrderType())
  {
    glCopyTexSubImage2D(internals.VtkTexture->GetTarget(), 0, 0, 0, 0, 0, width, height);
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
      glCopyTexSubImage2D(
        internals.VtkTexture->GetTarget(), 0, xofst, yofst, xsrc, ysrc, width, height);
    }
  }
  vtkOpenGLCheckErrorMacro("FBO->Internals->VtkTexture  xfer failed ");
  glBindTexture(internals.VtkTexture->GetTarget(), 0);
  internals.VtkFrameBuffer->RemoveColorAttachments(0);
  internals.VtkFrameBuffer->RestorePreviousBindingsAndBuffers();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::Render(vtkRenderWindow* window)
{
  auto& internals = (*this->Internals);
  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(window);
  if (oglRenWin == nullptr)
  {
    return;
  }
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, renderInto=%s", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), vtkLogIdentifier(window));

  // when rendering into opengl, we may want to invert along Y dimension.
  bool invert_y = this->SliceOrder == vtkRawVideoFrame::SliceOrderType::TopDown;
  const int chromaHeight = vtkRawVideoFrame::GetChromaHeight(this->Height, this->PixelFormat);
  auto& tex = internals.VtkTexture;

  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      this->RGBA32Delegate->Render(tex, oglRenWin, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      this->RGB24Delegate->Render(tex, oglRenWin, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      this->NV12Delegate->Render(tex, oglRenWin, this->Strides, chromaHeight, invert_y);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      this->IYUVDelegate->Render(tex, oglRenWin, this->Strides, chromaHeight, invert_y);
      break;
  }
}
