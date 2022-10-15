/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVideoFrameCapture.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVideoFrameCapture.h"
#include "vtkIYUVCaptureFS.h"
#include "vtkNV12CaptureFS.h"
#include "vtkObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRGB24CaptureFS.h"
#include "vtkRGBA32CaptureFS.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

namespace
{
const char* VertexShader =
  R"***(
  //VTK::System::Dec
  in vec4 vertexMC;

  void main()
  {
    gl_Position = vertexMC;
  }
  )***";
}

void vtkOpenGLVideoFrameCapture::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLVideoFrameCapture::Capture(vtkTextureObject* destTexture,
  VTKPixelFormatType destPixFmt, vtkOpenGLRenderWindow* window, int destWidth, int destHeight,
  int lumaHeight, int chromaHeight, int* strides /*= nullptr*/, bool invert_y /*= false*/,
  bool ignore_alpha /*= true*/)
{
  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    std::string VSSource = ::VertexShader;
    std::string GSSource;
    std::string FSSource;

    switch (destPixFmt)
    {
      case VTKPixelFormatType::VTKPF_RGBA32:
        FSSource = vtkRGBA32CaptureFS;
        if (invert_y)
        {
          vtkShaderProgram::Substitute(
            FSSource, "//VTK::RGBA32FlipY::Impl", "id_RGBA.y = resolution[1] - 1 - id_RGBA.y;\n");
        }
        break;
      case VTKPixelFormatType::VTKPF_RGB24:
        FSSource = vtkRGB24CaptureFS;
        if (invert_y)
        {
          vtkShaderProgram::Substitute(
            FSSource, "//VTK::RGB24FlipY::Impl", "id_RGBA.y = resolution[1] - 1 - id_RGBA.y;\n");
        }
        break;
      case VTKPixelFormatType::VTKPF_NV12:
        FSSource = vtkNV12CaptureFS;
        if (invert_y)
        {
          vtkShaderProgram::Substitute(
            FSSource, "//VTK::LumaFlipY::Impl", "id_RGBA.y = resolution[1] - 1 - id_RGBA.y;\n");
          vtkShaderProgram::Substitute(FSSource, "//VTK::CrFlipY::Impl",
            "id_RGBA_start.y = resolution[1] - id_RGBA_start.y - 2;\n");
          vtkShaderProgram::Substitute(FSSource, "//VTK::CbFlipY::Impl",
            "id_RGBA_start.y = resolution[1] - id_RGBA_start.y - 2;\n");
        }
        break;
      case VTKPixelFormatType::VTKPF_IYUV:
        FSSource = vtkIYUVCaptureFS;
        if (invert_y)
        {
          vtkShaderProgram::Substitute(
            FSSource, "//VTK::LumaFlipY::Impl", "id_RGBA.y = resolution[1] - 1 - id_RGBA.y;\n");
          vtkShaderProgram::Substitute(FSSource, "//VTK::CrFlipY::Impl",
            "id_RGBA_start.y = resolution[1] - id_RGBA_start.y - 2;\n");
          vtkShaderProgram::Substitute(FSSource, "//VTK::CbFlipY::Impl",
            "id_RGBA_start.y = resolution[1] - id_RGBA_start.y - 2;\n");
        }
        break;
    }
    program = shaderCache->ReadyShaderProgram(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  }

  if (program != nullptr)
  {
    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };
    GLuint iboData[] = { 0, 1, 2, 2, 1, 3 };

    shaderCache->ReadyShaderProgram(program);
    vtkOpenGLCheckErrors("Error readying shader program ");

    vtkOpenGLState* state = window->GetState();
    vtkOpenGLState::ScopedglEnableDisable scissorTestSave(state, GL_SCISSOR_TEST);
    state->vtkglDisable(GL_SCISSOR_TEST);

    vtkOpenGLState::ScopedglEnableDisable depthTestSave(state, GL_DEPTH_TEST);
    state->vtkglDisable(GL_DEPTH_TEST);

    vtkOpenGLState::ScopedglEnableDisable blendSave(state, GL_BLEND);
    state->vtkglDisable(GL_BLEND);

    vtkOpenGLState::ScopedglViewport vportSave(state);
    switch (destPixFmt)
    {
      case VTKPixelFormatType::VTKPF_RGBA32:
      case VTKPixelFormatType::VTKPF_RGB24:
        state->vtkglViewport(0, 0, destWidth, destHeight);
        break;
      case VTKPixelFormatType::VTKPF_NV12:
      case VTKPixelFormatType::VTKPF_IYUV:
        state->vtkglViewport(0, 0, strides[0], lumaHeight + chromaHeight);
        break;
    }

    // bind and activate the texture before rendering that quad.
    vtkOpenGLState::ScopedglActiveTexture textureSave(state);
    destTexture->Activate();
    program->SetUniform1iv("resolution", 2, window->GetSize());
    program->SetUniformi("destTexture", destTexture->GetTextureUnit());
    switch (destPixFmt)
    {
      case VTKPixelFormatType::VTKPF_NV12:
        program->SetUniformi("lumaHeight", lumaHeight);
        program->SetUniformi("chromaHeight", chromaHeight);
        break;
      case VTKPixelFormatType::VTKPF_IYUV:
        program->SetUniform1iv("strides", 3, strides);
        program->SetUniformi("lumaHeight", lumaHeight);
        program->SetUniformi("chromaHeight", chromaHeight);
        break;
      default:
        break;
    }
    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    destTexture->Deactivate();
  }
}
