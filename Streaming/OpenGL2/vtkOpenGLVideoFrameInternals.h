/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVideoFrameInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLVideoFrameInternals
 * @brief   private data members for vtkOpenGLVideoFrame
 *
 * @sa vtkCompressedVideoPacket, vtkVideoEncoder, vtkVideoDecoder, vtkPixelFormatTypes
 */

#ifndef vtkOpenGLVideoFrameInternals_h
#define vtkOpenGLVideoFrameInternals_h

#include "vtkNew.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkTextureObject.h"
#include "vtkUnsignedCharArray.h"
#include <thread>

#include <vtk_glad.h>

class vtkOpenGLVideoFrameInternals
{
public:
  vtkNew<vtkTextureObject> VtkTexture;
  vtkNew<vtkOpenGLFramebufferObject> VtkFrameBuffer;
  std::thread::id Tid{ std::this_thread::get_id() };
  GLsync sync = nullptr;
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenGLVideoFrameInternals.h
