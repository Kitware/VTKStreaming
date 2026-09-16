// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkOpenGLVideoFrameCapture
 * @brief   class that can capture a vtk opengl render window's display into an
 *          inverted or upright RGB24/RGBA32/IYUV/NV12 texture.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkNew.h"
#include "vtkOpenGLHelper.h"
#include "vtkPixelFormatTypes.h"

class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLVideoFrameCapture
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Capture(vtkTextureObject* rgba32Texture, VTKPixelFormatType destPixFmt,
    vtkOpenGLRenderWindow* window, int destWidth, int destHeight, int lumaHeight, int chromaHeight,
    int* strides = nullptr, bool invert_y = false, bool ignore_alpha = true);

private:
  vtkOpenGLHelper DrawHelper;
  vtkOpenGLHelper ChromaDrawHelper;
  vtkNew<vtkOpenGLFramebufferObject> PlaneFrameBuffer;
};
// VTK-HeaderTest-Exclude: vtkOpenGLVideoFrameCapture.h
