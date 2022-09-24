/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVideoFrame.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLVideoFrame
 * @brief   class that represents a raw video frame with data in the host (CPU) address space.
 *
 * @sa vtkCompressedVideoPacket, vtkVideoEncoder, vtkVideoDecoder, vtkPixelFormatTypes
 */

#ifndef vtkOpenGLVideoFrame_h
#define vtkOpenGLVideoFrame_h

#include "vtkRawVideoFrame.h"

#include "vtkStreamingOpenGL2Module.h" // for export macro
#include "vtkUnsignedCharArray.h"      // for ivar

class vtkRenderWindow;
class vtkOpenGLHelper;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLFramebufferObject;

class vtkOpenGLIYUVRenderDelegate;
class vtkOpenGLNV12RenderDelegate;
class vtkOpenGLRGB24RenderDelegate;
class vtkOpenGLRGBA32RenderDelegate;

class VTKSTREAMINGOPENGL2_EXPORT vtkOpenGLVideoFrame : public vtkRawVideoFrame
{
public:
  vtkTypeMacro(vtkOpenGLVideoFrame, vtkRawVideoFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkOpenGLVideoFrame* New();

  ///@{
  /**
   * Initialize resources with window's OpenGL context.
   */
  void InitializeGraphicsResources(vtkRenderWindow* window);
  void ReleaseGraphicsResources();
  ///@}

  using vtkRawVideoFrame::CopyData;
  using vtkRawVideoFrame::GetData;

  void Render(vtkRenderWindow* window) override;
  void Capture(vtkRenderWindow* window) override;

  ///@{
  /**
   * Parent class API for memory management.
   */
  unsigned int GetActualSize() const override;
  void AllocateDataStore() override;
  void* GetResourceHandle() noexcept override;
  ///@}

  ///@{
  /**
   * Methods to copy members and data. Subclasses may copy device <-> host memory.
   * CopyData Assumes that underlying buffer is allocated.
   */
  void CopyMetadata(vtkRawVideoFrame* from) noexcept override;
  ///@}

protected:
  vtkOpenGLVideoFrame();
  ~vtkOpenGLVideoFrame() override;

  vtkNew<vtkUnsignedCharArray> Cache;
  unsigned int ActualSize = 0;
  vtkTextureObject* Texture = nullptr;
  vtkOpenGLFramebufferObject* FBO = nullptr;

  void UploadData(unsigned char* data);

  void CopyDataInternal(unsigned char* from, unsigned int size) override;
  unsigned int GetDataInternal(unsigned char*& data) const override;
  void CopyFrameDataInternal(vtkRawVideoFrame* from) override;

private:
  vtkOpenGLVideoFrame(const vtkOpenGLVideoFrame&) = delete;
  void operator=(const vtkOpenGLVideoFrame&) = delete;

  std::unique_ptr<vtkOpenGLIYUVRenderDelegate> IYUVDelegate;
  std::unique_ptr<vtkOpenGLNV12RenderDelegate> NV12Delegate;
  std::unique_ptr<vtkOpenGLRGB24RenderDelegate> RGB24Delegate;
  std::unique_ptr<vtkOpenGLRGBA32RenderDelegate> RGBA32Delegate;
};

#endif // vtkOpenGLVideoFrame
