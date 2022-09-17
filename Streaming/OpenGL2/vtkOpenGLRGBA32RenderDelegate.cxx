/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRGBA32RenderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRGBA32RenderDelegate.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
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

void vtkOpenGLRGBA32RenderDelegate::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLRGBA32RenderDelegate::Render(
  vtkTextureObject* rgba32Texture, vtkOpenGLRenderWindow* window, bool invert_y /* = false*/)
{
  vtkLogScopeF(TRACE, "%s textureContext=%s, window=%s, Texture=%d", __func__,
    vtkLogIdentifier(rgba32Texture->GetContext()), vtkLogIdentifier(window),
    rgba32Texture->GetHandle());

  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    // create shader programs.
    std::string VSSource = ::VertexShader;
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    std::string GSSource;

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D rgba32Texture;\n"
      "uniform int windowHeight;\n");
    if (invert_y)
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
        "float yCoord = windowHeight - gl_FragCoord.y - 0.5;\n"
        "//VTK::FSQ::Impl");
    }
    else
    {
      vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
        "float yCoord = gl_FragCoord.y - 0.5;\n"
        "//VTK::FSQ::Impl");
    }
    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Impl",
      "ivec2 pixelPos = ivec2(gl_FragCoord.x - 0.5, yCoord);\n"
      "float r = texelFetch(rgba32Texture, pixelPos, 0).x;\n"
      "\n"
      "float g = texelFetch(rgba32Texture, pixelPos, 0).y;\n"
      "\n"
      "float b = texelFetch(rgba32Texture, pixelPos, 0).z;\n"
      "\n"
      "float a = texelFetch(rgba32Texture, pixelPos, 0).w;\n"
      "gl_FragData[0] = vec4(r,g,b,a);\n");
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
    rgba32Texture->Activate();
    program->SetUniformi("rgba32Texture", rgba32Texture->GetTextureUnit());
    program->SetUniformi("windowHeight", window->GetSize()[1]);
    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    rgba32Texture->Deactivate();

    if (startedWindowRender)
    {
      window->End();
      window->Frame();
    }
  }
}
