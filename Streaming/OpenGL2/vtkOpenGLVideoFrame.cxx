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
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVideoFrameCapture.h"
#include "vtkOpenGLVideoFrameInternals.h"
#include "vtkOpenGLVideoFrameRenderer.h"
#include "vtkPixelFormatTypes.h"
#include "vtkSmartPointer.h"
#include "vtk_glad.h"

#include <algorithm>

vtkStandardNewMacro(vtkOpenGLVideoFrame);

#define ENSURE_MAIN_THREAD                                                                         \
  do                                                                                               \
  {                                                                                                \
    if (this->Internals->Tid != std::this_thread::get_id())                                        \
    {                                                                                              \
      vtkLog(ERROR, "Attempted to execute thread-unsafe code.");                                   \
      abort();                                                                                     \
    }                                                                                              \
  } while (0)

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::vtkOpenGLVideoFrame()
  : FrameGrabber(std::unique_ptr<vtkOpenGLVideoFrameCapture>(new vtkOpenGLVideoFrameCapture()))
  , FrameRenderer(std::unique_ptr<vtkOpenGLVideoFrameRenderer>(new vtkOpenGLVideoFrameRenderer()))
  , Internals(std::unique_ptr<vtkOpenGLVideoFrameInternals>(new vtkOpenGLVideoFrameInternals()))
{
}

