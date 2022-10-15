/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVideoFrameCapture.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLVideoFrameCapture
 * @brief   class that can capture a vtk opengl render window's display into an
 *          inverted or upright RGB24 texture.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkOpenGLHelper.h"
#include "vtkPixelFormatTypes.h"

class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLVideoFrameCapture
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Capture(vtkTextureObject* destTexture, VTKPixelFormatType destPixFmt,
    vtkOpenGLRenderWindow* window, int destWidth, int destHeight, int lumaHeight, int chromaHeight,
    int* strides = nullptr, bool invert_y = false, bool ignore_alpha = true);

private:
  vtkOpenGLHelper DrawHelper;
};
// VTK-HeaderTest-Exclude: vtkOpenGLVideoFrameCapture.h
