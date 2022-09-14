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
    shaderCache->ReadyShaderProgram(program);
    vtkOpenGLCheckErrors("Error readying shader program ");
    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    // bind and activate these textures before rendering that quad.
    rgba32Texture->Activate();
    program->SetUniformi("rgba32Texture", rgba32Texture->GetTextureUnit());
    program->SetUniformi("windowHeight", window->GetSize()[1]);
    GLuint iboData[] = { 0, 1, 2, 2, 1, 3 };

    window->Start();
    vtkOpenGLState* state = window->GetState();
    state->vtkglDisable(GL_SCISSOR_TEST);
    state->vtkglDisable(GL_DEPTH_TEST);

    vtkOpenGLRenderUtilities::RenderTriangles(
      verts, 4, iboData, 6, nullptr, program, this->DrawHelper.VAO);
    window->End();
    window->Frame();
    rgba32Texture->Deactivate();
  }
}
