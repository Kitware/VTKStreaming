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
 * @brief   class that represents a raw video frame backed by an OpenGL texture.
 *
 * @warning: The Copy methods are safe only when either the
 * source and destination frames' contexts are setup with
 * OpenGL object sharing or, both frames share one OpenGL context.
 *
 * @warning: You may request this frame to copy data from another,
 * OpenGL does not provide any guarantee that the copy is finished upon return.
 * Use fences where appropriate.
 *
 * @sa vtkCompressedVideoPacket
 */

#ifndef vtkOpenGLVideoFrame_h
#define vtkOpenGLVideoFrame_h

#include "vtkRawVideoFrame.h"

#include "vtkStreamingOpenGL2Module.h" // for export macro

#include <memory> // for ivar

class vtkRenderWindow;
class vtkOpenGLHelper;
class vtkTextureObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLFramebufferObject;

class vtkOpenGLIYUVCaptureDelegate;
class vtkOpenGLNV12CaptureDelegate;
class vtkOpenGLRGBA32CaptureDelegate;
class vtkOpenGLRGB24CaptureDelegate;
class vtkOpenGLIYUVRenderDelegate;
class vtkOpenGLNV12RenderDelegate;
class vtkOpenGLRGB24RenderDelegate;
class vtkOpenGLRGBA32RenderDelegate;
class vtkOpenGLVideoFrameInternals;

class VTKSTREAMINGOPENGL2_EXPORT vtkOpenGLVideoFrame : public vtkRawVideoFrame
{
public:
  vtkTypeMacro(vtkOpenGLVideoFrame, vtkRawVideoFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkOpenGLVideoFrame* New();

  ///@{
  /**
   * Initialize resources with the supplied window's OpenGL context.
   */
  void SetContext(vtkOpenGLRenderWindow* window);
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
   * Copy members and shallow/deep copy the data.
   * ShallowCopy: Assigns the source texture target and handle
   *              to our texture.
   * DeepCopy:    Copies the source texture data into our texture.
   */
  void ShallowCopy(vtkRawVideoFrame* from) noexcept override;
  void DeepCopy(vtkRawVideoFrame* from) override;
  ///@}

protected:
  vtkOpenGLVideoFrame();
  ~vtkOpenGLVideoFrame() override;

  unsigned int ActualSize = 0;

  void CopyDataInternal(unsigned char* from, int rowsize, int numrows) override;
  void CopyPlanarDataInternal(unsigned char* from, int rowsize, int numrows, int plane) override;
  unsigned int GetDataInternal(unsigned char*& data) override;
  void UploadData(unsigned char* from, int rowsize, int numrows, int plane);

private:
  vtkOpenGLVideoFrame(const vtkOpenGLVideoFrame&) = delete;
  void operator=(const vtkOpenGLVideoFrame&) = delete;

  std::unique_ptr<vtkOpenGLIYUVCaptureDelegate> IYUVGrabber;
  std::unique_ptr<vtkOpenGLNV12CaptureDelegate> NV12Grabber;
  std::unique_ptr<vtkOpenGLRGB24CaptureDelegate> RGB24Grabber;
  std::unique_ptr<vtkOpenGLRGBA32CaptureDelegate> RGBA32Grabber;

  std::unique_ptr<vtkOpenGLIYUVRenderDelegate> IYUVRenderer;
  std::unique_ptr<vtkOpenGLNV12RenderDelegate> NV12Renderer;
  std::unique_ptr<vtkOpenGLRGB24RenderDelegate> RGB24Renderer;
  std::unique_ptr<vtkOpenGLRGBA32RenderDelegate> RGBA32Renderer;
  std::unique_ptr<vtkOpenGLVideoFrameInternals> Internals;
};

#endif // vtkOpenGLVideoFrame
