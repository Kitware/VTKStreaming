/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRGB24CaptureDelegate.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLRGB24CaptureDelegate
 * @brief   class that can capture a vtk opengl render window's display into an
 *          inverted or upright RGB24 texture.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkOpenGLHelper.h"

class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLRGB24CaptureDelegate
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Capture(vtkTextureObject* rgba32Texture, vtkOpenGLRenderWindow* window, int destWidth,
    int destHeight, bool invert_y = false);

private:
  vtkOpenGLHelper DrawHelper;
};
// VTK-HeaderTest-Exclude: vtkOpenGLRGB24CaptureDelegate.h
