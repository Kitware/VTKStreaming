/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLIYUVCaptureDelegate.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLIYUVCaptureDelegate.h"
#include "vtkIYUVCaptureFS.h"
#include "vtkObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
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

void vtkOpenGLIYUVCaptureDelegate::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLIYUVCaptureDelegate::Capture(vtkTextureObject* rgba32Texture,
  vtkOpenGLRenderWindow* window, int* strides, int chromaHeight, bool invert_y /*=false*/)
{
  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    std::string VSSource = ::VertexShader;
    std::string FSSource = vtkIYUVCaptureFS;
    std::string GSSource;

    if (invert_y)
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::LumaFlipY::Impl", "srcIndex.y = resolution[1] - 1 - srcIndex.y;\n");
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::CrFlipY::Impl", "start.y = resolution[1] - start.y - 2;\n");
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::CbFlipY::Impl", "start.y = resolution[1] - start.y - 2;\n");
    }
    program = shaderCache->ReadyShaderProgram(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  }

  if (program != nullptr)
  {
    const int width = window->GetActualSize()[0];
    const int height = window->GetActualSize()[1] + chromaHeight;
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
    state->vtkglViewport(0, 0, width, height);

    // bind and activate the texture before rendering that quad.
    vtkOpenGLState::ScopedglActiveTexture textureSave(state);
    rgba32Texture->Activate();
    program->SetUniform1iv("resolution", 2, window->GetSize());
    program->SetUniform1iv("strides", 3, strides);
    program->SetUniformi("rgba32Texture", rgba32Texture->GetTextureUnit());
    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    rgba32Texture->Deactivate();
  }
}