/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLIYUVRenderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLIYUVRenderDelegate
 * @brief   class that can render IYUV textures from a vtkOpenGLVideoFrame.
 *
 * @sa vtkOpenGLVideoFrame
 */

#include "vtkOpenGLHelper.h"

class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkOpenGLIYUVRenderDelegate
{
public:
  void ReleaseGraphicsResources(vtkOpenGLRenderWindow* window);
  void Render(vtkTextureObject* iyuvTexture, vtkOpenGLRenderWindow* window, int strides[3],
    int chromaHeight, bool invert_y = false);

private:
  vtkOpenGLHelper DrawHelper;
};
// VTK-HeaderTest-Exclude: vtkOpenGLIYUVRenderDelegate.h