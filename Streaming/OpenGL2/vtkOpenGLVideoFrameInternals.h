// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
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
