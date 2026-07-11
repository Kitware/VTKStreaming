// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "vtkOpenGLVideoFrameRenderer.h"
#include "vtkIYUVRenderFS.h"
#include "vtkLogger.h"
#include "vtkNV12RenderFS.h"
#include "vtkObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRGB24RenderFS.h"
#include "vtkRGBA32RenderFS.h"
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

void vtkOpenGLVideoFrameRenderer::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLVideoFrameRenderer::Render(vtkTextureObject* srcTexture, VTKPixelFormatType srcPixFmt,
  vtkOpenGLRenderWindow* window, int strides[3], int lumaHeight, int chromaHeight,
  bool invert_y /* = false*/)
{
  vtkLogScopeF(TRACE, "%s textureContext=%s, window=%s, Texture=%d", __func__,
    vtkLogIdentifier(srcTexture->GetContext()), vtkLogIdentifier(window), srcTexture->GetHandle());

  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    // create shader programs.
    std::string VSSource = ::VertexShader;
    std::string FSSource;
    std::string GSSource;
    switch (srcPixFmt)
    {
      case VTKPixelFormatType::VTKPF_RGBA32:
        FSSource = vtkRGBA32RenderFS;
        break;
      case VTKPixelFormatType::VTKPF_RGB24:
        FSSource = vtkRGB24RenderFS;
        break;
      case VTKPixelFormatType::VTKPF_NV12:
        FSSource = vtkNV12RenderFS;
        break;
      case VTKPixelFormatType::VTKPF_IYUV:
        FSSource = vtkIYUVRenderFS;
        break;
    }
    if (invert_y)
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::FLIPY::Impl", "float yCoord = resolution[1] - gl_FragCoord.y - 0.5;\n");
    }
    else
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::FLIPY::Impl", "float yCoord = gl_FragCoord.y - 0.5;\n");
    }
    program = shaderCache->ReadyShaderProgram(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  }

  if (program != nullptr)
  {
    const int width = window->GetActualSize()[0];
    const int height = window->GetActualSize()[1];
    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };
    GLuint iboData[] = { 0, 1, 2, 2, 1, 3 };
    bool startedWindowRender = false;

    if (!window->CheckInRenderStatus())
    {
      window->Start();
      startedWindowRender = true;
    }

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
    state->vtkglViewport(0, 0, width, height);

    // bind and activate the texture before rendering that quad.
    vtkOpenGLState::ScopedglActiveTexture textureSave(state);
    srcTexture->Activate();
    program->SetUniform1iv("resolution", 2, window->GetSize());
    switch (srcPixFmt)
    {
      case VTKPixelFormatType::VTKPF_NV12:
        program->SetUniformi("nv12Texture", srcTexture->GetTextureUnit());
        program->SetUniformi("lumaHeight", lumaHeight);
        break;
      case VTKPixelFormatType::VTKPF_IYUV:
        program->SetUniformi("iyuvTexture", srcTexture->GetTextureUnit());
        program->SetUniform1iv("strides", 3, strides);
        program->SetUniformi("chromaHeight", chromaHeight);
        program->SetUniformi("lumaHeight", lumaHeight);
        break;
      case VTKPixelFormatType::VTKPF_RGB24:
      case VTKPixelFormatType::VTKPF_RGBA32:
        break;
    }
    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    srcTexture->Deactivate();

    if (startedWindowRender)
    {
      window->End();
      window->Frame();
    }
  }
}
