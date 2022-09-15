/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLNV12RenderDelegate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLNV12RenderDelegate.h"
#include "vtkLogger.h"
#include "vtkNV12RenderFS.h"
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

void vtkOpenGLNV12RenderDelegate::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLNV12RenderDelegate::Render(vtkTextureObject* nv12Texture,
  vtkOpenGLRenderWindow* window, int strides[3], int chromaHeight, bool invert_y /* = false*/)
{
  vtkLogScopeF(TRACE, "%s textureContext=%s, window=%s, Texture=%d", __func__,
    vtkLogIdentifier(nv12Texture->GetContext()), vtkLogIdentifier(window),
    nv12Texture->GetHandle());

  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    // create shader programs.
    std::string VSSource = ::VertexShader;
    std::string FSSource = vtkNV12RenderFS;
    std::string GSSource;

    if (invert_y)
    {
      vtkShaderProgram::Substitute(
        FSSource, "//VTK::FLIPY::Impl", "float yCoord = windowDims[1] - gl_FragCoord.y - 0.5;\n");
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
    shaderCache->ReadyShaderProgram(program);
    vtkOpenGLCheckErrors("Error readying shader program ");
    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    // bind and activate these textures before rendering that quad.
    nv12Texture->Activate();
    program->SetUniform1iv("strides", 3, strides);
    program->SetUniform1iv("windowDims", 2, window->GetSize());
    program->SetUniformi("nv12Texture", nv12Texture->GetTextureUnit());
    program->SetUniformi("chromaHeight", chromaHeight);
    GLuint iboData[] = { 0, 1, 2, 2, 1, 3 };

    window->Start();
    vtkOpenGLState* state = window->GetState();
    state->vtkglDisable(GL_SCISSOR_TEST);
    state->vtkglDisable(GL_DEPTH_TEST);

    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    window->End();
    window->Frame();
    nv12Texture->Deactivate();
  }
}