//------------------------------------------------------------------------------
vtkOpenGLVideoFrame::~vtkOpenGLVideoFrame()
{
  ENSURE_MAIN_THREAD;
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
  if (myWindow == nullptr)
  {
    return;
  }

  this->FrameGrabber->ReleaseGraphicsResources(myWindow);
  this->FrameRenderer->ReleaseGraphicsResources(myWindow);
  vtkLogScopeF(
    TRACE, "%s->%s, myWindow=%s", vtkLogIdentifier(this), __func__, vtkLogIdentifier(myWindow));
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

  const bool invert_y = this->SliceOrder == vtkRawVideoFrame::SliceOrderType::TopDown;
  const int chromaHeight =
    vtkRawVideoFrame::GetChromaHeight(this->DisplayHeight, this->PixelFormat);
  const int lumaHeight = this->StorageHeight;

  internals.VtkFrameBuffer->SaveCurrentBindingsAndBuffers();
  internals.VtkFrameBuffer->AddColorAttachment(
    0, internals.VtkTexture, 0, internals.VtkTexture->GetTarget(), 0);
  vtkOpenGLCheckErrorMacro("Failed to add output texture to read framebuffer. ");

  // Some applications may not have a default framebuffer (ex: offscreen rendering).
  // bind ours, then check status.
  internals.VtkFrameBuffer->Bind(GL_DRAW_FRAMEBUFFER);
  internals.VtkFrameBuffer->CheckFrameBufferStatus(GL_DRAW_FRAMEBUFFER);
  internals.VtkFrameBuffer->ActivateDrawBuffers(1);
  vtkOpenGLCheckErrorMacro("Failed to bind draw framebuffer. ");

  auto rgba32Texture = oglRenWin->GetDisplayFramebuffer()->GetColorAttachmentAsTextureObject(0);
  // video frames do not care about alpha-channel anyway, so skip it always.
  this->FrameGrabber->Capture(rgba32Texture, this->PixelFormat, oglRenWin, this->StorageWidth,
    this->StorageHeight, lumaHeight, chromaHeight, this->Strides, invert_y,
    /*ignore_alpha=*/true);
  vtkOpenGLCheckErrors("ERROR capturing render window. ");

  internals.VtkFrameBuffer->RemoveColorAttachments(0);
  internals.VtkFrameBuffer->RestorePreviousBindingsAndBuffers();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::CopyDataInternal(unsigned char* data, int rowsize, int numrows)
{
  const auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s tex=%d nr=%d rsz=%d", vtkLogIdentifier(this), __func__,
    internals.VtkTexture->GetHandle(), numrows, rowsize);

  const int allocSize = this->GetActualSize();
  const int size = numrows * rowsize;
  if (size > allocSize)
  {
    this->AllocateDataStore();
    this->UploadData(data, rowsize, numrows, 0);
  }
  else
  {
    this->UploadData(data, rowsize, numrows, 0);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::CopyPlanarDataInternal(
  unsigned char* data, int rowsize, int numrows, int plane)
{
  const auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s tex=%d nr=%d rsz=%d plane=%d", vtkLogIdentifier(this), __func__,
    internals.VtkTexture->GetHandle(), numrows, rowsize, plane);
  this->UploadData(data, rowsize, numrows, plane);
}

//------------------------------------------------------------------------------
void vtkOpenGLVideoFrame::UploadData(unsigned char* from, int rowsize, int numrows, int plane)
{
  const auto& internals = (*this->Internals);
  vtkLogScopeFunction(TRACE);

  const auto chromaHeight =
    vtkRawVideoFrame::GetChromaHeight(this->DisplayHeight, this->PixelFormat);

  // recover key attributes of the texture.
  auto& tex = internals.VtkTexture;
  const GLenum& target = tex->GetTarget();
  const GLenum& format = tex->GetFormat(tex->GetVTKDataType(), tex->GetComponents(), false);
  const GLenum& datatype = tex->GetDataType(tex->GetVTKDataType());
  const int& components = tex->GetComponents();

  tex->GetContext()->MakeCurrent();
  tex->Bind();
  auto ostate = tex->GetContext()->GetState();
  GLint oldRowLength = 0, oldRowAlign = 0;
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldRowAlign);

  int yofst = 0;
  int ymin = 0;
  int ymax = 0;
  unsigned char* dptr = from;
  int rowId = 0;
  int width = tex->GetWidth();
  if (this->PixelFormat == VTKPixelFormatType::VTKPF_IYUV)
  {
    switch (plane)
    {
      case 0:
        ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, rowsize);
        glTexSubImage2D(target, 0, 0, 0, width, this->StorageHeight, format, datatype, dptr);
        break;
      case 1:
        ymin = this->StorageHeight;
        ymax = this->StorageHeight + (chromaHeight >> 1);
        for (int yofst = ymin, rowId = 0; yofst < ymax; ++yofst && ++rowId)
        {
          glTexSubImage2D(
            target, 0, 0, yofst, (width >> 1), 1, format, datatype, &dptr[rowId * rowsize]);
          ++rowId;
          glTexSubImage2D(target, 0, (width >> 1), yofst, (width >> 1), 1, format, datatype,
            &dptr[rowId * rowsize]);
        }
        break;
      case 2:
        ymin = this->StorageHeight + (chromaHeight >> 1);
        ymax = this->StorageHeight + chromaHeight;
        for (int yofst = ymin, rowId = 0; yofst < ymax; ++yofst && ++rowId)
        {
          glTexSubImage2D(
            target, 0, 0, yofst, (width >> 1), 1, format, datatype, &dptr[rowId * rowsize]);
          ++rowId;
          glTexSubImage2D(target, 0, (width >> 1), yofst, (width >> 1), 1, format, datatype,
            &dptr[rowId * rowsize]);
        }
        break;
      default:
        break;
    }
  }
  else if (this->PixelFormat == VTKPixelFormatType::VTKPF_NV12)
  {
    switch (plane)
    {
      case 0:
        ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, rowsize);
        glTexSubImage2D(target, 0, 0, 0, width, this->StorageHeight, format, datatype, dptr);
        break;
      case 1:
        ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, rowsize);
        glTexSubImage2D(
          target, 0, 0, this->StorageHeight, width, chromaHeight, format, datatype, dptr);
        break;
      default:
        break;
    }
  }
  else
  {
    ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, rowsize / components);
    ostate->vtkglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(target, 0, 0, 0, rowsize / components, numrows, format, datatype, from);
    ostate->vtkglPixelStorei(GL_UNPACK_ALIGNMENT, oldRowAlign);
  }
  vtkOpenGLCheckErrors("ERROR uploading pixels to OpenGL texture. ");
  ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);

  glBindTexture(target, 0);
  this->Modified();
}

