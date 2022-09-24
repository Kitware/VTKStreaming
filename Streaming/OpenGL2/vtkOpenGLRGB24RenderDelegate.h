/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRGB24RenderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLRGB24RenderDelegate
 * @brief   class that can render RGB24 textures from a vtkOpenGLVideoFrame.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkOpenGLHelper.h"

class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLRGB24RenderDelegate
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Render(vtkTextureObject* rgb24Texture, vtkOpenGLRenderWindow* window, bool invert_y = false);

private:
  vtkOpenGLHelper DrawHelper;
};
// VTK-HeaderTest-Exclude: vtkOpenGLRGB24RenderDelegate.h
