/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRGBA32CaptureDelegate.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLRGBA32CaptureDelegate
 * @brief   class that can capture a vtk opengl render window's display into an
 *          inverted or upright and(or) alpha-skipped RGBA32 texture.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkOpenGLHelper.h"

class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLRGBA32CaptureDelegate
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Capture(vtkTextureObject* rgba32Texture, vtkOpenGLRenderWindow* window, int destWidth,
    int destHeight, bool invert_y = false, bool ignore_alpha = true);

private:
  vtkOpenGLHelper DrawHelper;
};
// VTK-HeaderTest-Exclude: vtkOpenGLRGBA32CaptureDelegate.h