//------------------------------------------------------------------------------
unsigned int vtkOpenGLVideoFrame::GetDataInternal(unsigned char*& data)
{
  auto& internals = (*this->Internals);
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, handle=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(internals.VtkTexture->GetContext()), internals.VtkTexture->GetHandle());

  if (internals.sync != nullptr)
  {
    // 5 seconds seems sensible.
    auto timeout = 5000000000;
    auto result = glClientWaitSync(internals.sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    if (result == GL_TIMEOUT_EXPIRED)
    {
      vtkLogF(WARNING, "GPU unresponsive! timeout=%1.fs.", timeout / 1e9);
      return 0;
    }
    else if (result == GL_WAIT_FAILED)
    {
      vtkLog(ERROR, << "Wait failed!");
      return 0;
    }
    internals.sync = nullptr;
  }

  data = new unsigned char[this->ActualSize];
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
  const auto& internals = (*this->Internals);
  const auto& texture = internals.VtkTexture;
  vtkLogScopeF(TRACE, "%s->%s myWindow=%s, handle=%d", vtkLogIdentifier(this), __func__,
    vtkLogIdentifier(texture->GetContext()), texture->GetHandle());

  auto myWindow = texture->GetContext();
  if (myWindow == nullptr)
  {
    vtkLog(ERROR, << "An OpenGL render window is required. "
                     "Call ::SetContext with an instance of "
                     "vtkOpenGLRenderWindow.");
    return;
  }
  this->ComputeDefaultStrides();
  const auto estimate = vtkRawVideoFrame::GetEstimatedSize(
    this->DisplayWidth, this->DisplayHeight, this->PixelFormat, this->Strides);
  if (this->ActualSize == estimate)
  {
    vtkLogF(TRACE, "ActualSize (%d) == estimate (%d)", this->ActualSize, estimate);
    return;
  }

  const unsigned int chromaHeight =
    vtkRawVideoFrame::GetChromaHeight(this->DisplayHeight, this->PixelFormat);
  const int dataType = VTK_UNSIGNED_CHAR;

  texture->SetMagnificationFilter(vtkTextureObject::Nearest);
  texture->SetMinificationFilter(vtkTextureObject::Nearest);
  texture->SetWrapS(vtkTextureObject::Repeat);
  texture->SetWrapT(vtkTextureObject::Repeat);

  switch (this->PixelFormat)
  {
    case VTKPixelFormatType::VTKPF_RGBA32:
      texture->Allocate2D(this->Strides[0] >> 2, this->StorageHeight, 4, dataType, 0);
      break;
    case VTKPixelFormatType::VTKPF_RGB24:
      texture->Allocate2D(this->Strides[0] / 3, this->StorageHeight, 3, dataType, 0);
      break;
    case VTKPixelFormatType::VTKPF_NV12:
      texture->Allocate2D(this->Strides[0], this->StorageHeight + chromaHeight, 1, dataType, 0);
      break;
    case VTKPixelFormatType::VTKPF_IYUV:
      texture->Allocate2D(this->Strides[0], this->StorageHeight + chromaHeight, 1, dataType, 0);
      break;
  }
  vtkOpenGLCheckErrors("ERROR allocating gl texture. ");
  this->ActualSize = texture->GetWidth() * texture->GetHeight() * texture->GetComponents();

  vtkLogF(TRACE, "DisplayWidth: %d, DisplayHeight: %d", this->DisplayWidth, this->DisplayHeight);
  vtkLogF(TRACE, "StorageWidth: %d, StorageHeight: %d", this->StorageWidth, this->StorageHeight);
  vtkLogF(TRACE, "TextureWidth: %d, TextureHeight: %d", texture->GetWidth(), texture->GetHeight());
  vtkLogF(TRACE, "Strides: %d|%d|%d", this->Strides[0], this->Strides[1], this->Strides[2]);
  vtkLogF(TRACE, "ActualSize: %d", this->ActualSize);
  vtkLogF(TRACE, "handle=%d, internalformat=%06x, format=%06x ", texture->GetHandle(),
    texture->GetInternalFormat(dataType, 1, 0), texture->GetFormat(dataType, 1, 0));

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
    // setup synchronization.
    internals.sync = glFrame->Internals->sync;
    glFrame->Internals->sync = nullptr;
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
    auto array = from->GetData();
    this->CopyDataInternal(array->GetPointer(0), from->GetStrides()[0], from->GetStorageHeight());
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
      int width = this->Strides[0] / internals.VtkTexture->GetComponents();
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
  const bool invert_y = this->SliceOrder == vtkRawVideoFrame::SliceOrderType::TopDown;
  const int chromaHeight =
    vtkRawVideoFrame::GetChromaHeight(this->DisplayHeight, this->PixelFormat);
  const auto lumaHeight = this->StorageHeight;
  auto& tex = internals.VtkTexture;
  this->FrameRenderer->Render(
    tex, this->PixelFormat, oglRenWin, this->Strides, lumaHeight, chromaHeight, invert_y);
  vtkOpenGLCheckErrors("ERROR rendering texture. ");
}
