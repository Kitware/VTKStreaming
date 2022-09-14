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

#include "vtkOpenGLRGB24RenderDelegate.h"
#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

void vtkOpenGLRGB24RenderDelegate::ReleaseGraphicsResources(vtkOpenGLRenderWindow* window)
{
  this->DrawHelper.ReleaseGraphicsResources(window);
}

void vtkOpenGLRGB24RenderDelegate::Render(
  vtkTextureObject* rgb24Texture, vtkOpenGLRenderWindow* window, bool invert_y /* = false*/)
{
  vtkLogScopeF(TRACE, "%s textureContext=%s, window=%s, Texture=%d", __func__,
    vtkLogIdentifier(rgb24Texture->GetContext()), vtkLogIdentifier(window),
    rgb24Texture->GetHandle());

  vtkShaderProgram* program = this->DrawHelper.Program;
  vtkOpenGLShaderCache* shaderCache = window->GetShaderCache();

  if (program == nullptr)
  {
    // create shader programs.
    std::string VSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader();
    std::string FSSource = vtkOpenGLRenderUtilities::GetFullScreenQuadFragmentShaderTemplate();
    std::string GSSource;

    vtkShaderProgram::Substitute(FSSource, "//VTK::FSQ::Decl",
      "uniform sampler2D rgb24Texture;\n"
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
      "gl_FragData[0] = vec4(r,g,b,1.0f);\n");
    program = shaderCache->ReadyShaderProgram(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  }

  if (program != nullptr)
  {
    shaderCache->ReadyShaderProgram(program);
    vtkOpenGLCheckErrors("Error readying shader program ");
    float verts[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

    // bind and activate these textures before rendering that quad.
    rgb24Texture->Activate();
    program->SetUniformi("rgb24Texture", rgb24Texture->GetTextureUnit());
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
    rgb24Texture->Deactivate();
  }
}
